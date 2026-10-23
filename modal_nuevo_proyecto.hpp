#pragma once

#include "recursos_gui.hpp"
#include "controlador_directorio.hpp"
#include "gestor_proyectos.hpp"
#include "sanitizador.hpp"
#include <string>

/**
 * @file modal_nuevo_proyecto.hpp
 * @brief Ventana secundaria modal para la creación estandarizada de proyectos en el servidor SMB.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 2 - Paso 4)
 */

namespace ModalMC {

/**
 * @brief Despliega el formulario modal de alta de proyectos con validación y previsualización en tiempo real.
 * Bloquea la ventana padre hasta que el usuario concluya la creación o cancele la operación.
 * 
 * @param hwndPadre Handle de la ventana principal propietaria.
 * @param estado Puntero al estado global de la aplicación (proporciona rutaBaseUNC).
 * @param outProyectoCreado Referencia donde se asignará el nuevo proyecto si la creación fue exitosa.
 * @return true si el proyecto fue creado físicamente en el servidor, false si se canceló o falló.
 */
bool mostrarDialogoCreacion(
    HWND hwndPadre,
    GUI::EstadoAplicacion* estado,
    MC::Proyecto& outProyectoCreado
);

} // namespace ModalMC
