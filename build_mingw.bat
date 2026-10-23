@echo off
setlocal EnableExtensions EnableDelayedExpansion

:: ==============================================================================
:: SCRIPT DE COMPILACIÓN AUTOMATIZADA: MINGW-W64 (GCC EN WINDOWS)
:: Proyecto: Master Control — Directorio de Proyectos GLP
:: Modo: Release Monolítico Estático (-O3 -flto -static -s)
:: Binario de salida: bin\DirectorioMC.exe (< 1 MB, 0 dependencias externas)
:: ==============================================================================

title Master Control - Compilador MinGW-w64

echo ============================================================================
echo   MASTER CONTROL [GLP] - PIPELINE DE COMPILACION NATIVA MINGW [GCC C++17]
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
:: 1. VERIFICAR O LOCALIZAR G++ Y WINDRES EN EL SISTEMA
:: ------------------------------------------------------------------------------
:VERIFICAR_ENTORNO
where g++.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    where windres.exe >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        echo [INFO] Herramientas 'g++' y 'windres' detectadas en el PATH del sistema.
        goto PREPARAR_DIRECTORIOS
    )
)

echo [INFO] 'g++' o 'windres' no estan en el PATH. Buscando instalaciones tipicas de MinGW-w64/MSYS2...

if exist "C:\msys64\mingw64\bin\g++.exe" (
    echo [INFO] MinGW-w64 localizado en: C:\msys64\mingw64\bin
    set "PATH=C:\msys64\mingw64\bin;%PATH%"
    goto PREPARAR_DIRECTORIOS
)
if exist "C:\msys64\ucrt64\bin\g++.exe" (
    echo [INFO] MinGW-w64 localizado en: C:\msys64\ucrt64\bin
    set "PATH=C:\msys64\ucrt64\bin;%PATH%"
    goto PREPARAR_DIRECTORIOS
)
if exist "C:\msys64\clang64\bin\g++.exe" (
    echo [INFO] MinGW-w64 localizado en: C:\msys64\clang64\bin
    set "PATH=C:\msys64\clang64\bin;%PATH%"
    goto PREPARAR_DIRECTORIOS
)
if exist "C:\mingw64\bin\g++.exe" (
    echo [INFO] MinGW-w64 localizado en: C:\mingw64\bin
    set "PATH=C:\mingw64\bin;%PATH%"
    goto PREPARAR_DIRECTORIOS
)
if exist "C:\MinGW\bin\g++.exe" (
    echo [INFO] MinGW-w64 localizado en: C:\MinGW\bin
    set "PATH=C:\MinGW\bin;%PATH%"
    goto PREPARAR_DIRECTORIOS
)

echo.
echo [ERROR CRITICO] No se encontraron las herramientas 'g++' o 'windres'.
echo Asegurese de tener instalado MinGW-w64 o MSYS2 y de agregar su directorio 'bin' a la variable PATH.
echo Rutas habituales: C:\msys64\mingw64\bin o C:\mingw64\bin
echo.
exit /b 1

:: ------------------------------------------------------------------------------
:: 2. PREPARACIÓN DE DIRECTORIOS DE TRABAJO
:: ------------------------------------------------------------------------------
:PREPARAR_DIRECTORIOS
if not exist build mkdir build
if not exist bin mkdir bin

:: ------------------------------------------------------------------------------
:: 3. PASO A: COMPILACIÓN DEL SCRIPT DE RECURSOS (windres)
:: ------------------------------------------------------------------------------
echo [1/2] Compilando script de recursos Win32 [recursos.rc -^> build\recursos.res]...
windres recursos.rc -O coff -o build\recursos.res
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Fallo la compilacion de recursos con windres [Error %ERRORLEVEL%].
    exit /b %ERRORLEVEL%
)

:: ------------------------------------------------------------------------------
:: 4. PASO B: COMPILACIÓN Y ENLACE MONOLÍTICO OPTIMIZADO (g++)
:: ------------------------------------------------------------------------------
echo [2/2] Compilando fuentes C++17 y enlazando binario [-O3 -flto -static -s]...

g++ -std=c++17 -O3 -flto -mwindows ^
    -static-libgcc -static-libstdc++ -static -s ^
    -Wall -Wextra ^
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
    -o bin\DirectorioMC.exe ^
    -lcomctl32 -lshell32 -luser32 -lgdi32 -ladvapi32

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR CRITICO] Fallo la compilacion con g++ [Error %ERRORLEVEL%].
    exit /b %ERRORLEVEL%
)

:: ------------------------------------------------------------------------------
:: 5. VERIFICACIÓN POST-BUILD Y REPORTE DE MÉTRICAS
:: ------------------------------------------------------------------------------
echo.
echo ============================================================================
echo   COMPILACION MINGW-W64 COMPLETADA CON EXITO
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
echo  Optimizacion LTO : Activa [-flto, funciones inline inter-modulares]
echo  Tabla de Simbolos: Purgada [-s / Strip completo]
echo  Enlace Estatico  : Completo [Sin libstdc++-6.dll ni libgcc_s_seh-1.dll]
echo  Subsistema       : Windows GUI [-mwindows, sin consola negra]
echo.
if !BYTES! lss 1048576 (
    echo  [CONFIRMACION] El binario cumple la directiva de tamano: INFERIOR A 1 MB.
) else (
    echo  [ADVERTENCIA] El binario supero 1 MB [!KB! KB]. Verifique banderas de enlace.
)

echo ============================================================================
exit /b 0
