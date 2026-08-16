<#
.SYNOPSIS
    One-command build + test for Project Apogee.

.DESCRIPTION
    Sets up the exact toolchain this project needs (the Qt-matched MinGW
    13.1.0 compiler -- NOT whatever g++ happens to already be on PATH, which
    may be a different, ABI-incompatible version) for THIS SCRIPT'S PROCESS
    ONLY. It does not modify your permanent system/user PATH -- run this
    script (or one like it) every time you build, rather than expecting a
    plain `cmake --build` to work from an arbitrary terminal.

.PARAMETER Clean
    Delete the build/ directory first and reconfigure from scratch.

.PARAMETER SkipTests
    Configure and build only; don't run apogee_tests.exe afterward.

.PARAMETER BuildGui
    Also configure and build the Qt6 apogee_studio GUI target
    (APOGEE_BUILD_GUI=ON). Off by default so headless data-layer/core work
    doesn't pay Qt Widgets' extra configure/build cost.
#>
param(
    [switch]$Clean,
    [switch]$SkipTests,
    [switch]$BuildGui
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot

# --- Prerequisites this script expects to already be installed --------
$qtMingw = "C:\Users\reach\Dance\Qt\Tools\mingw1310_64\bin"
$qtRoot  = "C:\Users\reach\Dance\Qt\6.9.3\mingw_64"
$qtBin   = "$qtRoot\bin"
$cmakeBin = "C:\Program Files\CMake\bin"

foreach ($p in @($qtMingw, $qtBin, $cmakeBin)) {
    if (-not (Test-Path $p)) {
        throw "Missing prerequisite: $p`nSee BUILD.md for how to install it."
    }
}

# Prepend the exact toolchain this project needs, ahead of anything else
# already on PATH (in particular, ahead of any other g++ that might exist).
# This also keeps Qt's DLLs discoverable for the rest of this script, since
# rocket_data links Qt Core/Sql/Network dynamically.
$env:Path = "$qtMingw;$qtBin;$cmakeBin;$env:Path"

if ($Clean) {
    Write-Host "Removing build/ ..."
    Remove-Item -Recurse -Force "$root\build" -ErrorAction SilentlyContinue
}

Write-Host "Configuring ..."
$guiFlags = @("-DAPOGEE_BUILD_GUI=OFF")
if ($BuildGui) {
    $guiFlags = @("-DAPOGEE_BUILD_GUI=ON")
}
& cmake -S $root -B "$root\build" -G "MinGW Makefiles" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$qtRoot" `
    -DAPOGEE_BUILD_TESTS=ON @guiFlags
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed (exit $LASTEXITCODE)" }

Write-Host "Building ..."
& cmake --build "$root\build" -j 4
if ($LASTEXITCODE -ne 0) { throw "Build failed (exit $LASTEXITCODE)" }

if (-not $SkipTests) {
    Write-Host "Running tests ..."
    & "$root\build\apogee_tests.exe"
    if ($LASTEXITCODE -ne 0) { throw "Tests failed (exit $LASTEXITCODE)" }
}

Write-Host "`nDone. Executables:"
Write-Host "  $root\build\apogee_tests.exe"
if ($BuildGui) {
    Write-Host "  $root\build\apogee_studio.exe"
}
