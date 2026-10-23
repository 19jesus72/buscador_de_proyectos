#pragma once

#include <string>
#include <utility>
#include <cstdint>

/**
 * @file modelo_archivo.hpp
 * @brief Estructura de datos para los entregables técnicos de un proyecto.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 1 - Paso 3)
 */

namespace MC {

/**
 * @struct ArchivoEntregable
 * @brief Representa un archivo o plano técnico detectado bajo demanda dentro de una carpeta de proyecto.
 */
struct ArchivoEntregable {
    std::wstring nombre;        ///< Nombre del archivo (ej. 6_Planos_Red_GLP.dwg)
    std::wstring extension;     ///< Extensión del archivo en minúsculas (ej. .dwg, .pdf)
    uintmax_t tamanioBytes{0};  ///< Tamaño en bytes del archivo
    double tamanioMB{0.0};      ///< Tamaño formateado en Megabytes (MB)

    ArchivoEntregable() = default;

    ArchivoEntregable(std::wstring nom, std::wstring ext, uintmax_t bytes)
        : nombre(std::move(nom)),
          extension(std::move(ext)),
          tamanioBytes(bytes),
          tamanioMB(static_cast<double>(bytes) / (1024.0 * 1024.0)) {}
};

} // namespace MC
