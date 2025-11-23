# Script PowerShell para desplegar el Lambda

Write-Host "🚀 Desplegando Lambda function..." -ForegroundColor Cyan

# Verificar que serverless esté instalado
try {
    $null = Get-Command serverless -ErrorAction Stop
} catch {
    Write-Host "❌ Serverless Framework no está instalado" -ForegroundColor Red
    Write-Host "   Instalar con: npm install -g serverless" -ForegroundColor Yellow
    exit 1
}

# Ir al directorio aws
Set-Location $PSScriptRoot

# Empaquetar primero
Write-Host "📦 Empaquetando..." -ForegroundColor Yellow
& .\package.ps1

# Desplegar
Write-Host "🚀 Desplegando a AWS..." -ForegroundColor Yellow
serverless deploy

Write-Host ""
Write-Host "✅ Deployment completo!" -ForegroundColor Green
Write-Host ""
Write-Host "📋 Para ver los logs:" -ForegroundColor Cyan
Write-Host "   serverless logs -f compile -t" -ForegroundColor White
Write-Host ""
Write-Host "📋 Para eliminar:" -ForegroundColor Cyan
Write-Host "   serverless remove" -ForegroundColor White

