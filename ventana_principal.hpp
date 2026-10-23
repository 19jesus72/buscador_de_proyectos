#pragma once

#include "recursos_gui.hpp"
#include "mapeo_rutas.hpp"
#include <string>

/**
 * @file ventana_principal.hpp
 * @brief Declaración de la clase y manejadores de la ventana principal Win32.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 2 - Paso 2)
 */

namespace GUI {

/// @brief Nombre de la clase de ventana registrada en el sistema
inline constexpr const wchar_t* NOMBRE_CLASE_VENTANA = L"MasterControl_Directorio";

/// @brief Título principal que se visualiza en la barra de título
inline constexpr const wchar_t* TITULO_VENTANA =
    L"Master Control — Directorio Interno, Búsqueda y Gobernanza de Proyectos";

/**
 * @brief Registra la clase de ventana en el sistema operativo Windows.
 * @param hInstance Instancia de la aplicación.
 * @return true si el registro fue exitoso o ya existía, false si falló.
 */
bool registrarClaseVentana(HINSTANCE hInstance);

/**
 * @brief Crea e inicializa la ventana principal centrada en pantalla con sus dimensiones predeterminadas.
 * @param hInstance Instancia de la aplicación.
 * @param nCmdShow Modo de visualización inicial (ej. SW_SHOW).
 * @param rutaBaseUNC Ruta base de red Windows SMB a inspeccionar.
 * @return HWND Handle de la ventana principal creada, o nullptr en caso de error.
 */
HWND crearVentanaPrincipal(
    HINSTANCE hInstance,
    int nCmdShow,
    const std::wstring& rutaBaseUNC = MC::BASE_UNC_DEFAULT
);

/**
 * @brief Procedimiento de ventana (Window Procedure) que despacha los mensajes de Win32.
 */
LRESULT CALLBACK WndProcPrincipal(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

} // namespace GUI
