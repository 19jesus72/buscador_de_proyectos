@echo off
setlocal EnableExtensions EnableDelayedExpansion

:: ==============================================================================
:: AUDITORÍA DE DEPENDENCIAS Y TABLA DE IMPORTACIONES (IAT)
:: Proyecto: Master Control — Directorio de Proyectos GLP
:: Criterio: Solo DLLs nativas del sistema operativo Windows
:: Prohibido: VCRUNTIME140.dll, MSVCP140.dll, libgcc*, libstdc++*, libwinpthread*
:: ==============================================================================

title Master Control - Auditoria de Dependencias IAT

echo ============================================================================
echo   MASTER CONTROL [GLP] - AUDITORIA DE DEPENDENCIAS DEL BINARIO (PE / IAT)
echo ============================================================================
echo.

:: 1. LOCALIZAR EL EJECUTABLE OBJETIVO
set "TARGET_EXE=%~1"
if "%TARGET_EXE%"=="" set "TARGET_EXE=bin\DirectorioMC.exe"

if not exist "%TARGET_EXE%" (
    if exist "DirectorioMC.exe" (
        set "TARGET_EXE=DirectorioMC.exe"
    ) else (
        echo [ERROR] No se encontro el archivo binario: "%TARGET_EXE%"
        echo Ejecute previamente 'build_msvc.bat' o 'build_mingw.bat' para compilarlo.
        echo.
        exit /b 1
    )
)

echo [AUDITORIA] Inspeccionando cabeceras de importacion en: "%TARGET_EXE%"
echo.

:: 2. EJECUTAR ANÁLISIS DE LA TABLA IAT CON POWERSHELL NATIVO
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$target = '%TARGET_EXE%';" ^
    "$whitelist = @('KERNEL32.DLL','USER32.DLL','GDI32.DLL','COMCTL32.DLL','SHELL32.DLL','ADVAPI32.DLL','NTDLL.DLL','SHLWAPI.DLL');" ^
    "$forbidden = @('VCRUNTIME140.DLL','MSVCP140.DLL','VCRUNTIME140_1.DLL','LIBGCC_S_SEH-1.DLL','LIBSTDC++-6.DLL','LIBWINPTHREAD-1.DLL');" ^
    "try {" ^
    "    $bytes = [System.IO.File]::ReadAllBytes((Resolve-Path $target));" ^
    "    if ($bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) { Write-Host '[ERROR] No es un archivo PE valido.' -ForegroundColor Red; exit 1 }" ^
    "    $e_lfanew = [BitConverter]::ToInt32($bytes, 0x3C);" ^
    "    if ($bytes[$e_lfanew] -ne 0x50 -or $bytes[$e_lfanew+1] -ne 0x45) { Write-Host '[ERROR] Firma PE no valida.' -ForegroundColor Red; exit 1 }" ^
    "    $magic = [BitConverter]::ToUInt16($bytes, $e_lfanew + 24);" ^
    "    $is64 = ($magic -eq 0x20B);" ^
    "    $numSections = [BitConverter]::ToUInt16($bytes, $e_lfanew + 6);" ^
    "    $optHeaderSize = [BitConverter]::ToUInt16($bytes, $e_lfanew + 20);" ^
    "    $dataDirRel = if ($is64) { 120 } else { 104 };" ^
    "    $importDirOffset = $e_lfanew + 24 + $dataDirRel;" ^
    "    $importRva = [BitConverter]::ToUInt32($bytes, $importDirOffset);" ^
    "    if ($importRva -eq 0) { Write-Host '[AVISO] No se detecto seccion de importaciones (IAT vacia).' -ForegroundColor Yellow; exit 0 }" ^
    "    $secHeaderOffset = $e_lfanew + 24 + $optHeaderSize;" ^
    "    $sections = @();" ^
    "    for ($i = 0; $i -lt $numSections; $i++) {" ^
    "        $offset = $secHeaderOffset + ($i * 40);" ^
    "        $vSize = [BitConverter]::ToUInt32($bytes, $offset + 8);" ^
    "        $vAddr = [BitConverter]::ToUInt32($bytes, $offset + 12);" ^
    "        $rSize = [BitConverter]::ToUInt32($bytes, $offset + 16);" ^
    "        $rOffset = [BitConverter]::ToUInt32($bytes, $offset + 20);" ^
    "        $sections += [PSCustomObject]@{ VirtualAddress = $vAddr; VirtualSize = $vSize; RawOffset = $rOffset; RawSize = $rSize };" ^
    "    }" ^
    "    function RvaToOffset([uint32]$rva) {" ^
    "        foreach ($sec in $sections) {" ^
    "            if ($rva -ge $sec.VirtualAddress -and $rva -lt ($sec.VirtualAddress + $sec.VirtualSize)) {" ^
    "                return $sec.RawOffset + ($rva - $sec.VirtualAddress);" ^
    "            }" ^
    "        }" ^
    "        return 0;" ^
    "    }" ^
    "    $descOffset = RvaToOffset $importRva;" ^
    "    $dlls = @();" ^
    "    while ($true) {" ^
    "        $nameRva = [BitConverter]::ToUInt32($bytes, $descOffset + 12);" ^
    "        $firstThunk = [BitConverter]::ToUInt32($bytes, $descOffset + 16);" ^
    "        if ($nameRva -eq 0 -and $firstThunk -eq 0) { break }" ^
    "        $nameOffset = RvaToOffset $nameRva;" ^
    "        if ($nameOffset -gt 0) {" ^
    "            $len = 0;" ^
    "            while ($bytes[$nameOffset + $len] -ne 0) { $len++ }" ^
    "            $dlls += [System.Text.Encoding]::ASCII.GetString($bytes, $nameOffset, $len);" ^
    "        }" ^
    "        $descOffset += 20;" ^
    "    }" ^
    "    Write-Host '============================================================================';" ^
    "    Write-Host '  MODULOS Y LIBRERIAS DINAMICAS DETECTADAS EN LA TABLA IAT:';" ^
    "    Write-Host '============================================================================';" ^
    "    $tieneFalla = $false;" ^
    "    foreach ($dll in $dlls) {" ^
    "        $upper = $dll.ToUpper();" ^
    "        $esApiSet = $upper.StartsWith('API-MS-WIN-') -or $upper.StartsWith('EXT-MS-WIN-');" ^
    "        if ($whitelist -contains $upper -or $esApiSet) {" ^
    "            Write-Host ('  [OK - NATIVA]      ' + $dll) -ForegroundColor Green;" ^
    "        } elseif ($forbidden -contains $upper) {" ^
    "            Write-Host ('  [PROHIBIDA]        ' + $dll + '  <-- VIOLACION DE REQUISITO ESTÁTICO') -ForegroundColor Red;" ^
    "            $tieneFalla = $true;" ^
    "        } else {" ^
    "            Write-Host ('  [NO AUTORIZADA]    ' + $dll) -ForegroundColor Yellow;" ^
    "            $tieneFalla = $true;" ^
    "        }" ^
    "    }" ^
    "    Write-Host '============================================================================';" ^
    "    if ($tieneFalla) {" ^
    "        Write-Host '[AUDITORIA FALLIDA] El binario depende de bibliotecas externas no autorizadas.' -ForegroundColor Red;" ^
    "        exit 1;" ^
    "    } else {" ^
    "        Write-Host '[CERTIFICADO] El binario es 100% portatil y autonomo.' -ForegroundColor Green;" ^
    "        Write-Host '             Solo depende de subsistemas nativos de Windows 10/11.' -ForegroundColor Green;" ^
    "        exit 0;" ^
    "    }" ^
    "} catch {" ^
    "    Write-Host ('[ERROR DE LECTURA] ' + $_.Exception.Message) -ForegroundColor Red;" ^
    "    exit 1;" ^
    "}"

if %ERRORLEVEL% neq 0 (
    echo.
    echo [FALLO] Auditoria de dependencias rechazada. Codigo de salida: %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
exit /b 0
