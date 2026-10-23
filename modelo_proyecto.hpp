#pragma once

#include <string>
#include <utility>

/**
 * @file modelo_proyecto.hpp
 * @brief Estructura de datos fundamental para la representación de proyectos de Master Control.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 1 - Paso 1)
 */

namespace MC {

/**
 * @struct Proyecto
 * @brief Entidad de dominio que encapsula los metadatos y rutas físicas/lógicas de un proyecto.
 */
struct Proyecto {
    std::wstring nombreCarpeta;  ///< Nombre plano del proyecto / anclaje (ej. CC4322_PAMPITE_ARTZ_OBRA_NUEVA)
    std::wstring categoria;      ///< Línea de negocio (ej. 01_OBRAS_E_INSTALACIONES_(CC_4.XX_Y_4XXX))
    std::wstring subdivision;    ///< División temporal o comercial (ej. 2026, ENI_ECUADOR)
    std::wstring rutaUNC;        ///< Ruta absoluta UNC de acceso en red Windows (\\\\192.168.1.242\\...)
    std::wstring rutaLinux;      ///< Ruta absoluta POSIX en el servidor ClearOS 7 (/home/mcontrol/...)

    /**
     * @brief Constructor predeterminado. Inicializa todas las cadenas vacías.
     */
    Proyecto() = default;

    /**
     * @brief Constructor parametrizado con semántica de movimiento (move semantics).
     * @param nombre Nombre de la carpeta del proyecto.
     * @param cat Categoría o línea de negocio correspondiente.
     * @param subdiv Año, cliente recurrente o subdivisión operativa.
     * @param unc Ruta completa UNC en red Windows.
     * @param linuxRuta Ruta equivalente POSIX en servidor Linux.
     */
    Proyecto(std::wstring nombre,
             std::wstring cat,
             std::wstring subdiv,
             std::wstring unc,
             std::wstring linuxRuta)
        : nombreCarpeta(std::move(nombre)),
          categoria(std::move(cat)),
          subdivision(std::move(subdiv)),
          rutaUNC(std::move(unc)),
          rutaLinux(std::move(linuxRuta)) {}

    /**
     * @brief Comprueba si la entidad contiene datos válidos mínimos.
     * @return true si tiene al menos nombre de carpeta y ruta UNC definida.
     */
    [[nodiscard]] bool esValido() const noexcept {
        return !nombreCarpeta.empty() && !rutaUNC.empty();
    }
};

} // namespace MC
