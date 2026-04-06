@echo off
echo ============================================================
echo  Busca de Combustiveis - EDA2 Grupo 07
echo ============================================================

if not exist busca_combustivel.exe (
    echo [AVISO] busca_combustivel.exe nao encontrado. Compilando...
    call compilar.bat
    if not exist busca_combustivel.exe exit /b 1
)

echo.
echo Iniciando servidor...
echo Acesse no navegador: http://localhost:8080
echo.
echo Pressione Ctrl+C para encerrar.
echo.

busca_combustivel.exe
