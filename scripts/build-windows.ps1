param(
    [string]$QtPrefix = $env:QT_ROOT_DIR,
    [string]$BuildDir = "build",
    [switch]$SkipEverythingSdk,
    [switch]$SkipDeploy
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root $BuildDir

$cmakeArgs = @("-S", $Root, "-B", $Build, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release")
if ($QtPrefix) {
    $cmakeArgs += "-DCMAKE_PREFIX_PATH=$QtPrefix"
}

Write-Host "[Quickary] Configuring..."
& cmake @cmakeArgs
Write-Host "[Quickary] Building Release..."
& cmake --build $Build --config Release

$Exe = Get-ChildItem -Path $Build -Filter Quickary.exe -Recurse | Select-Object -First 1
if (-not $Exe) { throw "Quickary.exe was not produced." }

if (-not $SkipDeploy) {
    $windeploy = $null
    if ($QtPrefix) {
        $candidate = Join-Path $QtPrefix "bin/windeployqt.exe"
        if (Test-Path $candidate) { $windeploy = $candidate }
    }
    if (-not $windeploy) {
        $cmd = Get-Command windeployqt.exe -ErrorAction SilentlyContinue
        if ($cmd) { $windeploy = $cmd.Source }
    }
    if ($windeploy) {
        Write-Host "[Quickary] Deploying Qt runtime..."
        & $windeploy --release --no-translations $Exe.FullName
    } else {
        Write-Warning "windeployqt.exe was not found. The executable was built but Qt runtime DLLs were not deployed."
    }
}

if (-not $SkipEverythingSdk) {
    $deps = Join-Path $Root ".deps/everything-sdk"
    $zip = Join-Path $Root ".deps/Everything-SDK.zip"
    New-Item -ItemType Directory -Force -Path (Split-Path $zip) | Out-Null
    if (-not (Test-Path $zip)) {
        Write-Host "[Quickary] Downloading the official Everything SDK..."
        Invoke-WebRequest -Uri "https://www.voidtools.com/Everything-SDK.zip" -OutFile $zip
    }
    if (-not (Test-Path $deps)) {
        Expand-Archive -Path $zip -DestinationPath $deps -Force
    }
    $everythingDll = Get-ChildItem -Path $deps -Filter Everything64.dll -Recurse | Select-Object -First 1
    if ($everythingDll) {
        Copy-Item $everythingDll.FullName (Join-Path $Exe.DirectoryName "Everything64.dll") -Force
        Write-Host "[Quickary] Everything64.dll copied beside Quickary.exe."
    } else {
        Write-Warning "Everything64.dll was not found in the downloaded SDK archive."
    }
}

$everything = Get-Process Everything -ErrorAction SilentlyContinue
if (-not $everything) {
    Write-Warning "Everything is not currently running. Quickary will still launch, but file search needs the standard (non-Lite) Everything client/service running."
}

Write-Host ""
Write-Host "Quickary ready: $($Exe.FullName)"
