#pragma once

#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include "recursos_gui.hpp"
#include "modelo.hpp"
#include "mapeo_rutas.hpp"
#include "indexador.hpp"
#include "mensajes_hilos.hpp"

/**
 * @file controlador_directorio.hpp
 * @brief Controlador del estado, máquina de estados visual y lógica de presentación para Win32.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 3 - Paso 3)
 */

namespace GUI {

/**
 * @enum class EstadoUI
 * @brief Estados de la máquina de estados visual para gobernar controles y cursor.
 */
enum class EstadoUI {
    INACTIVO,     ///< Listo para recibir órdenes y búsquedas del usuario
    ESCANEANDO,   ///< Hilo de fondo activo leyendo directorios SMB en ClearOS 7
    ERROR_RED     ///< Falla de conexión o servidor Samba inaccesible
};

/**
 * @struct EstadoAplicacion
 * @brief Estructura de estado acoplada al ciclo de vida de la ventana principal mediante GWLP_USERDATA.
 */
struct EstadoAplicacion {
    HWND hwndVentanaPrincipal   = nullptr;
    HWND hwndBusqueda           = nullptr;
    HWND hwndBtnRefrescar       = nullptr;
    HWND hwndListaProyectos     = nullptr;
    HWND hwndEditRutaUNC        = nullptr;
    HWND hwndEditRutaLnx        = nullptr;
    HWND hwndBtnAbrirCarpeta    = nullptr;
    HWND hwndBtnCopiarUNC       = nullptr;
    HWND hwndBtnCopiarLnx       = nullptr;
    HWND hwndListaEntregables   = nullptr;
    HWND hwndBtnNuevo           = nullptr;
    HWND hwndStatusBar          = nullptr;

    std::wstring rutaBaseUNC;                                  ///< Ruta base UNC corporativa en red SMB
    std::vector<MC::Proyecto> inventarioMaestro;                ///< Inventario total de proyectos en RAM
    std::vector<const MC::Proyecto*> vistaFiltrada;            ///< Proyectos coincidentes con el filtro
    const MC::Proyecto* proyectoSeleccionado = nullptr;         ///< Puntero al proyecto actualmente seleccionado
    std::vector<MC::ArchivoEntregable> entregablesActuales;    ///< Entregables lazy del proyecto activo

    // Gestión concurrente de hilos de fondo y cancelación cooperativa
    std::thread hiloEscaneo;                                   ///< Hilo secundario de exploración SMB
    std::atomic<bool> escaneoEnProgreso{ false };              ///< Bandera atómica de sincronización
    std::atomic<bool> solicitarCancelacion{ false };           ///< Token atómico de cancelación cooperativa

    // Máquina de estados visual y cursores
    EstadoUI estadoActual = EstadoUI::INACTIVO;                 ///< Estado visual operativo
    HCURSOR hCursorEspera = nullptr;                           ///< Cursor de reloj de arena / espera (IDC_WAIT)
    HCURSOR hCursorNormal = nullptr;                           ///< Cursor de flecha estándar (IDC_ARROW)

    /**
     * @brief Cancela cooperativamente el hilo secundario de escaneo y bloquea hasta que concluya limpiamente.
     * Evita violaciones de acceso (0xC0000005) y descarta recursos de red de forma segura.
     */
    void detenerHiloEscaneo();
};

/**
 * @brief Transición centralizada de la máquina de estados visual.
 * Habilita/deshabilita controles y conmuta cursores sin riesgo de carreras.
 * 
 * @param estado Puntero al estado de la aplicación.
 * @param nuevoEstado Estado al que transiciona la interfaz.
 */
void cambiarEstadoUI(EstadoAplicacion* estado, EstadoUI nuevoEstado);

/**
 * @brief Llena el control WC_LISTVIEW con los proyectos presentes en la vista filtrada.
 */
void poblarTablaProyectos(EstadoAplicacion* estado);

/**
 * @brief Filtra en memoria RAM según el término tecleado por el usuario.
 */
void aplicarFiltro(EstadoAplicacion* estado, const std::wstring& termino);

/**
 * @brief Inicia la exploración y lectura del servidor SMB en un hilo secundario independiente (std::thread).
 * Sincroniza la cancelación previa si un escaneo estuviera en curso.
 */
void iniciarEscaneoAsincrono(EstadoAplicacion* estado);

/**
 * @brief Procesa la selección de un proyecto: llena cuadros de texto, habilita botones
 * y ejecuta la carga bajo demanda (lazy loading) de los entregables técnicos en el ListBox.
 */
void seleccionarProyecto(EstadoAplicacion* estado, const MC::Proyecto* pProyecto);

/**
 * @brief Abre el archivo entregable seleccionado en el ListBox con su aplicación predeterminada.
 */
void abrirEntregableSeleccionado(EstadoAplicacion* estado);

} // namespace GUI
