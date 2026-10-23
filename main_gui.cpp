#include "ventana_principal.hpp"
#include "mapeo_rutas.hpp"
#include <string>

/**
 * @file main_gui.cpp
 * @brief Punto de entrada wWinMain para el aplicativo gráfico nativo de Master Control.
 * @author Master Control — Ingeniería y Proyectos GLP
 * @version 1.0 (Fase 2 - Paso 2)
 */

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/,
    LPWSTR    lpCmdLine,
    int       nCmdShow) {

    // 1. Inicializar Controles Comunes Modernos (ComCtl32 v6)
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX) };
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // 2. Registrar la clase de ventana nativa
    if (!GUI::registrarClaseVentana(hInstance)) {
        MessageBoxW(
            nullptr,
            L"No se pudo registrar la clase de ventana de la aplicación.",
            L"Error Crítico — Master Control",
            MB_ICONERROR | MB_OK
        );
        return 1;
    }

    // 3. Evaluar ruta base por línea de comandos o asignar la corporativa predeterminada
    std::wstring rutaBaseUNC = MC::BASE_UNC_DEFAULT;
    if (lpCmdLine && *lpCmdLine) {
        // Quitar comillas si el argumento viene entrecomillado
        std::wstring argStr = lpCmdLine;
        while (!argStr.empty() && (argStr.front() == L' ' || argStr.front() == L'"')) {
            argStr.erase(argStr.begin());
        }
        while (!argStr.empty() && (argStr.back() == L' ' || argStr.back() == L'"')) {
            argStr.pop_back();
        }
        if (!argStr.empty()) {
            rutaBaseUNC = argStr;
        }
    }

    // 4. Instanciar la ventana principal con sus controles y cargar inventario
    HWND hwndPrincipal = GUI::crearVentanaPrincipal(hInstance, nCmdShow, rutaBaseUNC);
    if (!hwndPrincipal) {
        MessageBoxW(
            nullptr,
            L"Fallo en la creación de la ventana principal nativa.",
            L"Error Crítico — Master Control",
            MB_ICONERROR | MB_OK
        );
        return 1;
    }

    // 5. Bucle principal de mensajes de Windows
    MSG msg = { nullptr };
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
