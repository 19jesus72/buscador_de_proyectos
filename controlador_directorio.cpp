#include "controlador_directorio.hpp"
#include "acciones_shell.hpp"
#include <cwctype>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

namespace GUI {

namespace {

std::wstring convertirAMayusculas(const std::wstring& str) {
    std::wstring mayus;
    mayus.reserve(str.length());
    for (wchar_t ch : str) {
        mayus.push_back(std::towupper(ch));
    }
    return mayus;
}

bool contieneSubcadenaInsensible(const std::wstring& texto, const std::wstring& subMayus) {
    if (subMayus.empty()) return true;
    if (texto.empty()) return false;

    auto it = std::search(
        texto.begin(), texto.end(),
        subMayus.begin(), subMayus.end(),
        [](wchar_t c1, wchar_t c2) {
            return std::towupper(c1) == c2;
        }
    );
    return (it != texto.end());
}

} // namespace anónimo

void EstadoAplicacion::detenerHiloEscaneo() {
    if (escaneoEnProgreso.load()) {
        // 1. Indicar al hilo de trabajo que debe abortar su ejecución cooperativamente
        solicitarCancelacion.store(true);
    }

    // 2. Esperar pacientemente a que el hilo concluya y libere sus descriptores
    if (hiloEscaneo.joinable()) {
        hiloEscaneo.join();
    }

    escaneoEnProgreso.store(false);
    solicitarCancelacion.store(false);
}

void cambiarEstadoUI(EstadoAplicacion* estado, EstadoUI nuevoEstado) {
    if (!estado) {
        return;
    }

    estado->estadoActual = nuevoEstado;

    if (nuevoEstado == EstadoUI::ESCANEANDO) {
        // 1. Bloquear controles que inducirían concurrencia o mutaciones durante la lectura SMB
        if (estado->hwndBtnRefrescar) {
            EnableWindow(estado->hwndBtnRefrescar, FALSE);
        }
        if (estado->hwndBtnNuevo) {
            EnableWindow(estado->hwndBtnNuevo, FALSE);
        }
        if (estado->hwndBusqueda) {
            EnableWindow(estado->hwndBusqueda, FALSE);
        }

        // 2. Deshabilitar botones de acción
        if (estado->hwndBtnAbrirCarpeta) EnableWindow(estado->hwndBtnAbrirCarpeta, FALSE);
        if (estado->hwndBtnCopiarUNC)    EnableWindow(estado->hwndBtnCopiarUNC, FALSE);
        if (estado->hwndBtnCopiarLnx)    EnableWindow(estado->hwndBtnCopiarLnx, FALSE);

        // 3. Forzar cursor de espera inmediatamente
        if (estado->hCursorEspera) {
            SetCursor(estado->hCursorEspera);
        }
    } else {
        // Estado INACTIVO o ERROR_RED: Restaurar plena operatividad
        if (estado->hwndBtnRefrescar) {
            EnableWindow(estado->hwndBtnRefrescar, TRUE);
        }
        if (estado->hwndBtnNuevo) {
            EnableWindow(estado->hwndBtnNuevo, TRUE);
        }
        if (estado->hwndBusqueda) {
            EnableWindow(estado->hwndBusqueda, TRUE);
        }

        // Restaurar cursor estándar de flecha
        if (estado->hCursorNormal) {
            SetCursor(estado->hCursorNormal);
        }

        // Restablecer botones de acción según la existencia de selección
        bool tieneSeleccion = (estado->proyectoSeleccionado != nullptr);
        if (estado->hwndBtnAbrirCarpeta) EnableWindow(estado->hwndBtnAbrirCarpeta, tieneSeleccion);
        if (estado->hwndBtnCopiarUNC)    EnableWindow(estado->hwndBtnCopiarUNC, tieneSeleccion);
        if (estado->hwndBtnCopiarLnx)    EnableWindow(estado->hwndBtnCopiarLnx, tieneSeleccion);
    }
}

void poblarTablaProyectos(EstadoAplicacion* estado) {
    if (!estado || !estado->hwndListaProyectos) {
        return;
    }

    SendMessageW(estado->hwndListaProyectos, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(estado->hwndListaProyectos);

    for (size_t i = 0; i < estado->vistaFiltrada.size(); ++i) {
        const MC::Proyecto* pProyecto = estado->vistaFiltrada[i];
        if (!pProyecto) continue;

        LVITEMW item = { 0 };
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = static_cast<int>(i);
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(pProyecto->nombreCarpeta.c_str());
        item.lParam = reinterpret_cast<LPARAM>(pProyecto);

        int idx = ListView_InsertItem(estado->hwndListaProyectos, &item);
        if (idx != -1) {
            ListView_SetItemText(
                estado->hwndListaProyectos, idx, 1,
                const_cast<LPWSTR>(pProyecto->categoria.c_str())
            );
            ListView_SetItemText(
                estado->hwndListaProyectos, idx, 2,
                const_cast<LPWSTR>(pProyecto->subdivision.c_str())
            );
        }
    }

    SendMessageW(estado->hwndListaProyectos, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(estado->hwndListaProyectos, nullptr, TRUE);

    seleccionarProyecto(estado, nullptr);

    if (estado->hwndStatusBar) {
        std::wstring textoEstado = L"Mostrando: " +
                                   std::to_wstring(estado->vistaFiltrada.size()) +
                                   L" de " +
                                   std::to_wstring(estado->inventarioMaestro.size()) +
                                   L" proyectos";

        SendMessageW(estado->hwndStatusBar, SB_SETTEXTW, 0,
                     reinterpret_cast<LPARAM>(textoEstado.c_str()));

        std::wstring textoServidor = L"Servidor: " + estado->rutaBaseUNC;
        SendMessageW(estado->hwndStatusBar, SB_SETTEXTW, 1,
                     reinterpret_cast<LPARAM>(textoServidor.c_str()));
    }
}

void aplicarFiltro(EstadoAplicacion* estado, const std::wstring& termino) {
    if (!estado) {
        return;
    }

    // Si está escaneando, no ejecutar filtrado para evitar lecturas sobre vectores en mutación
    if (estado->estadoActual == EstadoUI::ESCANEANDO) {
        return;
    }

    estado->vistaFiltrada.clear();

    if (termino.empty()) {
        estado->vistaFiltrada.reserve(estado->inventarioMaestro.size());
        for (const auto& p : estado->inventarioMaestro) {
            estado->vistaFiltrada.push_back(&p);
        }
    } else {
        std::wstring terminoMayus = convertirAMayusculas(termino);
        estado->vistaFiltrada.reserve(estado->inventarioMaestro.size() / 2);

        for (const auto& p : estado->inventarioMaestro) {
            if (contieneSubcadenaInsensible(p.nombreCarpeta, terminoMayus) ||
                contieneSubcadenaInsensible(p.categoria, terminoMayus) ||
                contieneSubcadenaInsensible(p.subdivision, terminoMayus)) {
                estado->vistaFiltrada.push_back(&p);
            }
        }
    }

    poblarTablaProyectos(estado);
}

void iniciarEscaneoAsincrono(EstadoAplicacion* estado) {
    if (!estado) {
        return;
    }

    // Asegurar la detención previa y sincronizada de cualquier ciclo de escaneo previo
    estado->detenerHiloEscaneo();

    estado->escaneoEnProgreso.store(true);
    estado->solicitarCancelacion.store(false);

    if (estado->hwndVentanaPrincipal) {
        PostMessageW(estado->hwndVentanaPrincipal, WM_APP_SCAN_INICIADO, 0, 0);
    }

    estado->hiloEscaneo = std::thread([estado]() {
        long long duracion = 0;
        auto inventario = IndexadorMC::indexarRepositorioConProgreso(
            estado->rutaBaseUNC,
            estado->hwndVentanaPrincipal,
            estado->solicitarCancelacion,
            duracion
        );

        // Si el hilo fue cancelado durante su trabajo o falló críticamente, no enviar resultado final
        if (!estado->solicitarCancelacion.load() && duracion >= 0) {
            auto* pResultado = new DatosResultadoScan{ std::move(inventario), duracion };
            PostMessageW(estado->hwndVentanaPrincipal, WM_APP_SCAN_COMPLETADO, 0, reinterpret_cast<LPARAM>(pResultado));
        }
    });
}

void seleccionarProyecto(EstadoAplicacion* estado, const MC::Proyecto* pProyecto) {
    if (!estado) return;

    estado->proyectoSeleccionado = pProyecto;

    if (!pProyecto) {
        if (estado->hwndEditRutaUNC) SetWindowTextW(estado->hwndEditRutaUNC, L"");
        if (estado->hwndEditRutaLnx) SetWindowTextW(estado->hwndEditRutaLnx, L"");

        if (estado->hwndBtnAbrirCarpeta) EnableWindow(estado->hwndBtnAbrirCarpeta, FALSE);
        if (estado->hwndBtnCopiarUNC)    EnableWindow(estado->hwndBtnCopiarUNC, FALSE);
        if (estado->hwndBtnCopiarLnx)    EnableWindow(estado->hwndBtnCopiarLnx, FALSE);

        if (estado->hwndListaEntregables) {
            SendMessageW(estado->hwndListaEntregables, LB_RESETCONTENT, 0, 0);
        }
        estado->entregablesActuales.clear();
        return;
    }

    if (estado->hwndEditRutaUNC) {
        SetWindowTextW(estado->hwndEditRutaUNC, pProyecto->rutaUNC.c_str());
    }
    if (estado->hwndEditRutaLnx) {
        SetWindowTextW(estado->hwndEditRutaLnx, pProyecto->rutaLinux.c_str());
    }

    // Habilitar botones de acción solo si no estamos en estado de escaneo activo
    if (estado->estadoActual != EstadoUI::ESCANEANDO) {
        if (estado->hwndBtnAbrirCarpeta) EnableWindow(estado->hwndBtnAbrirCarpeta, TRUE);
        if (estado->hwndBtnCopiarUNC)    EnableWindow(estado->hwndBtnCopiarUNC, TRUE);
        if (estado->hwndBtnCopiarLnx)    EnableWindow(estado->hwndBtnCopiarLnx, TRUE);
    }

    estado->entregablesActuales = MC::listarEntregables(pProyecto->rutaUNC);

    if (estado->hwndListaEntregables) {
        SendMessageW(estado->hwndListaEntregables, WM_SETREDRAW, FALSE, 0);
        SendMessageW(estado->hwndListaEntregables, LB_RESETCONTENT, 0, 0);

        if (estado->entregablesActuales.empty()) {
            SendMessageW(estado->hwndListaEntregables, LB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(L"(Carpeta vacía / Sin entregables respaldados)"));
        } else {
            for (const auto& archivo : estado->entregablesActuales) {
                std::wstringstream ss;
                ss << archivo.nombre << L"  ("
                   << std::fixed << std::setprecision(2) << archivo.tamanioMB << L" MB)";
                SendMessageW(estado->hwndListaEntregables, LB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(ss.str().c_str()));
            }
        }

        SendMessageW(estado->hwndListaEntregables, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(estado->hwndListaEntregables, nullptr, TRUE);
    }

    if (estado->hwndStatusBar) {
        std::wstring textoDetalle = L"Activo: " + pProyecto->nombreCarpeta +
                                    L" (" + std::to_wstring(estado->entregablesActuales.size()) + L" archivos)";
        SendMessageW(estado->hwndStatusBar, SB_SETTEXTW, 0,
                     reinterpret_cast<LPARAM>(textoDetalle.c_str()));
    }
}

void abrirEntregableSeleccionado(EstadoAplicacion* estado) {
    if (!estado || !estado->proyectoSeleccionado || !estado->hwndListaEntregables) {
        return;
    }

    if (estado->estadoActual == EstadoUI::ESCANEANDO) {
        return;
    }

    LRESULT selIdx = SendMessageW(estado->hwndListaEntregables, LB_GETCURSEL, 0, 0);
    if (selIdx == LB_ERR) {
        return;
    }

    if (static_cast<size_t>(selIdx) < estado->entregablesActuales.size()) {
        const auto& arch = estado->entregablesActuales[selIdx];
        fs::path rutaCompleta = fs::path(estado->proyectoSeleccionado->rutaUNC) / arch.nombre;
        ShellMC::abrirArchivoConAsociado(estado->hwndVentanaPrincipal, rutaCompleta.wstring());
    }
}

} // namespace GUI
