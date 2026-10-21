<#
.SYNOPSIS
    Brings a local game client folder up to date with the latest build of `main` from GitHub, with no manual steps.

.DESCRIPTION
    After a client change is merged to `main`, the CI workflow builds the Windows client bundle (the program without the game's data).
    This script finds the newest bundle that matches `main`, downloads it, unpacks it and copies it over the client folder, keeping the
    client's `config.ini`, `Data` and `fonts`. Afterwards the only thing left to do is to start `Main.exe` again.

    - If `main` has code changes that no finished build contains yet, it waits for the running build, or starts one (workflow_dispatch).
    - A build that only differs from `main` by documentation is accepted (CI runs for documentation-only merges are often cancelled).
    - It refuses to touch a folder whose `Main.exe` is running: close the game first (the script never closes it).
    - It records what is installed in `bundle-info.txt` inside the client folder and says "already up to date" when nothing changed.

    Needs: the GitHub CLI (`gh`, logged in) and `tar` (included in Windows 10/11).

.PARAMETER Target
    The client folder to update. Defaults to the environment variable OPENMU_CLIENT_DIR.

.PARAMETER Repo
    The client repository on GitHub.

.PARAMETER Ref
    The branch whose build to install.

.PARAMETER WaitMinutes
    How long to wait for a running or started build.

.PARAMETER RunId
    Install the bundle of this CI run instead of the newest matching one (for example to go back to an older build).

.PARAMETER DataSource
    Where to copy `Data` and `fonts` from if the client folder has none (the `src/bin` folder of a client clone).

.PARAMETER LocalBuild
    The FAST path for your own testing: install the output of a local build (the folder with Main.exe, for example
    <dev worktree>\out\build\windows-x64\src\Release) instead of downloading from GitHub. An incremental local build takes about a minute, the
    GitHub build 20 to 40 minutes. Same rules: config.ini, Data and fonts are kept, and the game must not be running.

.PARAMETER Force
    Install even if the same build is already installed.

.EXAMPLE
    .\scripts\update-live-client.ps1 -Target "C:\Users\rause\OneDrive\Desktop\OpenMU Live Client"

.EXAMPLE
    .\scripts\update-live-client.ps1 -Target "C:\OpenMU Live Client" -LocalBuild ".\out\build\windows-x64\src\Release"
#>
[CmdletBinding()]
param(
    [string]$Target = $env:OPENMU_CLIENT_DIR,
    [string]$Repo = 'rauseklis/MuMain',
    [string]$Ref = 'main',
    [int]$WaitMinutes = 60,
    [string]$RunId,
    [string]$DataSource,
    [string]$LocalBuild,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$ArtifactPattern = '^mu-client-windows-native-x64-.*-main$'

function Write-Step([string]$text) { Write-Host "==> $text" }

function Invoke-Gh([string[]]$arguments) {
    $output = & gh @arguments 2>&1
    if ($LASTEXITCODE -ne 0) { throw "gh $($arguments -join ' ') failed: $output" }
    return $output
}

if ([string]::IsNullOrWhiteSpace($Target)) {
    throw 'Say which client folder to update: -Target <folder> (or set the environment variable OPENMU_CLIENT_DIR).'
}
if (-not (Get-Command gh -ErrorAction SilentlyContinue)) { throw 'The GitHub CLI (gh) is not installed.' }
if (-not (Test-Path -LiteralPath $Target)) { New-Item -ItemType Directory -Path $Target | Out-Null }
$Target = (Resolve-Path -LiteralPath $Target).Path

# The game must not be running from this folder: its files are in use, and the script never closes a game.
$running = Get-Process -Name Main -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path.StartsWith($Target, [System.StringComparison]::OrdinalIgnoreCase) }
if ($running) { throw "The game is running from $Target (Main.exe, process $($running.Id)). Close it first, then run this again." }

# --- local build: no GitHub at all ----------------------------------------------------------------------------------------------
if ($LocalBuild) {
    $LocalBuild = (Resolve-Path -LiteralPath $LocalBuild).Path
    if (-not (Test-Path -LiteralPath (Join-Path $LocalBuild 'Main.exe'))) { throw "No Main.exe in $LocalBuild. Build the client first (scripts\build-client-windows.cmd)." }
    Write-Step "Installing the local build $LocalBuild into $Target (your config.ini, Data and fonts are kept)"
    & robocopy $LocalBuild $Target /E /XD Data fonts /XF config.ini /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed with code $LASTEXITCODE." }
    foreach ($folder in 'Data', 'fonts') {
        if (-not (Test-Path -LiteralPath (Join-Path $Target $folder))) {
            if ([string]::IsNullOrWhiteSpace($DataSource) -or -not (Test-Path -LiteralPath (Join-Path $DataSource $folder))) {
                Write-Warning "$folder is missing in $Target. Pass -DataSource <client clone>\src\bin so it can be copied."
            } else {
                Write-Step "Copying $folder from $DataSource ..."
                Copy-Item -LiteralPath (Join-Path $DataSource $folder) -Destination (Join-Path $Target $folder) -Recurse
            }
        }
    }
    if (-not (Test-Path -LiteralPath (Join-Path $Target 'config.ini')) -and (Test-Path -LiteralPath (Join-Path $LocalBuild 'config.ini'))) {
        Copy-Item -LiteralPath (Join-Path $LocalBuild 'config.ini') -Destination (Join-Path $Target 'config.ini')
        Write-Warning 'There was no config.ini: the default one was copied (ServerIP=localhost). Set ServerIP and ServerPort in it.'
    }
    $commit = (& git -C $LocalBuild rev-parse HEAD 2>$null)
    "source=local build $LocalBuild`ncommit=$commit`ninstalled=$((Get-Date).ToString('s'))" | Set-Content -LiteralPath (Join-Path $Target 'bundle-info.txt') -Encoding ascii
    $server = (Select-String -LiteralPath (Join-Path $Target 'config.ini') -Pattern '^(ServerIP|ServerPort)=' | ForEach-Object { $_.Line }) -join '  '
    Write-Host ''
    Write-Host "Done. Settings kept: $server"
    Write-Host "Start the game again: $(Join-Path $Target 'Main.exe')"
    return
}

# --- find the bundle ---------------------------------------------------------------------------------------------------------
$latestSha = (Invoke-Gh @('api', "repos/$Repo/commits/$Ref", '--jq', '.sha')).Trim()
Write-Step "Latest $Ref commit: $($latestSha.Substring(0, 8))"

function Get-WindowsArtifact([string]$runId) {
    $json = Invoke-Gh @('api', "repos/$Repo/actions/runs/$runId/artifacts", '--jq', '[.artifacts[] | select(.expired == false) | {name: .name, size: .size_in_bytes}]')
    $list = $json | ConvertFrom-Json
    return $list | Where-Object { $_.name -match $ArtifactPattern } | Select-Object -First 1
}

# Files that do not change the game: a build that only misses these is as good as the newest commit.
function Test-OnlyHarmlessChanges([string]$fromSha, [string]$toSha) {
    if ($fromSha -eq $toSha) { return $true }
    $files = (Invoke-Gh @('api', "repos/$Repo/compare/$fromSha...$toSha", '--paginate', '--jq', '.files[].filename')) -split "`r?`n" | Where-Object { $_ }
    $changed = $files | Where-Object { $_ -notmatch '^(docs/|nuget-feed/README\.md$|scripts/update-live-client\.ps1$|\.github/ISSUE_TEMPLATE/|.*\.md$)' }
    return -not $changed
}

function Find-Bundle {
    if ($RunId) {
        $run = (Invoke-Gh @('api', "repos/$Repo/actions/runs/$RunId")) | ConvertFrom-Json
        $artifact = Get-WindowsArtifact $RunId
        if (-not $artifact) { throw "Run $RunId has no Windows client bundle (it may have expired or failed)." }
        return [pscustomobject]@{ RunId = "$RunId"; Sha = $run.head_sha; Artifact = $artifact.name; SizeMb = [math]::Round($artifact.size / 1MB) }
    }

    $runs = (Invoke-Gh @('run', 'list', '--repo', $Repo, '--workflow', 'CI', '--branch', $Ref, '--limit', '40', '--json', 'databaseId,headSha,status,conclusion,event,createdAt')) | ConvertFrom-Json
    foreach ($run in ($runs | Sort-Object createdAt -Descending)) {
        $artifact = Get-WindowsArtifact $run.databaseId
        if ($artifact -and (Test-OnlyHarmlessChanges $run.headSha $latestSha)) {
            return [pscustomobject]@{ RunId = "$($run.databaseId)"; Sha = $run.headSha; Artifact = $artifact.name; SizeMb = [math]::Round($artifact.size / 1MB) }
        }
        if ($artifact) { break }   # the newest build already predates a code change: wait for or start a newer one
    }
    return $null
}

$deadline = (Get-Date).AddMinutes($WaitMinutes)
$started = $false
$bundle = Find-Bundle
while (-not $bundle) {
    if ((Get-Date) -gt $deadline) { throw "No finished client build for $($latestSha.Substring(0, 8)) after $WaitMinutes minutes. Look at the Actions tab of $Repo." }

    $runs = (Invoke-Gh @('run', 'list', '--repo', $Repo, '--workflow', 'CI', '--branch', $Ref, '--limit', '20', '--json', 'databaseId,headSha,status')) | ConvertFrom-Json
    $active = $runs | Where-Object { $_.headSha -eq $latestSha -and $_.status -in @('queued', 'in_progress', 'pending', 'waiting', 'requested') }
    if (-not $active -and -not $started) {
        Write-Step 'No build for the latest commit yet: starting one (Actions -> CI -> Run workflow)'
        Invoke-Gh @('workflow', 'run', 'CI', '--repo', $Repo, '--ref', $Ref) | Out-Null
        $started = $true
    }
    Write-Step 'Waiting for the build to finish (checking every 60 seconds)...'
    Start-Sleep -Seconds 60
    $bundle = Find-Bundle
}

Write-Step "Using CI run $($bundle.RunId) (commit $($bundle.Sha.Substring(0, 8)), $($bundle.SizeMb) MB)"

$infoFile = Join-Path $Target 'bundle-info.txt'
if (-not $Force -and -not $RunId -and (Test-Path -LiteralPath $infoFile) -and ((Get-Content -LiteralPath $infoFile -Raw) -match "run=$($bundle.RunId)\b")) {
    Write-Host "Already up to date (run $($bundle.RunId)). Nothing to do. Start Main.exe in $Target"
    return
}

# --- download, unpack, install ------------------------------------------------------------------------------------------------
$work = Join-Path ([System.IO.Path]::GetTempPath()) ("openmu-client-" + [guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    Write-Step 'Downloading the bundle from GitHub...'
    Invoke-Gh @('run', 'download', $bundle.RunId, '--repo', $Repo, '-n', $bundle.Artifact, '-D', $work) | Out-Null
    $archive = Get-ChildItem -LiteralPath $work -Filter '*.tar.gz' | Select-Object -First 1
    if (-not $archive) { throw 'The downloaded artifact has no .tar.gz inside.' }

    $extract = Join-Path $work 'files'
    New-Item -ItemType Directory -Path $extract | Out-Null
    Write-Step 'Unpacking...'
    Push-Location $work
    try { & tar -xzf $archive.Name -C 'files' } finally { Pop-Location }
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath (Join-Path $extract 'Main.exe'))) { throw 'Unpacking failed: no Main.exe in the bundle.' }

    Write-Step "Installing into $Target (your config.ini, Data and fonts are kept)"
    & robocopy $extract $Target /E /XD Data fonts /XF config.ini /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed with code $LASTEXITCODE." }

    # The game's data is not part of the bundle: take it from a client clone if this folder has none.
    foreach ($folder in 'Data', 'fonts') {
        if (-not (Test-Path -LiteralPath (Join-Path $Target $folder))) {
            if ([string]::IsNullOrWhiteSpace($DataSource) -or -not (Test-Path -LiteralPath (Join-Path $DataSource $folder))) {
                Write-Warning "$folder is missing in $Target. Pass -DataSource <client clone>\src\bin so it can be copied."
            } else {
                Write-Step "Copying $folder from $DataSource ..."
                Copy-Item -LiteralPath (Join-Path $DataSource $folder) -Destination (Join-Path $Target $folder) -Recurse
            }
        }
    }

    # A new folder has no config.ini yet: start from the bundle's own (it points at localhost; set the server yourself).
    if (-not (Test-Path -LiteralPath (Join-Path $Target 'config.ini'))) {
        Copy-Item -LiteralPath (Join-Path $extract 'config.ini') -Destination (Join-Path $Target 'config.ini')
        Write-Warning 'There was no config.ini: the default one was copied (ServerIP=localhost). Set ServerIP and ServerPort in it.'
    }

    "run=$($bundle.RunId)`ncommit=$($bundle.Sha)`ninstalled=$((Get-Date).ToString('s'))`nartifact=$($bundle.Artifact)" | Set-Content -LiteralPath $infoFile -Encoding ascii
}
finally {
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
}

$server = (Select-String -LiteralPath (Join-Path $Target 'config.ini') -Pattern '^(ServerIP|ServerPort)=' | ForEach-Object { $_.Line }) -join '  '
Write-Host ''
Write-Host "Done. Client updated to commit $($bundle.Sha.Substring(0, 8)). Settings kept: $server"
Write-Host "Start the game again: $(Join-Path $Target 'Main.exe')"
