g++ -std=c++26 "builders/lexer/bundler.cpp" -I"C:/dev/C_C++/StdToolset/" -I"src" -I"tests" -D_DEBUG -o "test/parser/test.exe" -g

if ($LASTEXITCODE -ne 0) {
    Write-Error "Compilation failed (exit code $LASTEXITCODE)"
    exit $LASTEXITCODE
}
