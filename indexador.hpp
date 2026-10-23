#pragma once

#include <vector>
#include <string>
#include <atomic>
#include "modelo.hpp"
#include "mapeo_rutas.hpp"
#include "mensajes_hilos.hpp"

/**
 * @file indexador.hpp
 * @brief Motor de indexación superficial en memoria RAM sobre recurso SMB y lazy loading de entregables.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 3 - Paso 3)
 */

namespace MC {

/// @brief Líneas de negocio corporativas oficiales reconocidas por el sistema
inline const std::vector<std::wstring> LINEAS_NEGOCIO_OFICIALES = {
    L"01_OBRAS_E_INSTALACIONES_(CC_4.XX_Y_4XXX)",
    L"02_MANTENIMIENTOS_Y_SERVICIOS_(CC_2.XX_Y_2XXX)",
    L"03_CONTRATOS_MARCO_RECURRENTES_(CC_1.XX_Y_11XX)",
    L"04_LICITACIONES_(CC_3.XX)",
    L"05_PROPUESTAS_Y_COTIZACIONES",
    L"06_VENTA_MATERIALES_(CC_9.XX)"
};

/**
 * @brief Realiza un escaneo jerárquico superficial de exactamente 3 niveles sobre el repositorio SMB
 * con soporte de checkpoints atómicos de cancelación cooperativa y reporte asíncrono de progreso.
 * 
 * @param rutaBaseUNC Ruta de acceso raíz UNC en red Windows.
 * @param hwndDestino Handle de la ventana Win32 para notificaciones WM_APP_* (opcional).
 * @param tokenCancelacion Referencia a variable atómica para abortar la operación inmediatamente.
 * @param outDuracionMs Referencia donde se asigna el tiempo total del recorrido en milisegundos (-1 si falló).
 * @param baseLinux Ruta base equivalente en Linux (por defecto BASE_LINUX_DEFAULT).
 * @return std::vector<Proyecto> Lista plana de todos los proyectos indexados en memoria RAM.
 */
std::vector<Proyecto> indexarRepositorioConProgreso(
    const std::wstring& rutaBaseUNC,
    HWND hwndDestino,
    std::atomic<bool>& tokenCancelacion,
    long long& outDuracionMs,
    const std::wstring& baseLinux = BASE_LINUX_DEFAULT
);

/**
 * @brief Sobrecarga de compatibilidad para invocaciones síncronas o directas en consola/CLI.
 */
inline std::vector<Proyecto> indexarRepositorio(
    const std::wstring& rutaBaseUNC = BASE_UNC_DEFAULT,
    const std::wstring& baseLinux = BASE_LINUX_DEFAULT) {
    std::atomic<bool> tokenCancel{ false };
    long long duracion = 0;
    return indexarRepositorioConProgreso(rutaBaseUNC, nullptr, tokenCancel, duracion, baseLinux);
}

/**
 * @brief Inspecciona bajo demanda (Lazy Evaluation) los archivos entregables de un proyecto específico.
 * Se invoca únicamente cuando el usuario solicita ver los detalles de un proyecto.
 * 
 * @param rutaProyectoUNC Ruta absoluta de la carpeta del proyecto.
 * @return std::vector<ArchivoEntregable> Lista de archivos directos detectados con sus metadatos.
 */
std::vector<ArchivoEntregable> listarEntregables(
    const std::wstring& rutaProyectoUNC
);

} // namespace MC

namespace IndexadorMC = MC;
