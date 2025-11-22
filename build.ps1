# Script para compilar el compilador (sin make)

Write-Host "Compilando el compilador..." -ForegroundColor Yellow

$sources = @(
    "main.cpp",
    "scanner/token.cpp",
    "scanner/scanner.cpp",
    "parser/ast.cpp",
    "parser/parser.cpp",
    "visitors/codegen.cpp"
)

$compileCmd = "g++ -std=c++17 -Wall -Wextra -g -o compiler.exe " + ($sources -join " ")

Write-Host "Ejecutando: $compileCmd" -ForegroundColor Cyan
Invoke-Expression $compileCmd

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nCompilador compilado exitosamente!" -ForegroundColor Green
    Write-Host "Ejecutable: compiler.exe" -ForegroundColor Green
} else {
    Write-Host "`nError al compilar" -ForegroundColor Red
    Write-Host "Asegúrate de tener g++ instalado (MinGW o MSYS2)" -ForegroundColor Yellow
    exit 1
}

