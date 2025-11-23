# Script para ejecutar un solo test

param(
    [Parameter(Mandatory=$true)]
    [string]$TestFile
)

if (-not (Test-Path $TestFile)) {
    Write-Host "Error: Archivo no encontrado: $TestFile" -ForegroundColor Red
    exit 1
}

# Compilar el compilador si no existe
if (-not (Test-Path "./compiler.exe")) {
    Write-Host "Compilando el compilador..." -ForegroundColor Yellow
    & .\build.ps1
    if ($LASTEXITCODE -ne 0) {
        exit 1
    }
}

$testName = [System.IO.Path]::GetFileNameWithoutExtension($TestFile)
Write-Host "`n=== Ejecutando test: $testName ===" -ForegroundColor Cyan

# 1. Compilar con nuestro compilador
Write-Host "1. Compilando con el compilador..." -ForegroundColor Yellow
& ./compiler.exe $TestFile output.asm
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Fallo en la compilación" -ForegroundColor Red
    exit 1
}

# 2. Ensamblar
Write-Host "2. Ensamblando..." -ForegroundColor Yellow
nasm -f win64 output.asm -o output.o 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    # Intentar con elf64 (para WSL/Git Bash)
    Write-Host "   Intentando con formato elf64..." -ForegroundColor Yellow
    nasm -f elf64 output.asm -o output.o 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Fallo en el ensamblado (nasm)" -ForegroundColor Red
        Write-Host "Asegúrate de tener NASM instalado" -ForegroundColor Yellow
        exit 1
    }
    $useElf = $true
} else {
    $useElf = $false
}

# 3. Linkear
Write-Host "3. Linkeando..." -ForegroundColor Yellow
if ($useElf) {
    gcc output.o -o program.exe -no-pie 2>&1 | Out-Null
} else {
    gcc output.o -o program.exe 2>&1 | Out-Null
}
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Fallo en el linking (gcc)" -ForegroundColor Red
    exit 1
}

# 4. Ejecutar
Write-Host "4. Ejecutando programa..." -ForegroundColor Yellow
Write-Host "`n--- Salida del programa ---" -ForegroundColor Green
& ./program.exe
$exitCode = $LASTEXITCODE

Write-Host "`n--- Fin de salida ---" -ForegroundColor Green
Write-Host "`nExit code: $exitCode" -ForegroundColor Cyan

# Limpiar (opcional)
# Remove-Item -ErrorAction SilentlyContinue output.asm, output.o, program.exe

if ($exitCode -eq 0) {
    Write-Host "`n✓ Test completado exitosamente!" -ForegroundColor Green
} else {
    Write-Host "`n✗ Test falló con código de salida $exitCode" -ForegroundColor Red
}

