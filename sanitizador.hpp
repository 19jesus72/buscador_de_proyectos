#pragma once

#include <string>

/**
 * @file sanitizador.hpp
 * @brief Motor de sanitización, normalización y gobernanza de nombres de proyectos y centros de costo.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 1 - Paso 2)
 */

namespace NormalizadorMC {

/**
 * @brief Sanitiza una cadena según las reglas de compatibilidad de sistemas de archivos (XFS y Windows SMB).
 * Mapea tildes y eñes a ASCII básico, convierte a mayúsculas estrictas, reemplaza caracteres ilegales
 * y espacios por un único guion bajo, colapsa guiones bajos repetidos y realiza un trim de guiones bajos.
 * 
 * @param entrada Cadena de origen a sanitizar.
 * @return std::wstring Cadena sanitizada en mayúsculas sin caracteres prohibidos ni tildes.
 */
std::wstring sanitizarCadena(const std::wstring& entrada);

/**
 * @brief Normaliza el código de Centro de Costo (CC).
 * Elimina caracteres no alfanuméricos (excepto el punto decimal), asegura el prefijo 'CC'
 * o retorna 'S_CC' si la entrada es vacía o nula.
 * 
 * @param rawCC Cadena en bruto del Centro de Costo.
 * @return std::wstring Código de Centro de Costo normalizado (ej. CC4322, CC2.50, S_CC).
 */
std::wstring formatearCC(const std::wstring& rawCC);

/**
 * @brief Ensambla el nombre oficial de la carpeta de proyecto con blindaje preventivo para MAX_PATH.
 * Aplica la fórmula [PREFIJO_CC]_[CLIENTE]_[TIPO] (o [PREFIJO_CC]_[CLIENTE] si tipo está vacío),
 * acotando la longitud máxima a 100 caracteres sin terminar en guion bajo.
 * 
 * @param rawCC Centro de Costo de origen.
 * @param rawCliente Nombre del cliente o denominación del proyecto.
 * @param rawTipo Tipo de trabajo o entregable (opcional).
 * @return std::wstring Nombre de carpeta normalizado y acotado.
 */
std::wstring construirNombreProyecto(
    const std::wstring& rawCC,
    const std::wstring& rawCliente,
    const std::wstring& rawTipo = L""
);

} // namespace NormalizadorMC
