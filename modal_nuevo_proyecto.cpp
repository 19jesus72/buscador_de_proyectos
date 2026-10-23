#include "modal_nuevo_proyecto.hpp"
#include "diagnostico_red.hpp"
#include "resource.h"
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace ModalMC {

namespace {

constexpr const wchar_t* CLASE_MODAL_CREAR = L"MasterControl_ModalCrear";

struct ContextoModal {
    HWND hwndModal               = nullptr;
    HWND hwndPadre               = nullptr;
    GUI::EstadoAplicacion* estado = nullptr;
    MC::Proyecto* pOutProyecto   = nullptr;
    bool proyectoCreado          = false;

    HFONT hFontNormal            = nullptr;
    HFONT hFontBold              = nullptr;

    HWND hwndCBCat               = nullptr;
    HWND hwndCBSub               = nullptr;
    HWND hwndEditCC              = nullptr;
    HWND hwndEditCliente         = nullptr;
    HWND hwndEditTipo            = nullptr;
    HWND hwndLblPrevNombre       = nullptr;
    HWND hwndLblPrevLongitud     = nullptr;
    HWND hwndLblPrevUNC          = nullptr;
    HWND hwndBtnCrear            = nullptr;
    HWND hwndBtnCancelar         = nullptr;
};

std::wstring obtenerTextoCombo(HWND hwndCombo) {
    int idx = static_cast<int>(SendMessageW(hwndCombo, CB_GETCURSEL, 0, 0));
    if (idx == CB_ERR) return L"";
    int len = static_cast<int>(SendMessageW(hwndCombo, CB_GETLBTEXTLEN, idx, 0));
    if (len <= 0) return L"";
    std::wstring texto(len, L'\0');
    SendMessageW(hwndCombo, CB_GETLBTEXT, idx, reinterpret_cast<LPARAM>(&texto[0]));
    return texto;
}

std::wstring obtenerTextoEdit(HWND hwndEdit) {
    int len = GetWindowTextLengthW(hwndEdit);
    if (len <= 0) return L"";
    std::wstring texto(len + 1, L'\0');
    GetWindowTextW(hwndEdit, &texto[0], len + 1);
    texto.resize(len);
    return texto;
}

void repoblarSubdivisiones(ContextoModal* ctx) {
    if (!ctx || !ctx->hwndCBCat || !ctx->hwndCBSub) return;

    SendMessageW(ctx->hwndCBSub, CB_RESETCONTENT, 0, 0);

    int idxCat = static_cast<int>(SendMessageW(ctx->hwndCBCat, CB_GETCURSEL, 0, 0));

    if (idxCat == 2) {
        // 03_CONTRATOS_MARCO_RECURRENTES_(CC_1.XX_Y_11XX)
        SendMessageW(ctx->hwndCBSub, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"ENI_ECUADOR"));
        SendMessageW(ctx->hwndCBSub, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"VEPAGAS"));
        SendMessageW(ctx->hwndCBSub, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"OTROS"));
        SendMessageW(ctx->hwndCBSub, CB_SETCURSEL, 0, 0);
    } else if (idxCat == 5) {
        // 06_VENTA_MATERIALES_(CC_9.XX)
        SendMessageW(ctx->hwndCBSub, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"2024_2026"));
        SendMessageW(ctx->hwndCBSub, CB_SETCURSEL, 0, 0);
    } else {
        // Obras, Mantenimientos, Licitaciones, Propuestas
        SendMessageW(ctx->hwndCBSub, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"2026"));
        SendMessageW(ctx->hwndCBSub, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"2025"));
        SendMessageW(ctx->hwndCBSub, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"2024"));
        SendMessageW(ctx->hwndCBSub, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"2023"));
        SendMessageW(ctx->hwndCBSub, CB_SETCURSEL, 0, 0);
    }
}

void actualizarPrevisualizacion(ContextoModal* ctx) {
    if (!ctx) return;

    std::wstring cat = obtenerTextoCombo(ctx->hwndCBCat);
    std::wstring sub = obtenerTextoCombo(ctx->hwndCBSub);
    std::wstring rawCC = obtenerTextoEdit(ctx->hwndEditCC);
    std::wstring rawCliente = obtenerTextoEdit(ctx->hwndEditCliente);
    std::wstring rawTipo = obtenerTextoEdit(ctx->hwndEditTipo);

    std::wstring nombreNormalizado = NormalizadorMC::construirNombreProyecto(rawCC, rawCliente, rawTipo);
    SetWindowTextW(ctx->hwndLblPrevNombre, nombreNormalizado.c_str());

    size_t len = nombreNormalizado.length();
    std::wstring txtLen;
    if (len >= 100) {
        txtLen = L"Longitud: 100 / 100 caracteres (¡LÍMITE MÁXIMO ALCANZADO!)";
    } else {
        txtLen = L"Longitud: " + std::to_wstring(len) + L" / 100 caracteres (Regla MAX_PATH)";
    }
    SetWindowTextW(ctx->hwndLblPrevLongitud, txtLen.c_str());

    fs::path rutaUNC = fs::path(ctx->estado->rutaBaseUNC) / cat / sub / nombreNormalizado;
    SetWindowTextW(ctx->hwndLblPrevUNC, rutaUNC.wstring().c_str());

    // Habilitar creación solo si el cliente tiene contenido alfanumérico válido
    std::wstring clienteSanitizado = NormalizadorMC::sanitizarCadena(rawCliente);
    bool permitirCreacion = !clienteSanitizado.empty() && !cat.empty() && !sub.empty();
    EnableWindow(ctx->hwndBtnCrear, permitirCreacion ? TRUE : FALSE);
}

LRESULT CALLBACK WndProcModal(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW* pCS = reinterpret_cast<CREATESTRUCTW*>(lParam);
            ContextoModal* ctx = reinterpret_cast<ContextoModal*>(pCS->lpCreateParams);
            ctx->hwndModal = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));

            ctx->hFontNormal = CreateFontW(
                -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );
            ctx->hFontBold = CreateFontW(
                -12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );

            // Cargar y aplicar iconos corporativos en la ventana modal
            HICON hIconBig = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
            HICON hIconSmall = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

            if (hIconBig) {
                SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIconBig));
            }
            if (hIconSmall) {
                SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconSmall));
            }

            auto aplicarFmt = [ctx](HWND h, bool bold = false) {
                SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(bold ? ctx->hFontBold : ctx->hFontNormal), TRUE);
            };

            // 1. Selector de Categoría
            HWND lbl1 = CreateWindowExW(0, L"STATIC", L"1. Línea de Negocio Oficial (Categoría):",
                WS_CHILD | WS_VISIBLE, 20, 15, 300, 16, hwnd, nullptr, pCS->hInstance, nullptr);
            aplicarFmt(lbl1, true);

            ctx->hwndCBCat = CreateWindowExW(0, L"COMBOBOX", L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                20, 35, 565, 200, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MODAL_CB_CATEGORIA)),
                pCS->hInstance, nullptr);
            aplicarFmt(ctx->hwndCBCat);

            for (const auto& cat : MC::LINEAS_NEGOCIO_OFICIALES) {
                SendMessageW(ctx->hwndCBCat, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(cat.c_str()));
            }
            SendMessageW(ctx->hwndCBCat, CB_SETCURSEL, 0, 0);

            // 2. Selector de Subdivisión
            HWND lbl2 = CreateWindowExW(0, L"STATIC", L"2. Período / Año o Contrato Marco:",
                WS_CHILD | WS_VISIBLE, 20, 72, 300, 16, hwnd, nullptr, pCS->hInstance, nullptr);
            aplicarFmt(lbl2, true);

            ctx->hwndCBSub = CreateWindowExW(0, L"COMBOBOX", L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                20, 92, 565, 150, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MODAL_CB_SUBDIVISION)),
                pCS->hInstance, nullptr);
            aplicarFmt(ctx->hwndCBSub);
            repoblarSubdivisiones(ctx);

            // 3. Entradas de datos
            HWND lblCC = CreateWindowExW(0, L"STATIC", L"Centro Costo (CC):",
                WS_CHILD | WS_VISIBLE, 20, 130, 130, 16, hwnd, nullptr, pCS->hInstance, nullptr);
            aplicarFmt(lblCC, true);

            ctx->hwndEditCC = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
                20, 150, 130, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MODAL_EDIT_CC)),
                pCS->hInstance, nullptr);
            aplicarFmt(ctx->hwndEditCC);
            SendMessageW(ctx->hwndEditCC, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Ej. 4322"));

            HWND lblCli = CreateWindowExW(0, L"STATIC", L"Nombre del Cliente o Proyecto:",
                WS_CHILD | WS_VISIBLE, 165, 130, 260, 16, hwnd, nullptr, pCS->hInstance, nullptr);
            aplicarFmt(lblCli, true);

            ctx->hwndEditCliente = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
                165, 150, 260, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MODAL_EDIT_CLIENTE)),
                pCS->hInstance, nullptr);
            aplicarFmt(ctx->hwndEditCliente);
            SendMessageW(ctx->hwndEditCliente, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Ej. PAMPITE ARTZ"));

            HWND lblTipo = CreateWindowExW(0, L"STATIC", L"Tipo de Trabajo (Opcional):",
                WS_CHILD | WS_VISIBLE, 440, 130, 145, 16, hwnd, nullptr, pCS->hInstance, nullptr);
            aplicarFmt(lblTipo, true);

            ctx->hwndEditTipo = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
                440, 150, 145, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MODAL_EDIT_TIPO)),
                pCS->hInstance, nullptr);
            aplicarFmt(ctx->hwndEditTipo);
            SendMessageW(ctx->hwndEditTipo, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Ej. OBRA NUEVA"));

            // 4. Panel de Previsualización y Reglas de Gobernanza
            HWND grpPrev = CreateWindowExW(0, L"BUTTON", L" Previsualización Oficial de Gobernanza (Estructura Plana) ",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                20, 190, 565, 185, hwnd, nullptr, pCS->hInstance, nullptr);
            aplicarFmt(grpPrev, true);

            HWND lblPn = CreateWindowExW(0, L"STATIC", L"Nombre de Carpeta Resultante (Punto de Anclaje):",
                WS_CHILD | WS_VISIBLE, 35, 215, 530, 16, hwnd, nullptr, pCS->hInstance, nullptr);
            aplicarFmt(lblPn, false);

            ctx->hwndLblPrevNombre = CreateWindowExW(WS_EX_STATICEDGE, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                35, 235, 535, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MODAL_LBL_PREV_NOMBRE)),
                pCS->hInstance, nullptr);
            aplicarFmt(ctx->hwndLblPrevNombre, true);

            ctx->hwndLblPrevLongitud = CreateWindowExW(0, L"STATIC", L"Longitud: 0 / 100 caracteres",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                35, 265, 535, 16, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MODAL_LBL_PREV_LONGITUD)),
                pCS->hInstance, nullptr);
            aplicarFmt(ctx->hwndLblPrevLongitud, true);

            HWND lblPu = CreateWindowExW(0, L"STATIC", L"Ruta física completa en el servidor SMB:",
                WS_CHILD | WS_VISIBLE, 35, 290, 530, 16, hwnd, nullptr, pCS->hInstance, nullptr);
            aplicarFmt(lblPu, false);

            ctx->hwndLblPrevUNC = CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                35, 310, 535, 50, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MODAL_LBL_PREV_UNC)),
                pCS->hInstance, nullptr);
            aplicarFmt(ctx->hwndLblPrevUNC, false);

            // 5. Botones de Acción
            ctx->hwndBtnCrear = CreateWindowExW(0, L"BUTTON", L"✓ Crear en Servidor",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_DISABLED | WS_TABSTOP,
                325, 395, 160, 34, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MODAL_BTN_CREAR)),
                pCS->hInstance, nullptr);
            aplicarFmt(ctx->hwndBtnCrear, true);

            ctx->hwndBtnCancelar = CreateWindowExW(0, L"BUTTON", L"Cancelar",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                495, 395, 90, 34, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MODAL_BTN_CANCELAR)),
                pCS->hInstance, nullptr);
            aplicarFmt(ctx->hwndBtnCancelar, false);

            actualizarPrevisualizacion(ctx);
            return 0;
        }

        case WM_COMMAND: {
            ContextoModal* ctx = reinterpret_cast<ContextoModal*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA)
            );
            if (!ctx) break;

            WORD controlId = LOWORD(wParam);
            WORD notificacion = HIWORD(wParam);

            // Cascada de ComboBox de Categoría
            if (controlId == IDC_MODAL_CB_CATEGORIA && notificacion == CBN_SELCHANGE) {
                repoblarSubdivisiones(ctx);
                actualizarPrevisualizacion(ctx);
                return 0;
            }

            if (controlId == IDC_MODAL_CB_SUBDIVISION && notificacion == CBN_SELCHANGE) {
                actualizarPrevisualizacion(ctx);
                return 0;
            }

            // Cambios de texto en tiempo real
            if ((controlId == IDC_MODAL_EDIT_CC ||
                 controlId == IDC_MODAL_EDIT_CLIENTE ||
                 controlId == IDC_MODAL_EDIT_TIPO) && notificacion == EN_CHANGE) {
                actualizarPrevisualizacion(ctx);
                return 0;
            }

            // Botón Crear
            if (controlId == IDC_MODAL_BTN_CREAR && notificacion == BN_CLICKED) {
                // Sondeo previo ultrarrápido de conectividad de red SMB (< 200 ms)
                RedMC::EstadoSonda sonda = RedMC::verificarAccesoRecurso(ctx->estado->rutaBaseUNC);
                if (!sonda.accesible) {
                    std::wstring msg = sonda.mensajeDiagnostico + L"\n\nAcción sugerida: " + sonda.accionSugerida;
                    MessageBoxW(
                        hwnd,
                        msg.c_str(),
                        L"Fallo de Conexión SMB",
                        MB_ICONERROR | MB_OK
                    );
                    return 0;
                }

                std::wstring cat = obtenerTextoCombo(ctx->hwndCBCat);
                std::wstring sub = obtenerTextoCombo(ctx->hwndCBSub);
                std::wstring rawCC = obtenerTextoEdit(ctx->hwndEditCC);
                std::wstring rawCli = obtenerTextoEdit(ctx->hwndEditCliente);
                std::wstring rawTip = obtenerTextoEdit(ctx->hwndEditTipo);

                std::wstring nombreNormalizado = NormalizadorMC::construirNombreProyecto(rawCC, rawCli, rawTip);

                std::wstring outRutaCreada;
                std::wstring outError;

                bool creado = GestorMC::crearProyectoPlano(
                    ctx->estado->rutaBaseUNC,
                    cat,
                    sub,
                    nombreNormalizado,
                    outRutaCreada,
                    outError
                );

                if (!creado) {
                    MessageBoxW(
                        hwnd,
                        outError.c_str(),
                        L"Error de Creación — Master Control",
                        MB_ICONERROR | MB_OK
                    );
                    return 0;
                }

                // Asignar los datos del proyecto creado
                if (ctx->pOutProyecto) {
                    ctx->pOutProyecto->nombreCarpeta = nombreNormalizado;
                    ctx->pOutProyecto->categoria = cat;
                    ctx->pOutProyecto->subdivision = sub;
                    ctx->pOutProyecto->rutaUNC = outRutaCreada;
                    ctx->pOutProyecto->rutaLinux = MC::unc_a_linux(outRutaCreada, ctx->estado->rutaBaseUNC);
                }

                ctx->proyectoCreado = true;
                DestroyWindow(hwnd);
                return 0;
            }

            // Botón Cancelar
            if (controlId == IDC_MODAL_BTN_CANCELAR && notificacion == BN_CLICKED) {
                DestroyWindow(hwnd);
                return 0;
            }

            break;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = reinterpret_cast<HDC>(wParam);
            SetBkMode(hdcStatic, TRANSPARENT);
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
        }

        case WM_CLOSE: {
            DestroyWindow(hwnd);
            return 0;
        }

        case WM_DESTROY: {
            ContextoModal* ctx = reinterpret_cast<ContextoModal*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA)
            );
            if (ctx) {
                if (ctx->hFontNormal) DeleteObject(ctx->hFontNormal);
                if (ctx->hFontBold) DeleteObject(ctx->hFontBold);
            }
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

} // namespace anónimo

bool mostrarDialogoCreacion(
    HWND hwndPadre,
    GUI::EstadoAplicacion* estado,
    MC::Proyecto& outProyectoCreado) {

    if (!estado) return false;

    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwndPadre, GWLP_HINSTANCE));

    // Registrar clase modal si no existe
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    if (!GetClassInfoExW(hInstance, CLASE_MODAL_CREAR, &wc)) {
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
        wc.lpfnWndProc   = WndProcModal;
        wc.lpszClassName = CLASE_MODAL_CREAR;
        wc.lpszMenuName  = nullptr;
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        RegisterClassExW(&wc);
    }

    constexpr int anchoModal = 620;
    constexpr int altoModal  = 485;

    RECT rcPadre = { 0 };
    GetWindowRect(hwndPadre, &rcPadre);
    int xPos = rcPadre.left + ((rcPadre.right - rcPadre.left) - anchoModal) / 2;
    int yPos = rcPadre.top  + ((rcPadre.bottom - rcPadre.top) - altoModal) / 2;

    ContextoModal ctx;
    ctx.hwndPadre = hwndPadre;
    ctx.estado = estado;
    ctx.pOutProyecto = &outProyectoCreado;
    ctx.proyectoCreado = false;

    HWND hwndModal = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        CLASE_MODAL_CREAR,
        L"Master Control — Alta Estandarizada de Proyecto (Estructura Plana)",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        xPos, yPos,
        anchoModal, altoModal,
        hwndPadre,
        nullptr,
        hInstance,
        &ctx
    );

    if (!hwndModal) {
        return false;
    }

    // Modalidad estricta Win32: Deshabilitar ventana padre
    EnableWindow(hwndPadre, FALSE);
    ShowWindow(hwndModal, SW_SHOW);
    UpdateWindow(hwndModal);

    // Bucle modal de mensajes aislado
    MSG msg;
    while (IsWindow(hwndModal) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hwndModal, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // Rehabilitar ventana padre y devolver el foco
    EnableWindow(hwndPadre, TRUE);
    SetActiveWindow(hwndPadre);
    SetForegroundWindow(hwndPadre);

    return ctx.proyectoCreado;
}

} // namespace ModalMC
