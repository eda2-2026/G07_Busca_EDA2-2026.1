@echo off
echo ============================================================
echo  Compilando Busca de Combustiveis - EDA2 Grupo 07
echo ============================================================

REM Tenta usar gcc do PATH (MinGW, WinLibs, etc.)
where gcc >nul 2>&1
if %ERRORLEVEL% == 0 (
    echo Usando: gcc
    gcc -Wall -O2 -std=c99 -o busca_combustivel.exe ^
        src\main.c src\csv.c src\tabela_hash.c src\busca.c ^
        src\busca_sequencial.c src\busca_interpolacao.c src\servidor.c ^
        -lws2_32
    goto :fim
)

REM Tenta WinLibs na localizacao padrao
set WINLIBS="%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe"
if exist %WINLIBS% (
    echo Usando: WinLibs gcc
    %WINLIBS% -Wall -O2 -std=c99 -o busca_combustivel.exe ^
        src\main.c src\csv.c src\tabela_hash.c src\busca.c ^
        src\busca_sequencial.c src\busca_interpolacao.c src\servidor.c ^
        -lws2_32
    goto :fim
)

echo ERRO: gcc nao encontrado.
echo Instale o MinGW/WinLibs: https://winlibs.com/
pause
exit /b 1

:fim
if %ERRORLEVEL% == 0 (
    echo.
    echo [OK] Compilado: busca_combustivel.exe
    echo.
    echo Para executar:  executar.bat
    echo                 ou: busca_combustivel.exe
) else (
    echo [ERRO] Falha na compilacao.
)
pause
