@echo off
setlocal EnableExtensions EnableDelayedExpansion

:: ==============================================================================
:: INSTALADOR DESATENDIDO CORPORATIVO (POR USUARIO)
:: Proyecto: Master Control — Directorio de Proyectos GLP
:: Destino: %LOCALAPPDATA%\MasterControl\DirectorioProyectos\
:: Requiere UAC / Admin: NO (Instalacion nativa en espacio de usuario)
:: ==============================================================================

title Master Control - Instalador Corporativo

echo ============================================================================
echo   MASTER CONTROL [GLP] - INSTALADOR DESATENDIDO DE PUESTO DE TRABAJO
echo ============================================================================
echo.

:: 1. LOCALIZAR EL ARCHIVO FUENTE DIRECTORIO_MC.EXE
set "SRC_EXE="
if exist "bin\DirectorioMC.exe" set "SRC_EXE=bin\DirectorioMC.exe"
if not defined SRC_EXE (
    if exist "DirectorioMC.exe" set "SRC_EXE=DirectorioMC.exe"
)

if not defined SRC_EXE (
    echo [ERROR CRITICO] No se encontro el archivo ejecutable 'DirectorioMC.exe'.
    echo Asegurese de compilar primero la aplicacion ejecutando 'build_msvc.bat'
    echo o 'build_mingw.bat' antes de lanzar este instalador.
    echo.
    pause
    exit /b 1
)

echo [INFO] Archivo fuente localizado: "%SRC_EXE%"

:: 2. DETENER INSTANCIAS PREVIAS EN EJECUCIÓN
tasklist /FI "IMAGENAME eq DirectorioMC.exe" 2>nul | find /I /N "DirectorioMC.exe" >nul
if %ERRORLEVEL% equ 0 (
    echo [INFO] Cerrando instancia anterior de DirectorioMC.exe...
    taskkill /F /IM DirectorioMC.exe >nul 2>&1
    ping -n 2 127.0.0.1 >nul 2>&1
)

:: 3. PREPARAR CARPETA DE DESTINO EN %LOCALAPPDATA%
set "DEST_DIR=%LOCALAPPDATA%\MasterControl\DirectorioProyectos"
echo [INFO] Carpeta de instalacion: "%DEST_DIR%"

if not exist "%DEST_DIR%" (
    mkdir "%DEST_DIR%" >nul 2>&1
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] No se pudo crear la carpeta de destino en %LOCALAPPDATA%.
        exit /b 1
    )
)

:: 4. COPIAR BINARIO Y RECURSOS ASOCIADOS
echo [1/3] Copiando binario optimizado...
copy /Y "%SRC_EXE%" "%DEST_DIR%\DirectorioMC.exe" >nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Fallo al copiar el archivo ejecutable al directorio destino.
    exit /b 1
)

if exist "app.ico" (
    copy /Y "app.ico" "%DEST_DIR%\app.ico" >nul 2>&1
)

:: 5. CREAR ACCESO DIRECTO EN EL ESCRITORIO DEL USUARIO
echo [2/3] Generando acceso directo en el Escritorio...
set "SHORTCUT_PATH=%USERPROFILE%\Desktop\Directorio de Proyectos Master Control.lnk"

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$ws = New-Object -ComObject WScript.Shell;" ^
    "$s = $ws.CreateShortcut('%SHORTCUT_PATH%');" ^
    "$s.TargetPath = '%DEST_DIR%\DirectorioMC.exe';" ^
    "$s.WorkingDirectory = '%DEST_DIR%';" ^
    "$s.Description = 'Sistema Centralizado de Directorio y Gobernanza de Proyectos GLP - Master Control';" ^
    "$s.IconLocation = '%DEST_DIR%\DirectorioMC.exe,0';" ^
    "$s.Save();"

if exist "%SHORTCUT_PATH%" (
    echo [OK] Acceso directo creado: "%SHORTCUT_PATH%"
) else (
    echo [AVISO] No se pudo generar el acceso directo en el Escritorio.
)

:: 6. SONDEO Y VERIFICACIÓN DE CONECTIVIDAD AL SERVIDOR SMB
echo [3/3] Verificando conectividad con servidor Samba [\\192.168.1.242\mcontrol]...

if exist "\\192.168.1.242\mcontrol" (
    echo.
    echo  ========================================================================
    echo   [CONEXION EXITOSA] Recurso SMB alcanzable y autenticado.
    echo  ========================================================================
) else (
    echo.
    echo  ------------------------------------------------------------------------
    echo   [ADVERTENCIA DE RED] No se detecto acceso directo a \\192.168.1.242\mcontrol
    echo   Si este equipo requiere registrar credenciales corporativas en Windows,
    echo   ejecute el siguiente comando en la consola:
    echo.
    echo     cmdkey /add:192.168.1.242 /user:mcontrol /pass:mcontrol
    echo  ------------------------------------------------------------------------
)

echo.
echo ============================================================================
echo   INSTALACION COMPLETADA SATISFACTORIAMENTE
echo ============================================================================
echo  Ubicacion: "%DEST_DIR%\DirectorioMC.exe"
echo  El aplicativo ya se encuentra listo para usar desde el acceso directo.
echo.
exit /b 0
