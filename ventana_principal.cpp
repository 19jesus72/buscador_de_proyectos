#include "ventana_principal.hpp"
#include "resource.h"
#include "controlador_directorio.hpp"
#include "acciones_shell.hpp"
#include "portapapeles.hpp"
#include "modal_nuevo_proyecto.hpp"
#include "mensajes_hilos.hpp"
#include <commctrl.h>
#include <string>

namespace GUI {

namespace {

// Fuentes tipográficas de la interfaz
HFONT g_hFontNormal = nullptr;
HFONT g_hFontBold   = nullptr;
HFONT g_hFontTitle  = nullptr;

// Manijas de controles principales
HWND g_hwndSearchEdit      = nullptr;
HWND g_hwndBtnRefrescar    = nullptr;
HWND g_hwndListProyectos   = nullptr;
HWND g_hwndEditRutaUNC     = nullptr;
HWND g_hwndEditRutaLnx     = nullptr;
HWND g_hwndBtnAbrirCarpeta = nullptr;
HWND g_hwndBtnCopiarUNC    = nullptr;
HWND g_hwndBtnCopiarLnx    = nullptr;
HWND g_hwndListEntregables = nullptr;
HWND g_hwndBtnNuevo        = nullptr;
HWND g_hwndStatusBar       = nullptr;

// Manijas de etiquetas estáticas para redimensionamiento
HWND g_hwndStaticBuscar      = nullptr;
HWND g_hwndStaticDetalles    = nullptr;
HWND g_hwndStaticUNC         = nullptr;
HWND g_hwndStaticLNX         = nullptr;
HWND g_hwndStaticEntregables = nullptr;

void crearFuentesSistema() {
    if (!g_hFontNormal) {
        g_hFontNormal = CreateFontW(
            -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );
    }
    if (!g_hFontBold) {
        g_hFontBold = CreateFontW(
            -12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );
    }
    if (!g_hFontTitle) {
        g_hFontTitle = CreateFontW(
            -15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );
    }
}

void destruirFuentesSistema() {
    if (g_hFontNormal) { DeleteObject(g_hFontNormal); g_hFontNormal = nullptr; }
    if (g_hFontBold)   { DeleteObject(g_hFontBold);   g_hFontBold = nullptr; }
    if (g_hFontTitle)  { DeleteObject(g_hFontTitle);  g_hFontTitle = nullptr; }
}

void aplicarFuente(HWND hwndCtrl, HFONT hFont) {
    if (hwndCtrl && hFont) {
        SendMessageW(hwndCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }
}

} // namespace anónimo

bool registrarClaseVentana(HINSTANCE hInstance) {
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);

    wc.hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    if (!wc.hIcon) {
        wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    if (!wc.hIconSm) {
        wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
    }

    wc.hInstance     = hInstance;
    wc.lpfnWndProc   = WndProcPrincipal;
    wc.lpszClassName = NOMBRE_CLASE_VENTANA;
    wc.lpszMenuName  = nullptr;
    wc.style         = CS_HREDRAW | CS_VREDRAW;

    return (RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS);
}

HWND crearVentanaPrincipal(HINSTANCE hInstance, int nCmdShow, const std::wstring& rutaBaseUNC) {
    constexpr int anchoInicial = 1100;
    constexpr int altoInicial  = 700;

    int pantallaAncho = GetSystemMetrics(SM_CXSCREEN);
    int pantallaAlto  = GetSystemMetrics(SM_CYSCREEN);
    int xPos = (pantallaAncho - anchoInicial) / 2;
    int yPos = (pantallaAlto - altoInicial) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_WINDOWEDGE,
        NOMBRE_CLASE_VENTANA,
        TITULO_VENTANA,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        xPos, yPos,
        anchoInicial, altoInicial,
        nullptr,
        nullptr,
        hInstance,
        reinterpret_cast<LPVOID>(const_cast<std::wstring*>(&rutaBaseUNC))
    );

    if (hwnd) {
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }

    return hwnd;
}

LRESULT CALLBACK WndProcPrincipal(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    EstadoAplicacion* pEstado = reinterpret_cast<EstadoAplicacion*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA)
    );

    switch (msg) {
        case WM_CREATE: {
            crearFuentesSistema();

            // 1. Instanciar estado acoplado a la ventana
            CREATESTRUCTW* pCS = reinterpret_cast<CREATESTRUCTW*>(lParam);
            std::wstring rutaBase = MC::BASE_UNC_DEFAULT;
            if (pCS && pCS->lpCreateParams) {
                const std::wstring* pRuta = reinterpret_cast<const std::wstring*>(pCS->lpCreateParams);
                if (pRuta && !pRuta->empty()) {
                    rutaBase = *pRuta;
                }
            }

            pEstado = new EstadoAplicacion();
            pEstado->hwndVentanaPrincipal = hwnd;
            pEstado->rutaBaseUNC = rutaBase;

            // Cargar cursores del sistema para la máquina de estados
            pEstado->hCursorEspera = LoadCursorW(nullptr, IDC_WAIT);
            pEstado->hCursorNormal = LoadCursorW(nullptr, IDC_ARROW);

            // Cargar y aplicar iconos corporativos de alta nitidez
            HICON hIconBig = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
            HICON hIconSmall = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

            if (hIconBig) {
                SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIconBig));
            }
            if (hIconSmall) {
                SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconSmall));
            }

            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pEstado));

            // -------------------------------------------------------------
            // 2. BARRA SUPERIOR (Búsqueda y Actualización)
            // -------------------------------------------------------------
            g_hwndStaticBuscar = CreateWindowExW(
                0, L"STATIC", L"Buscar Proyecto:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATIC_BUSCAR)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndStaticBuscar, g_hFontBold);

            g_hwndSearchEdit = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SEARCH_EDIT)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndSearchEdit, g_hFontNormal);
            pEstado->hwndBusqueda = g_hwndSearchEdit;

            SendMessageW(g_hwndSearchEdit, EM_SETCUEBANNER, TRUE,
                         reinterpret_cast<LPARAM>(L"Filtrar por CC, cliente o año (ej: CC4322, PAMPITE, 2026)..."));

            g_hwndBtnRefrescar = CreateWindowExW(
                0, L"BUTTON", L"⟳ Actualizar",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_REFRESCAR)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndBtnRefrescar, g_hFontNormal);
            pEstado->hwndBtnRefrescar = g_hwndBtnRefrescar;

            // -------------------------------------------------------------
            // 3. PANEL IZQUIERDO: ListView de Proyectos (Report View)
            // -------------------------------------------------------------
            g_hwndListProyectos = CreateWindowExW(
                WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LISTA_PROYECTOS)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndListProyectos, g_hFontNormal);
            pEstado->hwndListaProyectos = g_hwndListProyectos;

            ListView_SetExtendedListViewStyle(
                g_hwndListProyectos,
                LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER
            );

            LVCOLUMNW lvc = { sizeof(LVCOLUMNW) };
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

            lvc.iSubItem = 0;
            lvc.cx = 360;
            lvc.pszText = const_cast<LPWSTR>(L"Carpeta del Proyecto (Punto de Anclaje)");
            ListView_InsertColumn(g_hwndListProyectos, 0, &lvc);

            lvc.iSubItem = 1;
            lvc.cx = 240;
            lvc.pszText = const_cast<LPWSTR>(L"Línea de Negocio / Categoría");
            ListView_InsertColumn(g_hwndListProyectos, 1, &lvc);

            lvc.iSubItem = 2;
            lvc.cx = 130;
            lvc.pszText = const_cast<LPWSTR>(L"Año / Subdivisión");
            ListView_InsertColumn(g_hwndListProyectos, 2, &lvc);

            // -------------------------------------------------------------
            // 4. PANEL DERECHO: Detalles, Acciones y Entregables
            // -------------------------------------------------------------
            g_hwndStaticDetalles = CreateWindowExW(
                0, L"STATIC", L"Detalles del Proyecto Seleccionado",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATIC_DETALLES)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndStaticDetalles, g_hFontTitle);

            g_hwndStaticUNC = CreateWindowExW(
                0, L"STATIC", L"Ruta de red Windows (SMB / UNC):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATIC_UNC)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndStaticUNC, g_hFontNormal);

            g_hwndEditRutaUNC = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_RUTA_UNC)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndEditRutaUNC, g_hFontNormal);
            pEstado->hwndEditRutaUNC = g_hwndEditRutaUNC;

            g_hwndStaticLNX = CreateWindowExW(
                0, L"STATIC", L"Ruta en Servidor Linux (ClearOS 7 / POSIX):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATIC_LNX)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndStaticLNX, g_hFontNormal);

            g_hwndEditRutaLnx = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_RUTA_LNX)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndEditRutaLnx, g_hFontNormal);
            pEstado->hwndEditRutaLnx = g_hwndEditRutaLnx;

            // Fila de botones de acción rápida
            g_hwndBtnAbrirCarpeta = CreateWindowExW(
                0, L"BUTTON", L"📂 Abrir Carpeta",
                WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_PUSHBUTTON,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_ABRIR_CARPETA)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndBtnAbrirCarpeta, g_hFontNormal);
            pEstado->hwndBtnAbrirCarpeta = g_hwndBtnAbrirCarpeta;

            g_hwndBtnCopiarUNC = CreateWindowExW(
                0, L"BUTTON", L"📋 Copiar UNC",
                WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_PUSHBUTTON,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_COPIAR_UNC)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndBtnCopiarUNC, g_hFontNormal);
            pEstado->hwndBtnCopiarUNC = g_hwndBtnCopiarUNC;

            g_hwndBtnCopiarLnx = CreateWindowExW(
                0, L"BUTTON", L"📋 Copiar Linux",
                WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_PUSHBUTTON,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_COPIAR_LNX)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndBtnCopiarLnx, g_hFontNormal);
            pEstado->hwndBtnCopiarLnx = g_hwndBtnCopiarLnx;

            g_hwndStaticEntregables = CreateWindowExW(
                0, L"STATIC", L"Entregables y Archivos Técnicos (Doble clic para abrir):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATIC_ENTREGABLES)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndStaticEntregables, g_hFontBold);

            g_hwndListEntregables = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LISTA_ENTREGABLES)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndListEntregables, g_hFontNormal);
            pEstado->hwndListaEntregables = g_hwndListEntregables;

            g_hwndBtnNuevo = CreateWindowExW(
                0, L"BUTTON", L"➕ Crear Nuevo Proyecto Estandarizado",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_NUEVO_PROYECTO)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndBtnNuevo, g_hFontBold);
            pEstado->hwndBtnNuevo = g_hwndBtnNuevo;

            // -------------------------------------------------------------
            // 5. BARRA DE ESTADO INFERIOR NATIVA
            // -------------------------------------------------------------
            g_hwndStatusBar = CreateWindowExW(
                0, STATUSCLASSNAMEW, nullptr,
                WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATUS_BAR)),
                pCS->hInstance, nullptr
            );
            aplicarFuente(g_hwndStatusBar, g_hFontNormal);
            pEstado->hwndStatusBar = g_hwndStatusBar;

            int partesStatus[] = { 360, 780, -1 };
            SendMessageW(g_hwndStatusBar, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(partesStatus));
            SendMessageW(g_hwndStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Listo. Conectando con servidor..."));
            SendMessageW(g_hwndStatusBar, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(pEstado->rutaBaseUNC.c_str()));
            SendMessageW(g_hwndStatusBar, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"C++17 / Win32 Nativo Monolítico"));

            // 6. LANZAMIENTO DEL ESCANEO ASÍNCRONO EN SEGUNDO PLANO (std::thread)
            iniciarEscaneoAsincrono(pEstado);

            return 0;
        }

        // =================================================================
        // MÁQUINA DE ESTADOS VISUAL: GESTIÓN PERSISTENTE DEL CURSOR
        // =================================================================
        case WM_SETCURSOR: {
            if (pEstado && pEstado->estadoActual == EstadoUI::ESCANEANDO) {
                // Si el puntero se encuentra sobre el área cliente de la ventana
                if (LOWORD(lParam) == HTCLIENT) {
                    SetCursor(pEstado->hCursorEspera);
                    return TRUE; // Notificar a Windows que el cursor fue despachado
                }
            }
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        // =================================================================
        // MANEJADORES DE MENSAJES DEL HILO DE TRABAJO (WM_APP)
        // =================================================================
        case WM_APP_SCAN_INICIADO: {
            if (pEstado && pEstado->solicitarCancelacion.load()) {
                return 0;
            }
            if (pEstado) {
                cambiarEstadoUI(pEstado, EstadoUI::ESCANEANDO);
            }
            if (g_hwndStatusBar) {
                SendMessageW(g_hwndStatusBar, SB_SETTEXTW, 0,
                             reinterpret_cast<LPARAM>(L"Conectando a \\\\192.168.1.242 y leyendo estructura..."));
            }
            return 0;
        }

        case WM_APP_SCAN_PROGRESO: {
            auto* pCat = reinterpret_cast<std::wstring*>(lParam);
            if (pEstado && pEstado->solicitarCancelacion.load()) {
                delete pCat;
                return 0;
            }
            if (pCat) {
                if (g_hwndStatusBar) {
                    std::wstring msgProgreso = L"Indexando: " + *pCat + L"...";
                    SendMessageW(g_hwndStatusBar, SB_SETTEXTW, 0,
                                 reinterpret_cast<LPARAM>(msgProgreso.c_str()));
                }
                delete pCat;
            }
            return 0;
        }

        case WM_APP_SCAN_COMPLETADO: {
            auto* pRes = reinterpret_cast<DatosResultadoScan*>(lParam);
            if (pEstado && pEstado->solicitarCancelacion.load()) {
                delete pRes;
                return 0;
            }
            if (pRes && pEstado) {
                pEstado->inventarioMaestro = std::move(pRes->inventario);
                long long ms = pRes->duracionMs;
                delete pRes;

                pEstado->escaneoEnProgreso.store(false);

                // Transición visual a INACTIVO (restaura controles y cursor estándar)
                cambiarEstadoUI(pEstado, EstadoUI::INACTIVO);

                // Aplicar el filtro según el término actual en el cuadro de búsqueda
                wchar_t bufferBusqueda[256] = { 0 };
                if (g_hwndSearchEdit) {
                    GetWindowTextW(g_hwndSearchEdit, bufferBusqueda, 256);
                }
                aplicarFiltro(pEstado, bufferBusqueda);

                if (g_hwndStatusBar) {
                    std::wstring txtFinal = L"Listo: " +
                                            std::to_wstring(pEstado->inventarioMaestro.size()) +
                                            L" proyectos indexados en " +
                                            std::to_wstring(ms) + L" ms.";
                    SendMessageW(g_hwndStatusBar, SB_SETTEXTW, 0,
                                 reinterpret_cast<LPARAM>(txtFinal.c_str()));
                }
            }
            return 0;
        }

        case WM_APP_SCAN_FALLIDO: {
            auto* pErr = reinterpret_cast<std::wstring*>(lParam);
            if (pEstado && pEstado->solicitarCancelacion.load()) {
                delete pErr;
                return 0;
            }
            if (pErr) {
                if (g_hwndStatusBar) {
                    std::wstring txtErr = L"ERROR SMB: " + *pErr;
                    SendMessageW(g_hwndStatusBar, SB_SETTEXTW, 0,
                                 reinterpret_cast<LPARAM>(txtErr.c_str()));
                }

                // Si la lista de proyectos en pantalla estaba vacía, poblar fila explicativa
                if (pEstado && pEstado->inventarioMaestro.empty() && g_hwndListaProyectos) {
                    SendMessageW(g_hwndListaProyectos, WM_SETREDRAW, FALSE, 0);
                    ListView_DeleteAllItems(g_hwndListaProyectos);

                    LVITEMW item = { 0 };
                    item.mask = LVIF_TEXT;
                    item.iItem = 0;
                    item.iSubItem = 0;
                    wchar_t col0[] = L"SIN CONEXIÓN AL SERVIDOR SMB";
                    item.pszText = col0;

                    int idx = ListView_InsertItem(g_hwndListaProyectos, &item);
                    if (idx != -1) {
                        wchar_t col1[] = L"Verifique la red o presione 'Actualizar'";
                        wchar_t col2[] = L"192.168.1.242";
                        ListView_SetItemText(g_hwndListaProyectos, idx, 1, col1);
                        ListView_SetItemText(g_hwndListaProyectos, idx, 2, col2);
                    }

                    SendMessageW(g_hwndListaProyectos, WM_SETREDRAW, TRUE, 0);
                    InvalidateRect(g_hwndListaProyectos, nullptr, TRUE);
                }

                delete pErr;
            }
            if (pEstado) {
                pEstado->escaneoEnProgreso.store(false);
                cambiarEstadoUI(pEstado, EstadoUI::ERROR_RED);
            }
            return 0;
        }

        case WM_NOTIFY: {
            if (!pEstado) break;

            LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);

            if (pnmh->idFrom == IDC_LISTA_PROYECTOS) {
                if (pnmh->code == LVN_ITEMCHANGED) {
                    LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(lParam);
                    if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_SELECTED)) {
                        const MC::Proyecto* p = reinterpret_cast<const MC::Proyecto*>(pnmv->lParam);
                        seleccionarProyecto(pEstado, p);
                    }
                    return 0;
                }

                if (pnmh->code == NM_DBLCLK) {
                    LPNMITEMACTIVATE pnmia = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
                    if (pnmia->iItem != -1) {
                        LVITEMW lvi = { 0 };
                        lvi.iItem = pnmia->iItem;
                        lvi.mask = LVIF_PARAM;
                        if (ListView_GetItem(g_hwndListProyectos, &lvi) && lvi.lParam) {
                            const MC::Proyecto* p = reinterpret_cast<const MC::Proyecto*>(lvi.lParam);
                            ShellMC::abrirCarpetaEnExplorador(hwnd, p->rutaUNC);
                        }
                    }
                    return 0;
                }
            }
            break;
        }

        case WM_COMMAND: {
            WORD controlId = LOWORD(wParam);
            WORD notificacion = HIWORD(wParam);

            // Filtrado reactivo en el campo de texto (solo si no estamos escaneando)
            if (controlId == IDC_SEARCH_EDIT && notificacion == EN_CHANGE) {
                if (pEstado && g_hwndSearchEdit && pEstado->estadoActual != EstadoUI::ESCANEANDO) {
                    wchar_t buffer[256] = { 0 };
                    GetWindowTextW(g_hwndSearchEdit, buffer, 256);
                    aplicarFiltro(pEstado, buffer);
                }
                return 0;
            }

            // Botón de re-escaneo / actualización (Bloqueo si ya está escaneando)
            if (controlId == IDC_BTN_REFRESCAR && notificacion == BN_CLICKED) {
                if (pEstado && pEstado->estadoActual != EstadoUI::ESCANEANDO) {
                    iniciarEscaneoAsincrono(pEstado);
                }
                return 0;
            }

            // Doble clic en un archivo del ListBox
            if (controlId == IDC_LISTA_ENTREGABLES && notificacion == LBN_DBLCLK) {
                if (pEstado && pEstado->estadoActual != EstadoUI::ESCANEANDO) {
                    abrirEntregableSeleccionado(pEstado);
                }
                return 0;
            }

            // Botón Abrir Carpeta
            if (controlId == IDC_BTN_ABRIR_CARPETA && notificacion == BN_CLICKED) {
                if (pEstado && pEstado->proyectoSeleccionado && pEstado->estadoActual != EstadoUI::ESCANEANDO) {
                    ShellMC::abrirCarpetaEnExplorador(hwnd, pEstado->proyectoSeleccionado->rutaUNC);
                }
                return 0;
            }

            // Botón Copiar UNC al Portapapeles
            if (controlId == IDC_BTN_COPIAR_UNC && notificacion == BN_CLICKED) {
                if (pEstado && pEstado->proyectoSeleccionado && pEstado->estadoActual != EstadoUI::ESCANEANDO) {
                    if (PortapapelesMC::copiarTexto(hwnd, pEstado->proyectoSeleccionado->rutaUNC)) {
                        if (pEstado->hwndStatusBar) {
                            SendMessageW(pEstado->hwndStatusBar, SB_SETTEXTW, 0,
                                         reinterpret_cast<LPARAM>(L"✓ Ruta UNC copiada al portapapeles."));
                        }
                    }
                }
                return 0;
            }

            // Botón Copiar Linux al Portapapeles
            if (controlId == IDC_BTN_COPIAR_LNX && notificacion == BN_CLICKED) {
                if (pEstado && pEstado->proyectoSeleccionado && pEstado->estadoActual != EstadoUI::ESCANEANDO) {
                    if (PortapapelesMC::copiarTexto(hwnd, pEstado->proyectoSeleccionado->rutaLinux)) {
                        if (pEstado->hwndStatusBar) {
                            SendMessageW(pEstado->hwndStatusBar, SB_SETTEXTW, 0,
                                         reinterpret_cast<LPARAM>(L"✓ Ruta Linux POSIX copiada al portapapeles."));
                        }
                    }
                }
                return 0;
            }

            // Botón Crear Nuevo Proyecto (Bloqueo si está escaneando)
            if (controlId == IDC_BTN_NUEVO_PROYECTO && notificacion == BN_CLICKED) {
                if (pEstado && pEstado->estadoActual != EstadoUI::ESCANEANDO) {
                    MC::Proyecto nuevoProyecto;
                    if (ModalMC::mostrarDialogoCreacion(hwnd, pEstado, nuevoProyecto)) {
                        pEstado->inventarioMaestro.push_back(nuevoProyecto);
                        SetWindowTextW(pEstado->hwndBusqueda, nuevoProyecto.nombreCarpeta.c_str());
                        aplicarFiltro(pEstado, nuevoProyecto.nombreCarpeta);

                        if (ListView_GetItemCount(pEstado->hwndListaProyectos) > 0) {
                            ListView_SetItemState(
                                pEstado->hwndListaProyectos, 0,
                                LVIS_SELECTED | LVIS_FOCUSED,
                                LVIS_SELECTED | LVIS_FOCUSED
                            );
                            ListView_EnsureVisible(pEstado->hwndListaProyectos, 0, FALSE);
                            seleccionarProyecto(pEstado, &pEstado->inventarioMaestro.back());
                        }

                        if (pEstado->hwndStatusBar) {
                            SendMessageW(pEstado->hwndStatusBar, SB_SETTEXTW, 0,
                                         reinterpret_cast<LPARAM>(L"✓ Proyecto creado exitosamente en el servidor."));
                        }
                    }
                }
                return 0;
            }

            return 0;
        }

        case WM_SIZE: {
            int anchoCliente = LOWORD(lParam);
            int altoCliente  = HIWORD(lParam);

            if (anchoCliente == 0 || altoCliente == 0) {
                return 0;
            }

            SendMessageW(g_hwndStatusBar, WM_SIZE, 0, 0);
            RECT rcStatus = { 0 };
            GetWindowRect(g_hwndStatusBar, &rcStatus);
            int altoStatus = rcStatus.bottom - rcStatus.top;
            if (altoStatus <= 0) altoStatus = 23;

            constexpr int margen = 12;
            constexpr int altoBarraSuperior = 28;
            constexpr int anchoLabelBuscar = 110;
            constexpr int anchoBtnRefrescar = 110;

            int yTop = margen;
            int xSearchEdit = margen + anchoLabelBuscar + 6;
            int xBtnRefrescar = anchoCliente - margen - anchoBtnRefrescar;
            int anchoSearchEdit = xBtnRefrescar - 8 - xSearchEdit;

            int yWorkArea = yTop + altoBarraSuperior + margen;
            int altoWorkArea = altoCliente - altoStatus - margen - yWorkArea;

            int anchoDisponible = anchoCliente - (3 * margen);
            int anchoPanelIzq = static_cast<int>(anchoDisponible * 0.63);
            int xPanelDer = margen + anchoPanelIzq + margen;
            int anchoPanelDer = anchoCliente - margen - xPanelDer;

            HDWP hdwp = BeginDeferWindowPos(14);
            if (hdwp) {
                hdwp = DeferWindowPos(hdwp, g_hwndStaticBuscar, nullptr,
                                      margen, yTop + 5, anchoLabelBuscar, 20,
                                      SWP_NOZORDER | SWP_NOACTIVATE);

                hdwp = DeferWindowPos(hdwp, g_hwndSearchEdit, nullptr,
                                      xSearchEdit, yTop, anchoSearchEdit, altoBarraSuperior,
                                      SWP_NOZORDER | SWP_NOACTIVATE);

                hdwp = DeferWindowPos(hdwp, g_hwndBtnRefrescar, nullptr,
                                      xBtnRefrescar, yTop, anchoBtnRefrescar, altoBarraSuperior,
                                      SWP_NOZORDER | SWP_NOACTIVATE);

                hdwp = DeferWindowPos(hdwp, g_hwndListProyectos, nullptr,
                                      margen, yWorkArea, anchoPanelIzq, altoWorkArea,
                                      SWP_NOZORDER | SWP_NOACTIVATE);

                int yRight = yWorkArea;
                hdwp = DeferWindowPos(hdwp, g_hwndStaticDetalles, nullptr,
                                      xPanelDer, yRight, anchoPanelDer, 20,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
                yRight += 24;

                hdwp = DeferWindowPos(hdwp, g_hwndStaticUNC, nullptr,
                                      xPanelDer, yRight, anchoPanelDer, 16,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
                yRight += 18;

                hdwp = DeferWindowPos(hdwp, g_hwndEditRutaUNC, nullptr,
                                      xPanelDer, yRight, anchoPanelDer, 24,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
                yRight += 30;

                hdwp = DeferWindowPos(hdwp, g_hwndStaticLNX, nullptr,
                                      xPanelDer, yRight, anchoPanelDer, 16,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
                yRight += 18;

                hdwp = DeferWindowPos(hdwp, g_hwndEditRutaLnx, nullptr,
                                      xPanelDer, yRight, anchoPanelDer, 24,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
                yRight += 32;

                int anchoBtnAccion = (anchoPanelDer - 12) / 3;
                hdwp = DeferWindowPos(hdwp, g_hwndBtnAbrirCarpeta, nullptr,
                                      xPanelDer, yRight, anchoBtnAccion, 28,
                                      SWP_NOZORDER | SWP_NOACTIVATE);

                hdwp = DeferWindowPos(hdwp, g_hwndBtnCopiarUNC, nullptr,
                                      xPanelDer + anchoBtnAccion + 6, yRight, anchoBtnAccion, 28,
                                      SWP_NOZORDER | SWP_NOACTIVATE);

                hdwp = DeferWindowPos(hdwp, g_hwndBtnCopiarLnx, nullptr,
                                      xPanelDer + 2 * (anchoBtnAccion + 6), yRight,
                                      anchoPanelDer - 2 * (anchoBtnAccion + 6), 28,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
                yRight += 36;

                hdwp = DeferWindowPos(hdwp, g_hwndStaticEntregables, nullptr,
                                      xPanelDer, yRight, anchoPanelDer, 18,
                                      SWP_NOZORDER | SWP_NOACTIVATE);
                yRight += 22;

                constexpr int altoBtnNuevo = 34;
                int yBtnNuevo = yWorkArea + altoWorkArea - altoBtnNuevo;
                hdwp = DeferWindowPos(hdwp, g_hwndBtnNuevo, nullptr,
                                      xPanelDer, yBtnNuevo, anchoPanelDer, altoBtnNuevo,
                                      SWP_NOZORDER | SWP_NOACTIVATE);

                int altoListBox = yBtnNuevo - 8 - yRight;
                if (altoListBox < 60) altoListBox = 60;

                hdwp = DeferWindowPos(hdwp, g_hwndListEntregables, nullptr,
                                      xPanelDer, yRight, anchoPanelDer, altoListBox,
                                      SWP_NOZORDER | SWP_NOACTIVATE);

                EndDeferWindowPos(hdwp);
            }

            return 0;
        }

        case WM_GETMINMAXINFO: {
            LPMINMAXINFO lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
            lpMMI->ptMinTrackSize.x = 900;
            lpMMI->ptMinTrackSize.y = 550;
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = reinterpret_cast<HDC>(wParam);
            SetBkMode(hdcStatic, TRANSPARENT);
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
        }

        case WM_CLOSE: {
            if (pEstado) {
                // Si el hilo está leyendo SMB, notificar al usuario en la barra de estado
                if (pEstado->escaneoEnProgreso.load()) {
                    if (g_hwndStatusBar) {
                        SendMessageW(g_hwndStatusBar, SB_SETTEXTW, 0,
                                     reinterpret_cast<LPARAM>(L"Cancelando operaciones de red y cerrando..."));
                    }
                }

                // Detener el hilo cooperativamente y hacer join() ANTES de destruir controles
                pEstado->detenerHiloEscaneo();
            }

            // Proceder con la destrucción natural de la ventana
            DestroyWindow(hwnd);
            return 0;
        }

        case WM_DESTROY: {
            if (pEstado) {
                // Sincronización final defensiva
                pEstado->detenerHiloEscaneo();
                delete pEstado;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            }

            destruirFuentesSistema();
            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace GUI
