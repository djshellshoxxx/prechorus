# PreChorus - one-shot Windows build. Right-click > "Run with PowerShell" AS ADMINISTRATOR.
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

function Ensure($cmd, $id) {
    if (-not (Get-Command $cmd -ErrorAction SilentlyContinue)) {
        Write-Host "Installing $id ..."
        winget install --id $id -e --accept-package-agreements --accept-source-agreements
    }
}
Ensure git   Git.Git
Ensure cmake Kitware.CMake

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = $null
if (Test-Path $vswhere) { $vsPath = & $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1 }
if (-not $vsPath) {
    $fallbackPaths = @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools",
        "$env:ProgramFiles\Microsoft Visual Studio\2022\BuildTools",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community",
        "$env:ProgramFiles\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Professional",
        "$env:ProgramFiles\Microsoft Visual Studio\2022\Professional"
    )
    foreach ($p in $fallbackPaths) {
        if (Test-Path "$p\VC\Tools\MSVC") { $vsPath = $p; break }
    }
}
if (-not $vsPath) {
    Write-Host "Installing Visual Studio C++ Build Tools (this is the slow part) ..."
    winget install --id Microsoft.VisualStudio.2022.BuildTools -e --accept-package-agreements --accept-source-agreements `
        --override "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
    if (Test-Path $vswhere) { $vsPath = & $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1 }
    if (-not $vsPath) {
        foreach ($p in $fallbackPaths) {
            if (Test-Path "$p\VC\Tools\MSVC") { $vsPath = $p; break }
        }
    }
}
if (-not $vsPath) { throw "C++ build tools still not found. Open 'Visual Studio Installer', Modify > tick 'Desktop development with C++', then rerun." }
Write-Host "Using compiler at: $vsPath"

$env:Path = [Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [Environment]::GetEnvironmentVariable("Path","User")

if (Test-Path "$root\build\CMakeCache.txt") {
    if (-not (Select-String -Path "$root\build\CMakeCache.txt" -Pattern "Visual Studio" -Quiet)) { Remove-Item "$root\build" -Recurse -Force }
}

$gen = "Visual Studio 17 2022"
if ($vsPath -match "\\2026\\|\\18\\") { $gen = "Visual Studio 18 2026" }

Write-Host "Configuring with generator '$gen' (downloads JUCE on first run) ..."
cmake -S $root -B "$root\build" -G $gen -A x64
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
Write-Host "Building ..."
cmake --build "$root\build" --config Release --target PreChorus_VST3 PreChorus_Standalone
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

$dst = "C:\Program Files\Common Files\VST3\PreChorus.vst3"
$src = Get-ChildItem -Path "$root\build" -Recurse -Directory -Filter "PreChorus.vst3" | Select-Object -First 1
if ($src) { Copy-Item $src.FullName $dst -Recurse -Force }
Write-Host ""
Write-Host "DONE.  VST3: $dst"
Write-Host "Standalone test app: $root\build\PreChorus_artefacts\Release\Standalone\PreChorus.exe"
Read-Host "Press Enter to close"
