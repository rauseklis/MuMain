<#
.SYNOPSIS
    Makes, signs and checks a list of the files of an installed game client, so damaged or swapped files are found.

.DESCRIPTION
    Three modes:
      NewKey  Creates a signing key pair. The PRIVATE key stays on the machine of the person who publishes releases
              (never in a repository, never on the server). The PUBLIC key is printed and can be shared.
      Sign    Hashes every file of a client folder and writes `client-manifest.json` plus `client-manifest.json.sig`
              (a signature of the exact bytes of the list), made with the private key.
      Verify  Checks the signature with the public key, then compares every file of the folder with the list and
              reports what is missing, changed or additional. It only reads; it never deletes or changes anything.

    Not covered by the list: the settings file, logs, screenshots and the install record (they differ per player).

.EXAMPLE
    .\scripts\client-manifest.ps1 -Mode NewKey -KeyDirectory "$env:USERPROFILE\.ssh\mu-release-signing"

.EXAMPLE
    .\scripts\client-manifest.ps1 -Mode Sign -Folder "C:\OpenMU Live Client" -Build d7f3dc756664f8f6 -PrivateKey "$env:USERPROFILE\.ssh\mu-release-signing\release.private"

.EXAMPLE
    .\scripts\client-manifest.ps1 -Mode Verify -Folder "C:\OpenMU Live Client" -PublicKey ".\scripts\release.public"
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('NewKey', 'Sign', 'Verify')]
    [string]$Mode,

    [string]$Folder,
    [string]$Build,
    [string]$PrivateKey,
    [string]$PublicKey,
    [string]$KeyDirectory,
    [string]$Manifest
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Security

# Files which differ per player or per run, and are therefore not part of the list.
$script:Excluded = @('config.ini', 'bundle-info.txt', 'client-manifest.json', 'client-manifest.json.sig')
$script:ExcludedPatterns = @('*.log', 'ScreenShots\*', 'Screenshots\*')

function Get-RelativeFiles([string]$root) {
    $rootFull = (Resolve-Path $root).Path.TrimEnd('\')
    Get-ChildItem -LiteralPath $rootFull -Recurse -File | ForEach-Object {
        $relative = $_.FullName.Substring($rootFull.Length + 1)
        $skip = $script:Excluded -contains $relative
        foreach ($pattern in $script:ExcludedPatterns) {
            if ($relative -like $pattern) { $skip = $true }
        }

        if (-not $skip) {
            [pscustomobject]@{ Path = $relative.Replace('\', '/'); Full = $_.FullName; Size = $_.Length }
        }
    } | Sort-Object Path
}

function Get-Sha256([string]$path) {
    (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Read-Key([string]$path, [bool]$withPrivate) {
    $text = (Get-Content -LiteralPath $path -Raw).Trim()
    $parts = $text.Split(':')
    $parameters = New-Object System.Security.Cryptography.ECParameters
    $parameters.Curve = [System.Security.Cryptography.ECCurve]::CreateFromFriendlyName('nistP256')
    $point = New-Object System.Security.Cryptography.ECPoint
    $point.X = [Convert]::FromBase64String($parts[0])
    $point.Y = [Convert]::FromBase64String($parts[1])
    $parameters.Q = $point
    if ($withPrivate) { $parameters.D = [Convert]::FromBase64String($parts[2]) }
    [System.Security.Cryptography.ECDsa]::Create($parameters)
}

switch ($Mode) {
    'NewKey' {
        if (-not $KeyDirectory) { throw 'Give -KeyDirectory (a folder OUTSIDE any repository and outside cloud-synced folders).' }
        New-Item -ItemType Directory -Force -Path $KeyDirectory | Out-Null
        $privatePath = Join-Path $KeyDirectory 'release.private'
        if (Test-Path $privatePath) { throw "$privatePath exists already; it is not overwritten." }
        $ec = [System.Security.Cryptography.ECDsa]::Create([System.Security.Cryptography.ECCurve]::CreateFromFriendlyName('nistP256'))
        $p = $ec.ExportParameters($true)
        $publicText = [Convert]::ToBase64String($p.Q.X) + ':' + [Convert]::ToBase64String($p.Q.Y)
        [IO.File]::WriteAllText($privatePath, $publicText + ':' + [Convert]::ToBase64String($p.D))
        $publicPath = Join-Path $KeyDirectory 'release.public'
        [IO.File]::WriteAllText($publicPath, $publicText)
        Write-Host "Private key written to $privatePath. Keep it on this machine only."
        Write-Host "Public key written to $publicPath. It may be shared and committed."
    }

    'Sign' {
        if (-not $Folder -or -not $Build -or -not $PrivateKey) { throw 'Sign needs -Folder, -Build and -PrivateKey.' }
        $files = @(Get-RelativeFiles $Folder | ForEach-Object { [ordered]@{ path = $_.Path; size = $_.Size; sha256 = (Get-Sha256 $_.Full) } })
        $document = [ordered]@{ build = $Build; created = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ'); files = $files }
        $json = $document | ConvertTo-Json -Depth 5
        $manifestPath = Join-Path $Folder 'client-manifest.json'
        $bytes = (New-Object System.Text.UTF8Encoding($false)).GetBytes($json)
        [IO.File]::WriteAllBytes($manifestPath, $bytes)
        $signer = Read-Key $PrivateKey $true
        $signature = $signer.SignData($bytes, [System.Security.Cryptography.HashAlgorithmName]::SHA256)
        [IO.File]::WriteAllText("$manifestPath.sig", [Convert]::ToBase64String($signature))
        Write-Host "Signed list of $($files.Count) files for build $Build written to $manifestPath"
    }

    'Verify' {
        if (-not $Folder -or -not $PublicKey) { throw 'Verify needs -Folder and -PublicKey.' }
        if (-not $Manifest) { $Manifest = Join-Path $Folder 'client-manifest.json' }
        if (-not (Test-Path $Manifest) -or -not (Test-Path "$Manifest.sig")) {
            Write-Host 'RESULT: NO LIST - the signed file list is missing.'
            exit 2
        }

        $bytes = [IO.File]::ReadAllBytes($Manifest)
        $signature = [Convert]::FromBase64String((Get-Content -LiteralPath "$Manifest.sig" -Raw).Trim())
        $verifier = Read-Key $PublicKey $false
        if (-not $verifier.VerifyData($bytes, $signature, [System.Security.Cryptography.HashAlgorithmName]::SHA256)) {
            Write-Host 'RESULT: BAD SIGNATURE - the file list was not made by the release key. Nothing was compared.'
            exit 3
        }

        $document = [Text.Encoding]::UTF8.GetString($bytes) | ConvertFrom-Json
        $expected = @{}
        foreach ($entry in $document.files) { $expected[$entry.path] = $entry }
        $actual = @{}
        foreach ($file in (Get-RelativeFiles $Folder)) { $actual[$file.Path] = $file }

        $missing = @($expected.Keys | Where-Object { -not $actual.ContainsKey($_) } | Sort-Object)
        $additional = @($actual.Keys | Where-Object { -not $expected.ContainsKey($_) } | Sort-Object)
        $changed = @($expected.Keys | Where-Object {
                $actual.ContainsKey($_) -and ($actual[$_].Size -ne $expected[$_].size -or (Get-Sha256 $actual[$_].Full) -ne $expected[$_].sha256)
            } | Sort-Object)

        Write-Host "Build in the list: $($document.build); files in the list: $($expected.Count)"
        $missing | ForEach-Object { Write-Host "MISSING   $_" }
        $changed | ForEach-Object { Write-Host "CHANGED   $_" }
        $additional | ForEach-Object { Write-Host "ADDITIONAL $_" }
        if ($missing.Count -eq 0 -and $changed.Count -eq 0 -and $additional.Count -eq 0) {
            Write-Host 'RESULT: OK - every file matches the signed list.'
            exit 0
        }

        Write-Host "RESULT: DIFFERENT - missing $($missing.Count), changed $($changed.Count), additional $($additional.Count). Nothing was changed or deleted."
        Write-Host 'To repair, run scripts\update-live-client.ps1 -Force (it only adds and replaces files).'
        exit 1
    }
}
