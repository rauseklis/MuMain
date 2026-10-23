<#
.SYNOPSIS
    Read-only check that this computer is set up for the normal workflow. Changes nothing.

.DESCRIPTION
    The normal workflow: start a session, give it the first prompt, say what to work on, build, push, merge, and the game folder is updated
    by script. This checks everything that workflow needs and prints OK / WARN / FAIL for each item with what to do about it.

    Run it from the client clone:  powershell -ExecutionPolicy Bypass -File scripts\check-setup.ps1 -Target "C:\OpenMU Live Client"

.PARAMETER Target
    The game folder. Defaults to the environment variable OPENMU_CLIENT_DIR.

.PARAMETER ServerHost
    The live server's address.

.PARAMETER ServerPort
    The live server's game port.
#>
[CmdletBinding()]
param(
    [string]$Target = $env:OPENMU_CLIENT_DIR,
    [string]$ServerHost = '109.199.104.230',
    [int]$ServerPort = 44406
)

$ErrorActionPreference = 'Continue'
$root = Split-Path -Parent $PSScriptRoot
# The main clone, also when this script runs from one of its worktrees (those live in <clone>\.worktrees\<name>).
$mainRoot = if ((Split-Path -Leaf (Split-Path -Parent $root)) -eq '.worktrees') { Split-Path -Parent (Split-Path -Parent $root) } else { $root }
$script:fails = 0
$script:warns = 0

function Ok([string]$m) { Write-Host "  OK    $m" }
function Warn([string]$m) { $script:warns++; Write-Host "  WARN  $m" }
function Fail([string]$m) { $script:fails++; Write-Host "  FAIL  $m" }
function Section([string]$m) { Write-Host ''; Write-Host $m }

Section '1. Tools'
if (Get-Command git -ErrorAction SilentlyContinue) { Ok 'git is installed' } else { Fail 'git is not installed' }
if (Get-Command gh -ErrorAction SilentlyContinue) {
    & gh auth status *> $null
    if ($LASTEXITCODE -eq 0) { Ok "GitHub CLI is logged in as $((& gh api user --jq .login 2>$null))" } else { Fail 'GitHub CLI is not logged in: run  gh auth login' }
} else { Fail 'GitHub CLI (gh) is not installed: https://cli.github.com' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if ((Test-Path -LiteralPath $vswhere) -and (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)) {
    Ok 'Visual Studio with the C++ tools is installed'
} else { Fail 'Visual Studio with the "Desktop development with C++" workload was not found (needed to build the client)' }

Section '2. Repositories'
$remote = (& git -C $root remote get-url origin 2>$null)
if ($remote -match 'rauseklis/MuMain') { Ok "this is the client clone ($root)" } else { Fail "this folder is not a clone of rauseklis/MuMain (origin: $remote)" }
& git -C $root fetch origin main --quiet 2>$null
if ($LASTEXITCODE -eq 0) { Ok 'can reach GitHub (git fetch works)' } else { Fail 'git fetch failed: check the internet connection and the GitHub login' }
$agents = (& git -C $root show origin/main:AGENTS.md 2>$null) -join "`n"
if ($agents -match 'Binding rules for every session') { Ok 'AGENTS.md with the binding rules is on main' } else { Warn 'AGENTS.md on main has no binding rules yet' }
$serverClone = @((Join-Path (Split-Path -Parent $mainRoot) 'server'), (Join-Path (Split-Path -Parent $mainRoot) 'OpenMU')) | Where-Object { Test-Path -LiteralPath (Join-Path $_ '.git') } | Select-Object -First 1
if ($serverClone -and ((& git -C $serverClone remote get-url origin 2>$null) -match 'rauseklis/OpenMU')) { Ok "server clone found ($serverClone)" } else { Warn 'no server clone (rauseklis/OpenMU) next to this one; needed only for server changes' }

Section '3. Build folder (the long-lived dev worktree: first build 15-20 minutes, later ones about a minute)'
$dev = Join-Path $mainRoot '.worktrees\dev'
if (Test-Path -LiteralPath $dev) { Ok "dev worktree exists ($dev)" } else { Fail "no dev worktree: run  git worktree add --detach .worktrees/dev origin/main  in the clone, then scripts\build-client-windows.cmd x64 inside it" }
$exe = Join-Path $dev 'out\build\windows-x64\src\Release\Main.exe'
if (Test-Path -LiteralPath $exe) { Ok "a build exists (built $((Get-Item -LiteralPath $exe).LastWriteTime.ToString('yyyy-MM-dd HH:mm')))" } else { Warn 'no build yet in the dev worktree: the first build takes 15-20 minutes' }
if (Test-Path -LiteralPath $dev) {
    $branch = (& git -C $dev branch --show-current 2>$null)
    $dirty = (& git -C $dev status --porcelain --untracked-files=no 2>$null)
    if (-not $branch -and -not $dirty) { Ok 'dev worktree is clean and on a detached origin/main' } else { Warn "dev worktree is on '$branch'$(if ($dirty) { ' with uncommitted changes' }): fine while working, put it back with  git switch --detach origin/main  after a merge" }
}

Section '4. Game folder'
if ([string]::IsNullOrWhiteSpace($Target)) { Fail 'no game folder given: pass -Target <folder> (or set OPENMU_CLIENT_DIR)' }
elseif (-not (Test-Path -LiteralPath $Target)) { Fail "game folder $Target does not exist" }
else {
    if (Test-Path -LiteralPath (Join-Path $Target 'Main.exe')) { Ok "Main.exe is in $Target" } else { Fail "no Main.exe in ${Target}: run scripts\update-live-client.ps1 -Target ""$Target""" }
    foreach ($f in 'Data', 'fonts') { if (Test-Path -LiteralPath (Join-Path $Target $f)) { Ok "$f folder is there" } else { Fail "$f folder is missing: run scripts\update-live-client.ps1, it copies it from the clone" } }
    $ini = Join-Path $Target 'config.ini'
    if (Test-Path -LiteralPath $ini) {
        $ip = (Select-String -LiteralPath $ini -Pattern '^ServerIP=(.*)$' | ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() } | Select-Object -First 1)
        $port = (Select-String -LiteralPath $ini -Pattern '^ServerPort=(.*)$' | ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() } | Select-Object -First 1)
        if ($ip -eq $ServerHost -and $port -eq "$ServerPort") { Ok "config.ini points at the live server ($ip`:$port)" } else { Warn "config.ini has ServerIP=$ip ServerPort=$port, the live server is $ServerHost`:$ServerPort" }
    } else { Fail 'no config.ini in the game folder' }
    if (Get-Process -Name Main -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path.StartsWith($Target, [System.StringComparison]::OrdinalIgnoreCase) }) { Warn 'the game is running from this folder: close it before updating' }
}

Section '5. Live server'
try {
    $c = New-Object System.Net.Sockets.TcpClient
    $iar = $c.BeginConnect($ServerHost, $ServerPort, $null, $null)
    if ($iar.AsyncWaitHandle.WaitOne(5000) -and $c.Connected) { Ok "game port $ServerHost`:$ServerPort answers" } else { Fail "game port $ServerHost`:$ServerPort does not answer" }
    $c.Close()
} catch { Fail "game port $ServerHost`:$ServerPort does not answer ($($_.Exception.Message))" }
if (Get-Command ssh -ErrorAction SilentlyContinue) {
    & ssh -o BatchMode=yes -o ConnectTimeout=8 openmu true *> $null
    if ($LASTEXITCODE -eq 0) { Ok 'ssh openmu works (can read the live server)' } else { Warn 'ssh openmu does not work yet: needed only for server operations; see docs/OPERATIONS.md' }
} else { Warn 'ssh is not installed' }

Write-Host ''
if ($script:fails -gt 0) { Write-Host "Result: $($script:fails) problem(s) to fix (FAIL), $($script:warns) warning(s). Nothing was changed."; exit 1 }
Write-Host "Result: ready to work ($($script:warns) warning(s)). Nothing was changed."
