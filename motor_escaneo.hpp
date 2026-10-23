#pragma once

#include <vector>
#include <string>
#include "modelo_proyecto.hpp"
#include "modelo_archivo.hpp"
#include "mapeo_rutas.hpp"

/**
 * @file motor_escaneo.hpp
 * @brief Motor de exploración superficial e indexación en memoria RAM sobre recurso SMB.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 1 - Paso 3)
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
 * @brief Realiza un escaneo jerárquico superficial de exactamente 3 niveles sobre el repositorio.
 * Nivel 1: Categorías (Líneas de Negocio)
 * Nivel 2: Subdivisiones (Años o Contratos Marco)
 * Nivel 3: Carpetas de Proyecto (Punto de anclaje)
 * 
 * Se detiene estrictamente en Nivel 3 sin penetrar en archivos hoja para proteger el enlace de red SMB.
 * 
 * @param rutaBaseUNC Ruta de acceso raíz UNC en red Windows (por defecto BASE_UNC_DEFAULT).
 * @param baseLinux Ruta base equivalente en Linux (por defecto BASE_LINUX_DEFAULT).
 * @return std::vector<Proyecto> Lista plana de todos los proyectos indexados en memoria.
 */
std::vector<Proyecto> escanearRepositorio(
    const std::wstring& rutaBaseUNC = BASE_UNC_DEFAULT,
    const std::wstring& baseLinux = BASE_LINUX_DEFAULT
);

/**
 * @brief Inspecciona bajo demanda (Lazy Evaluation) los archivos entregables de un proyecto específico.
 * Se invoca únicamente cuando el usuario hace clic o selecciona un proyecto en la interfaz.
 * 
 * @param rutaProyectoUNC Ruta absoluta de la carpeta del proyecto.
 * @return std::vector<ArchivoEntregable> Lista de archivos directos detectados con sus metadatos.
 */
std::vector<ArchivoEntregable> listarArchivosProyecto(
    const std::wstring& rutaProyectoUNC
);

} // namespace MC
