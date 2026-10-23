#pragma once

#include "resource.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

/**
 * @file recursos_gui.hpp
 * @brief Identificadores de controles y recursos para la interfaz gráfica Win32 de Master Control.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 2 - Paso 4)
 */

#if defined(_MSC_VER)
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

// Identificadores de Controles UI - Ventana Principal
enum ControlID : WORD {
    IDC_SEARCH_EDIT         = 1001,  ///< Barra de búsqueda superior
    IDC_BTN_REFRESCAR       = 1002,  ///< Botón para refrescar repositorio SMB
    IDC_LISTA_PROYECTOS     = 1003,  ///< ListView de proyectos (Modo Reporte)
    IDC_EDIT_RUTA_UNC       = 1004,  ///< Edit solo lectura para ruta Windows
    IDC_EDIT_RUTA_LNX       = 1005,  ///< Edit solo lectura para ruta Linux
    IDC_BTN_ABRIR_CARPETA   = 1006,  ///< Botón para abrir en Explorador de Windows
    IDC_BTN_COPIAR_UNC      = 1007,  ///< Botón para copiar ruta UNC
    IDC_BTN_COPIAR_LNX      = 1008,  ///< Botón para copiar ruta POSIX Linux
    IDC_LISTA_ENTREGABLES   = 1009,  ///< ListBox para entregables técnicos
    IDC_BTN_NUEVO_PROYECTO  = 1010,  ///< Botón para creación física de nuevo proyecto
    IDC_STATUS_BAR          = 1011,  ///< Barra de estado inferior nativa

    // Etiquetas estáticas complementarias de la ventana principal
    IDC_STATIC_BUSCAR       = 1101,
    IDC_STATIC_DETALLES     = 1102,
    IDC_STATIC_UNC          = 1103,
    IDC_STATIC_LNX          = 1104,
    IDC_STATIC_ENTREGABLES  = 1105,

    // Identificadores de Controles UI - Diálogo Modal de Creación
    IDC_MODAL_CB_CATEGORIA       = 2001,  ///< ComboBox de Línea de Negocio
    IDC_MODAL_CB_SUBDIVISION     = 2002,  ///< ComboBox de Año o Cliente Marco
    IDC_MODAL_EDIT_CC            = 2003,  ///< Edit para Centro de Costo
    IDC_MODAL_EDIT_CLIENTE       = 2004,  ///< Edit para Nombre/Cliente
    IDC_MODAL_EDIT_TIPO          = 2005,  ///< Edit para Tipo de Trabajo (opcional)
    IDC_MODAL_LBL_PREV_NOMBRE    = 2006,  ///< Label estático previsualización nombre plano
    IDC_MODAL_LBL_PREV_LONGITUD  = 2007,  ///< Label estático contador de caracteres (X / 100)
    IDC_MODAL_LBL_PREV_UNC       = 2008,  ///< Label estático ruta completa resultante
    IDC_MODAL_BTN_CREAR          = 2009,  ///< Botón Crear en servidor
    IDC_MODAL_BTN_CANCELAR       = 2010   ///< Botón Cancelar y salir
};
