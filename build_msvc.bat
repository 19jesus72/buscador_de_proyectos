@echo off
setlocal EnableExtensions EnableDelayedExpansion

:: ==============================================================================
:: SCRIPT DE COMPILACIÓN AUTOMATIZADA: MICROSOFT VISUAL C++ (MSVC)
:: Proyecto: Master Control — Directorio de Proyectos GLP
:: Modo: Release Monolítico Estático (/MT /O2 /GL /LTCG)
:: Binario de salida: bin\DirectorioMC.exe (< 1 MB, 0 dependencias externas)
:: ==============================================================================

title Master Control - Compilador MSVC

echo ============================================================================
echo   MASTER CONTROL [GLP] - PIPELINE DE COMPILACION NATIVA MSVC [C++17]
echo ============================================================================
echo.

:: ------------------------------------------------------------------------------
:: 0. PROCESAR PARÁMETRO DE LIMPIEZA ("clean")
:: ------------------------------------------------------------------------------
if /i "%~1"=="clean" goto ACCION_CLEAN
if /i "%~1"=="/clean" goto ACCION_CLEAN
if /i "%~1"=="-clean" goto ACCION_CLEAN

goto VERIFICAR_ENTORNO

:ACCION_CLEAN
echo [LIMPIEZA] Purgando carpetas de salida 'build\' y 'bin\'...
if exist build rmdir /s /q build
if exist bin rmdir /s /q bin
echo [LIMPIEZA] Proyecto limpio con exito.
exit /b 0

:: ------------------------------------------------------------------------------
:: 1. VERIFICAR O LOCALIZAR EL ENTORNO DE VISUAL STUDIO (vcvars64.bat)
:: ------------------------------------------------------------------------------
:VERIFICAR_ENTORNO
where cl.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [INFO] Herramienta 'cl.exe' detectada en el PATH del sistema.
    goto PREPARAR_DIRECTORIOS
)

echo [INFO] 'cl.exe' no esta en el PATH. Intentando autodetectar Visual Studio...

:: 1.1 Búsqueda en rutas directas estándar (VS 2022 y VS 2019)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    echo [INFO] Inicializando entorno desde VS 2022 Community...
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
    goto VERIFICAR_CL_POST_INIT
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    echo [INFO] Inicializando entorno desde VS 2022 Professional...
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul
    goto VERIFICAR_CL_POST_INIT
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    echo [INFO] Inicializando entorno desde VS 2022 Enterprise...
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul
    goto VERIFICAR_CL_POST_INIT
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    echo [INFO] Inicializando entorno desde VS 2022 BuildTools...
    call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
    goto VERIFICAR_CL_POST_INIT
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    echo [INFO] Inicializando entorno desde VS 2019 Community...
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
    goto VERIFICAR_CL_POST_INIT
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    echo [INFO] Inicializando entorno desde VS 2019 Professional...
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul
    goto VERIFICAR_CL_POST_INIT
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    echo [INFO] Inicializando entorno desde VS 2019 Enterprise...
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul
    goto VERIFICAR_CL_POST_INIT
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    echo [INFO] Inicializando entorno desde VS 2019 BuildTools...
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
    goto VERIFICAR_CL_POST_INIT
)

:: 1.2 Búsqueda con vswhere
set "VS_WHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VS_WHERE%" (
    for /f "usebackq delims=" %%i in (`"%VS_WHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do (
        set "VS_PATH=%%i"
    )
    if defined VS_PATH (
        if exist "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
            echo [INFO] Localizado Visual Studio en: "!VS_PATH!"
            call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" >nul
            goto VERIFICAR_CL_POST_INIT
        )
    )
)

:VERIFICAR_CL_POST_INIT
where cl.exe >nul 2>&1
if %ERRORLEVEL% equ 0 goto PREPARAR_DIRECTORIOS

echo.
echo [ERROR CRITICO] No se pudo encontrar el compilador Microsoft C/C++ [cl.exe].
echo Asegurese de ejecutar este script desde 'Developer Command Prompt for VS 2022'
echo o verificar la instalacion de la carga de trabajo 'Desarrollo para el escritorio con C++'.
echo.
exit /b 1

:: ------------------------------------------------------------------------------
:: 2. PREPARACIÓN DE DIRECTORIOS DE TRABAJO
:: ------------------------------------------------------------------------------
:PREPARAR_DIRECTORIOS
if not exist build mkdir build
if not exist bin mkdir bin

:: ------------------------------------------------------------------------------
:: 3. PASO A: COMPILACIÓN DEL SCRIPT DE RECURSOS (recursos.rc -> build\recursos.res)
:: ------------------------------------------------------------------------------
echo [1/2] Compilando recursos e iconos Win32 (recursos.rc -> build\recursos.res)...
rc.exe /nologo /fo "build\recursos.res" recursos.rc
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Fallo la compilacion de recursos [rc.exe devolvio error %ERRORLEVEL%].
    exit /b %ERRORLEVEL%
)

:: ------------------------------------------------------------------------------
:: 4. PASO B: COMPILACIÓN Y ENLACE MONOLÍTICO OPTIMIZADO (cl.exe)
:: ------------------------------------------------------------------------------
echo [2/2] Compilando fuentes C++17 y generando binario optimizado [LTCG /MT /O2]...

cl.exe /nologo /std:c++17 /O2 /GL /MT /DNDEBUG /DUNICODE /D_UNICODE /EHsc /utf-8 /W4 ^
    /Fo:build\ /Fe:bin\DirectorioMC.exe ^
    main_gui.cpp ^
    ventana_principal.cpp ^
    controlador_directorio.cpp ^
    modal_nuevo_proyecto.cpp ^
    diagnostico_red.cpp ^
    acciones_shell.cpp ^
    portapapeles.cpp ^
    gestor_proyectos.cpp ^
    sanitizador.cpp ^
    indexador.cpp ^
    mapeo_rutas.cpp ^
    build\recursos.res ^
    /link /SUBSYSTEM:WINDOWS /LTCG /OPT:REF /OPT:ICF /MANIFEST:NO ^
    comctl32.lib shell32.lib user32.lib gdi32.lib advapi32.lib

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR CRITICO] Fallo la compilacion o enlace del ejecutable [cl.exe devolvio error %ERRORLEVEL%].
    exit /b %ERRORLEVEL%
)

:: ------------------------------------------------------------------------------
:: 5. VERIFICACIÓN POST-BUILD Y REPORTE DE MÉTRICAS
:: ------------------------------------------------------------------------------
echo.
echo ============================================================================
echo   COMPILACION MSVC COMPLETADA CON EXITO
echo ============================================================================

if not exist bin\DirectorioMC.exe (
    echo [ERROR] No se detecto el archivo bin\DirectorioMC.exe al finalizar.
    exit /b 1
)

for %%F in (bin\DirectorioMC.exe) do (
    set "BYTES=%%~zF"
    set /a "KB=BYTES / 1024"
)

echo  Ejecutable final : bin\DirectorioMC.exe
echo  Tamano generado  : !KB! KB [!BYTES! bytes]
echo  Arquitectura     : x64 Nativa Monolitica [Sin DLLs ni runtimes externos]
echo  Subsistema       : Windows GUI [Sin consola negra]
echo.
if !BYTES! lss 1048576 (
    echo  [CONFIRMACION] El binario cumple la directiva de tamano: INFERIOR A 1 MB.
) else (
    echo  [ADVERTENCIA] El binario supero 1 MB [!KB! KB]. Verifique banderas de enlace.
)

echo ============================================================================
exit /b 0
