#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include "modelo.hpp"

/**
 * @file mensajes_hilos.hpp
 * @brief Definición del protocolo de mensajería asíncrona entre hilos de trabajo y la interfaz gráfica.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 3 - Paso 1)
 */

// Mensajes privados de la aplicación (WM_APP)
#define WM_APP_SCAN_INICIADO    (WM_APP + 100)
#define WM_APP_SCAN_PROGRESO     (WM_APP + 101)
#define WM_APP_SCAN_COMPLETADO   (WM_APP + 102)
#define WM_APP_SCAN_FALLIDO      (WM_APP + 103)

/**
 * @struct DatosResultadoScan
 * @brief Carga útil transferida de forma segura mediante LPARAM en WM_APP_SCAN_COMPLETADO.
 */
struct DatosResultadoScan {
    std::vector<MC::Proyecto> inventario;  ///< Inventario total de proyectos encontrados
    long long duracionMs = 0;             ///< Tiempo transcurrido de escaneo en milisegundos
};
