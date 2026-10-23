#pragma once

#include <windows.h>
#include <string>

/**
 * @file diagnostico_red.hpp
 * @brief Módulo de diagnóstico de red SMB y sondeo ligero (fast probe) con Win32.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 3 - Paso 4)
 */

namespace RedMC {

/**
 * @struct EstadoSonda
 * @brief Resultado detallado del sondeo rápido de conectividad de red SMB.
 */
struct EstadoSonda {
    bool accesible = false;              ///< Indica si la ruta UNC está accesible y es un directorio
    DWORD codigoErrorWin32 = 0;          ///< Código de error devuelto por GetLastError()
    std::wstring mensajeDiagnostico;     ///< Explicación técnica del estado detectado
    std::wstring accionSugerida;         ///< Instrucción recomendada para el usuario o soporte
};

/**
 * @brief Ejecuta un sondeo ultrarrápido (< 200 ms) sobre el recurso UNC usando GetFileAttributesW.
 * Intercepta caídas de red, servidor fuera de línea o credenciales expiradas antes de iterar con std::filesystem.
 * 
 * @param rutaUNC Ruta base UNC a verificar (ej. \\192.168.1.242\mcontrol\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS).
 * @return EstadoSonda Estructura con el diagnóstico y la acción recomendada.
 */
EstadoSonda verificarAccesoRecurso(const std::wstring& rutaUNC);

} // namespace RedMC
