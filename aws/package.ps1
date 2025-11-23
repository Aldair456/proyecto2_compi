# Script PowerShell para empaquetar el Lambda

Write-Host "📦 Empaquetando Lambda function..." -ForegroundColor Cyan

# Directorios
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$LambdaDir = Join-Path $ScriptDir "lambda"
$PackageDir = Join-Path $ScriptDir "package"
$ZipFile = Join-Path $ScriptDir "lambda_package.zip"

# Limpiar directorio de empaquetado
Write-Host "🧹 Limpiando directorio de empaquetado..." -ForegroundColor Yellow
if (Test-Path $PackageDir) {
    Remove-Item -Recurse -Force $PackageDir
}
New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null

# Copiar lambda function
Write-Host "📋 Copiando lambda function..." -ForegroundColor Yellow
Copy-Item (Join-Path $LambdaDir "lambda_function.py") $PackageDir

# Copiar archivos fuente del compilador
Write-Host "📋 Copiando archivos fuente del compilador..." -ForegroundColor Yellow

# Crear estructura de directorios
New-Item -ItemType Directory -Path (Join-Path $PackageDir "scanner") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $PackageDir "parser") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $PackageDir "visitors") -Force | Out-Null

# Copiar main.cpp
Copy-Item (Join-Path $ProjectRoot "main.cpp") $PackageDir

# Copiar scanner
Copy-Item (Join-Path $ProjectRoot "scanner\*.cpp") (Join-Path $PackageDir "scanner\")
Copy-Item (Join-Path $ProjectRoot "scanner\*.h") (Join-Path $PackageDir "scanner\")

# Copiar parser
Copy-Item (Join-Path $ProjectRoot "parser\*.cpp") (Join-Path $PackageDir "parser\")
Copy-Item (Join-Path $ProjectRoot "parser\*.h") (Join-Path $PackageDir "parser\")

# Copiar visitors
Copy-Item (Join-Path $ProjectRoot "visitors\*.cpp") (Join-Path $PackageDir "visitors\")
Copy-Item (Join-Path $ProjectRoot "visitors\*.h") (Join-Path $PackageDir "visitors\")

# Crear ZIP
Write-Host "📦 Creando ZIP..." -ForegroundColor Yellow
if (Test-Path $ZipFile) {
    Remove-Item -Force $ZipFile
}

Compress-Archive -Path "$PackageDir\*" -DestinationPath $ZipFile -Force

# Mostrar tamaño
$Size = (Get-Item $ZipFile).Length / 1MB
Write-Host "✅ Package creado: $ZipFile ($([math]::Round($Size, 2)) MB)" -ForegroundColor Green

# Mostrar contenido
Write-Host ""
Write-Host "📄 Contenido del package:" -ForegroundColor Cyan
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($ZipFile)
$zip.Entries | Select-Object -First 20 | ForEach-Object { Write-Host "   $($_.FullName)" }
$zip.Dispose()

Write-Host ""
Write-Host "🚀 Para desplegar:" -ForegroundColor Cyan
Write-Host "   aws lambda update-function-code \`" -ForegroundColor White
Write-Host "     --function-name compiler-debugger-dev-compile \`" -ForegroundColor White
Write-Host "     --zip-file fileb://$ZipFile" -ForegroundColor White

