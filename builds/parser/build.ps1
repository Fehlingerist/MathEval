# Usage: .\build.ps1 debug  OR  .\build.ps1 release
param (
    [Parameter(Position = 0)]
    [ValidateSet("debug", "release")]
    [string]$Mode = "debug"
)

$BaseDir = $PSScriptRoot;

$Compiler = "g++"
$Std      = "-std=c++26"

$SrcDir = "./src"
$StdToolsetDir = "C:/dev/C_C++/StdToolset"

$Source   = "$BaseDir/bundler.cpp"
$OutDir   = "$BaseDir/../../build"
$Includes = @("-I$StdToolsetDir", "-I$SrcDir")

if ($Mode -eq "debug") {
    Write-Host "Building in [DEBUG] mode..."
    $Flags   = "-g -D_DEBUG"
    $Output  = "$OutDir/parser_debug.exe"
} else {
    Write-Host "Building in [RELEASE] mode..."
    $Flags   = "-O3 -DNDEBUG" 
    $Output  = "$OutDir/parser.exe"
}
$FullCommand = "$Compiler $Std `"$Source`" $($Includes -join ' ') $Flags -o `"$Output`""

Write-Host "Running: $FullCommand"
Invoke-Expression $FullCommand

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build Successful: $Output"
} else {
    Write-Host "Build Failed with exit code $LASTEXITCODE"
}