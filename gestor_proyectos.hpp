#pragma once

#include <string>

/**
 * @file gestor_proyectos.hpp
 * @brief Módulo de creación y gestión física de proyectos en el servidor de archivos (ClearOS 7 / SMB).
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 1 - Paso 4)
 */

namespace GestorMC {

/**
 * @brief Crea físicamente el contenedor raíz de un proyecto en el servidor de forma estrictamente plana.
 * Queda prohibido generar subcarpetas internas (ej. 01_Ingenieria, 02_Comercial).
 * 
 * @param rutaBaseUNC Ruta raíz UNC del repositorio SMB.
 * @param categoria Línea de negocio oficial (Nivel 1).
 * @param subdivision Año o cliente marco (Nivel 2).
 * @param nombreProyectoNormalizado Nombre sanitizado del proyecto (Nivel 3, <= 100 caracteres).
 * @param outRutaCreadaUNC Parámetro de salida que recibe la ruta UNC absoluta del proyecto creado.
 * @param outError Parámetro de salida que recibe la descripción del error si la operación falla.
 * @return true si la carpeta se creó físicamente con éxito, false si hubo error o colisión previa.
 */
bool crearProyectoPlano(
    const std::wstring& rutaBaseUNC,
    const std::wstring& categoria,
    const std::wstring& subdivision,
    const std::wstring& nombreProyectoNormalizado,
    std::wstring& outRutaCreadaUNC,
    std::wstring& outError
);

} // namespace GestorMC
