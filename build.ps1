# Usage: .\build.ps1 debug  OR  .\build.ps1 release
param (
    [Parameter(Position = 0)]
    [ValidateSet("debug", "release")]
    [string]$Mode = "debug"
)

# --- Configuration ---
$Compiler = "g++"
$Std      = "-std=c++26"
$Includes = @("-I'C:/dev/C_C++/StdToolset/'", "-I'src'")
$Source   = "src/bundler.cpp"
$OutDir   = "build"

# Ensure the build directory exists
if (!(Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir | Out-Null }

# --- Mode Logic ---
if ($Mode -eq "debug") {
    Write-Host "Building in [DEBUG] mode..."
    $Flags   = "-g -D_DEBUG"
    $Output  = "$OutDir/lexer_debug.exe"
} else {
    Write-Host "Building in [RELEASE] mode..."
    $Flags   = "-O3 -DNDEBUG" # -O3 for max optimization
    $Output  = "$OutDir/lexer.exe"
}
$FullCommand = "$Compiler $Std $Source $($Includes -join ' ') $Flags -o $Output"

Invoke-Expression $FullCommand

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build Successful: $Output"
} else {
    Write-Host "Build Failed with exit code $LASTEXITCODE"
}