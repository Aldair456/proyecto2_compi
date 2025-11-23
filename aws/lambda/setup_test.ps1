# Script PowerShell para configurar el entorno de prueba del Lambda

Write-Host "🔧 Setting up Lambda test environment..." -ForegroundColor Cyan

# Ir al directorio raíz del proyecto
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $ScriptDir)

Write-Host "📁 Project root: $ProjectRoot" -ForegroundColor Yellow
Write-Host "📁 Lambda dir: $ScriptDir" -ForegroundColor Yellow

# Compilar el compilador si no existe
$CompilerPath = Join-Path $ProjectRoot "compiler.exe"
if (-not (Test-Path $CompilerPath)) {
    Write-Host "🔨 Compiling compiler..." -ForegroundColor Yellow
    Set-Location $ProjectRoot
    & .\build.ps1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Failed to compile compiler" -ForegroundColor Red
        exit 1
    }
}

# Copiar compilador al directorio lambda
if (Test-Path $CompilerPath) {
    Write-Host "📋 Copying compiler to lambda directory..." -ForegroundColor Yellow
    Copy-Item $CompilerPath (Join-Path $ScriptDir "compiler.exe")
} else {
    Write-Host "❌ Compiler not found at $CompilerPath" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Setup complete!" -ForegroundColor Green
Write-Host ""
Write-Host "🚀 To test the Lambda handler:" -ForegroundColor Cyan
Write-Host "   cd aws/lambda" -ForegroundColor White
Write-Host "   python test_lambda.py test_event_simple.json" -ForegroundColor White

