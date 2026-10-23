#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

/**
 * @file acciones_shell.hpp
 * @brief Invocación nativa del Shell de Windows para apertura de carpetas y archivos.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 2 - Paso 3)
 */

namespace ShellMC {

/**
 * @brief Abre la carpeta indicada en el Explorador de Windows (explorer.exe).
 * Valida la existencia previa de la ruta para advertir al usuario en caso de desconexión de red SMB.
 * 
 * @param hwndParent Handle de la ventana padre para cuadros de diálogo.
 * @param rutaUNC Ruta UNC o local de la carpeta del proyecto.
 * @return true si el Shell inició el explorador correctamente, false si falló.
 */
bool abrirCarpetaEnExplorador(HWND hwndParent, const std::wstring& rutaUNC);

/**
 * @brief Abre el archivo técnico seleccionado utilizando el programa predeterminado del sistema
 * (ej. AutoCAD para .dwg, Acrobat Reader para .pdf, Excel para .xlsx).
 * 
 * @param hwndParent Handle de la ventana padre.
 * @param rutaArchivoCompleta Ruta física absoluta del archivo a ejecutar.
 * @return true si el Shell despachó la ejecución, false en caso de error.
 */
bool abrirArchivoConAsociado(HWND hwndParent, const std::wstring& rutaArchivoCompleta);

} // namespace ShellMC
