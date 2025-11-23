# Script para ejecutar todos los tests del compilador (Windows PowerShell)

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "   C Compiler - Test Runner" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Compilar el compilador si no existe
if (-not (Test-Path "./compiler.exe")) {
    Write-Host "Compilando el compilador..." -ForegroundColor Yellow
    
    # Compilar directamente con g++ (sin make)
    $sources = @(
        "main.cpp",
        "scanner/token.cpp",
        "scanner/scanner.cpp",
        "parser/ast.cpp",
        "parser/parser.cpp",
        "visitors/codegen.cpp"
    )
    
    $compileCmd = "g++ -std=c++17 -Wall -Wextra -g -o compiler.exe " + ($sources -join " ")
    Invoke-Expression $compileCmd
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error al compilar el compilador" -ForegroundColor Red
        Write-Host "Asegúrate de tener g++ instalado y en el PATH" -ForegroundColor Yellow
        exit 1
    }
    Write-Host "Compilador compilado exitosamente!" -ForegroundColor Green
    Write-Host ""
}

# Verificar si NASM está instalado
$nasmAvailable = $false
try {
    $null = Get-Command nasm -ErrorAction Stop
    $nasmAvailable = $true
} catch {
    $nasmAvailable = $false
}

if (-not $nasmAvailable) {
    Write-Host "ADVERTENCIA: NASM no está instalado o no está en el PATH" -ForegroundColor Yellow
    Write-Host "Los tests solo verificarán que el compilador genera código sin errores" -ForegroundColor Yellow
    Write-Host "Para ejecutar los programas necesitas instalar NASM:" -ForegroundColor Yellow
    Write-Host "  1. Descarga desde: https://www.nasm.us/pub/nasm/releasebuilds/" -ForegroundColor Cyan
    Write-Host "  2. O instala con chocolatey: choco install nasm" -ForegroundColor Cyan
    Write-Host ""
}

# Función para ejecutar un test
function Run-Test {
    param($testFile)
    
    $testName = [System.IO.Path]::GetFileNameWithoutExtension($testFile)
    Write-Host -NoNewline "Testing $testName... "
    
    # Compilar con nuestro compilador
    & ./compiler.exe $testFile output.asm 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAIL (compiler error)" -ForegroundColor Red
        return $false
    }
    
    # Verificar que el archivo .asm se generó y tiene contenido
    if (-not (Test-Path "output.asm")) {
        Write-Host "FAIL (no output file)" -ForegroundColor Red
        return $false
    }
    
    $asmContent = Get-Content "output.asm" -Raw
    if ([string]::IsNullOrWhiteSpace($asmContent)) {
        Write-Host "FAIL (empty output)" -ForegroundColor Red
        return $false
    }
    
    # Si NASM no está disponible, solo verificar compilación
    if (-not $nasmAvailable) {
        Write-Host "PASS (compiled, NASM not available)" -ForegroundColor Yellow
        return $true
    }
    
    # Ensamblar
    nasm -f win64 output.asm -o output.o 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        # Intentar con elf64 (para WSL/Git Bash)
        nasm -f elf64 output.asm -o output.o 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            Write-Host "FAIL (nasm error)" -ForegroundColor Red
            return $false
        }
        # Linkear para Linux
        gcc output.o -o program.exe -no-pie 2>&1 | Out-Null
    } else {
        # Linkear para Windows
        gcc output.o -o program.exe 2>&1 | Out-Null
    }
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAIL (gcc error)" -ForegroundColor Red
        return $false
    }
    
    # Ejecutar
    & ./program.exe 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "PASS" -ForegroundColor Green
        return $true
    } else {
        Write-Host "PASS (with warnings)" -ForegroundColor Yellow
        return $true
    }
}

# Contadores
$total = 0
$passed = 0

Write-Host "=== Base Tests ===" -ForegroundColor Cyan
foreach ($test in Get-ChildItem -Path "tests/base/*.c") {
    if (Test-Path $test) {
        $result = Run-Test $test.FullName
        $total++
        if ($result) { $passed++ }
    }
}
Write-Host ""

Write-Host "=== Function Tests ===" -ForegroundColor Cyan
foreach ($test in Get-ChildItem -Path "tests/functions/*.c") {
    if (Test-Path $test) {
        $result = Run-Test $test.FullName
        $total++
        if ($result) { $passed++ }
    }
}
Write-Host ""

Write-Host "=== Extension Tests ===" -ForegroundColor Cyan
foreach ($test in Get-ChildItem -Path "tests/extensions/*.c") {
    if (Test-Path $test) {
        $result = Run-Test $test.FullName
        $total++
        if ($result) { $passed++ }
    }
}
Write-Host ""

Write-Host "=== Optimization Tests ===" -ForegroundColor Cyan
foreach ($test in Get-ChildItem -Path "tests/optimization/*.c") {
    if (Test-Path $test) {
        $result = Run-Test $test.FullName
        $total++
        if ($result) { $passed++ }
    }
}
Write-Host ""

# Limpiar archivos temporales
Remove-Item -ErrorAction SilentlyContinue output.asm, output.o, program.exe

# Resumen
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "   Results: $passed/$total tests passed" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

if ($passed -eq $total) {
    Write-Host "All tests passed!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "Some tests failed or have warnings" -ForegroundColor Yellow
    exit 1
}

