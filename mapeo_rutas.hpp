#pragma once

#include <string>
#include <string_view>

/**
 * @file mapeo_rutas.hpp
 * @brief Funciones de conversión bidireccional y normalización de rutas UNC (Windows) y POSIX (Linux).
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 1 - Paso 1)
 */

namespace MC {

/// @brief Ruta raíz corporativa predeterminada para acceso en red local Windows (SMB / UNC)
inline constexpr const wchar_t* BASE_UNC_DEFAULT =
    L"\\\\192.168.1.242\\mcontrol\\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS";

/// @brief Ruta física corporativa predeterminada en el sistema de archivos del servidor ClearOS 7 (POSIX)
inline constexpr const wchar_t* BASE_LINUX_DEFAULT =
    L"/home/mcontrol/RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS";

/**
 * @brief Normaliza los separadores de ruta y colapsa barras repetidas consecutivas.
 * Preserva de forma estricta el prefijo de doble barra inicial de red (\\\\) en Windows
 * y el prefijo de barra simple inicial (/) en rutas absolutas Linux.
 * 
 * @param ruta Cadena con la ruta de entrada a normalizar.
 * @param formatoWindows true para usar contrabarras '\\', false para barras '/'.
 * @return std::wstring Ruta normalizada y limpia de separadores redundantes.
 */
std::wstring normalizar_separadores(const std::wstring& ruta, bool formatoWindows);

/**
 * @brief Traduce una ruta de red Windows UNC a su ruta física equivalente en el servidor Linux.
 * 
 * @param rutaUNC Ruta absoluta de red Windows UNC (ej. \\\\192.168.1.242\\mcontrol\\...).
 * @param baseUNC Prefijo raíz esperado en Windows (por defecto BASE_UNC_DEFAULT).
 * @param baseLinux Prefijo raíz equivalente en Linux (por defecto BASE_LINUX_DEFAULT).
 * @return std::wstring Ruta Linux equivalente o cadena vacía L"" si la ruta no pertenece al repositorio base.
 */
std::wstring unc_a_linux(const std::wstring& rutaUNC,
                         const std::wstring& baseUNC = BASE_UNC_DEFAULT,
                         const std::wstring& baseLinux = BASE_LINUX_DEFAULT);

/**
 * @brief Traduce una ruta física Linux a su ruta de red Windows UNC equivalente.
 * 
 * @param rutaLinux Ruta absoluta en el servidor Linux (ej. /home/mcontrol/...).
 * @param baseLinux Prefijo raíz esperado en Linux (por defecto BASE_LINUX_DEFAULT).
 * @param baseUNC Prefijo raíz equivalente en Windows (por defecto BASE_UNC_DEFAULT).
 * @return std::wstring Ruta Windows UNC equivalente o cadena vacía L"" si la ruta no pertenece al repositorio base.
 */
std::wstring linux_a_unc(const std::wstring& rutaLinux,
                         const std::wstring& baseLinux = BASE_LINUX_DEFAULT,
                         const std::wstring& baseUNC = BASE_UNC_DEFAULT);

} // namespace MC
