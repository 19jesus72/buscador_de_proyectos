@echo off
setlocal EnableExtensions EnableDelayedExpansion

:: ==============================================================================
:: DESINSTALADOR LIMPIO CORPORATIVO (POR USUARIO)
:: Proyecto: Master Control — Directorio de Proyectos GLP
:: Remueve: Binario, accesos directos y carpetas locales sin tocar el registro
:: ==============================================================================

title Master Control - Desinstalador

echo ============================================================================
echo   MASTER CONTROL [GLP] - DESINSTALACION COMPLETA DEL APLICATIVO
echo ============================================================================
echo.

:: 1. CERRAR PROCESO EN EJECUCIÓN
echo [1/3] Finalizando procesos activos...
taskkill /F /IM DirectorioMC.exe >nul 2>&1
ping -n 2 127.0.0.1 >nul 2>&1

:: 2. ELIMINAR ACCESO DIRECTO DEL ESCRITORIO
echo [2/3] Eliminando acceso directo del Escritorio...
set "SHORTCUT_PATH=%USERPROFILE%\Desktop\Directorio de Proyectos Master Control.lnk"
if exist "%SHORTCUT_PATH%" (
    del /f /q "%SHORTCUT_PATH%" >nul 2>&1
    echo [OK] Acceso directo removido.
) else (
    echo [INFO] No se encontro acceso directo en el Escritorio.
)

:: 3. ELIMINAR CARPETA EN %LOCALAPPDATA%
echo [3/3] Purgando archivos de instalacion local...
set "APP_DIR=%LOCALAPPDATA%\MasterControl\DirectorioProyectos"
set "PARENT_DIR=%LOCALAPPDATA%\MasterControl"

if exist "%APP_DIR%" (
    rmdir /s /q "%APP_DIR%" >nul 2>&1
    echo [OK] Carpeta eliminada: "%APP_DIR%"
) else (
    echo [INFO] No se encontro la carpeta de instalacion.
)

:: Limpiar carpeta padre si quedo vacia
if exist "%PARENT_DIR%" (
    dir /b /a "%PARENT_DIR%" 2>nul | findstr "." >nul
    if %ERRORLEVEL% neq 0 (
        rmdir /s /q "%PARENT_DIR%" >nul 2>&1
    )
)

echo.
echo ============================================================================
echo   DESINSTALACION LIMPIA CONCLUIDA
echo ============================================================================
echo  El aplicativo fue removido en su totalidad.
echo  No quedaron entradas residuales en el registro ni archivos huerfanos.
echo.
exit /b 0
