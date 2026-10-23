#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

/**
 * @file portapapeles.hpp
 * @brief Funciones seguras para interacción con el portapapeles nativo de Windows (Win32).
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 2 - Paso 3)
 */

namespace PortapapelesMC {

/**
 * @brief Copia una cadena Unicode (UTF-16) al portapapeles nativo de Windows.
 * Gestiona reintentos breves si el portapapeles está bloqueado por otra aplicación,
 * reserva memoria global móvil GMEM_MOVEABLE y asigna el formato CF_UNICODETEXT.
 * 
 * @param hwndOwner Handle de la ventana propietaria.
 * @param texto Cadena de texto a copiar.
 * @return true si el texto fue transferido con éxito, false en caso de error.
 */
bool copiarTexto(HWND hwndOwner, const std::wstring& texto);

} // namespace PortapapelesMC
