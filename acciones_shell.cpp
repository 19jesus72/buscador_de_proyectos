#include "acciones_shell.hpp"
#include <shellapi.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace ShellMC {

bool abrirCarpetaEnExplorador(HWND hwndParent, const std::wstring& rutaUNC) {
    if (rutaUNC.empty()) {
        MessageBoxW(
            hwndParent,
            L"No se ha seleccionado ningún proyecto o la ruta está vacía.",
            L"Ruta no disponible — Master Control",
            MB_ICONWARNING | MB_OK
        );
        return false;
    }

    // Validación de accesibilidad en red SMB
    std::error_code ec;
    if (!fs::exists(fs::path(rutaUNC), ec)) {
        std::wstring mensaje = L"No se puede acceder a la ubicación en el servidor de red:\n\n" +
                               rutaUNC +
                               L"\n\nVerifique la conexión SMB con el servidor 192.168.1.242.";
        MessageBoxW(
            hwndParent,
            mensaje.c_str(),
            L"Recurso no accesible — Master Control",
            MB_ICONERROR | MB_OK
        );
        return false;
    }

    // Delegación directa a explorer.exe mediante el Shell de Windows
    HINSTANCE hResult = ShellExecuteW(
        hwndParent,
        L"open",
        rutaUNC.c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );

    INT_PTR resultado = reinterpret_cast<INT_PTR>(hResult);
    if (resultado <= 32) {
        std::wstring msgError = L"El Shell de Windows no pudo abrir la carpeta seleccionada.\nCódigo de error: " +
                                std::to_wstring(resultado);
        MessageBoxW(
            hwndParent,
            msgError.c_str(),
            L"Error al abrir carpeta — Master Control",
            MB_ICONERROR | MB_OK
        );
        return false;
    }

    return true;
}

bool abrirArchivoConAsociado(HWND hwndParent, const std::wstring& rutaArchivoCompleta) {
    if (rutaArchivoCompleta.empty()) {
        return false;
    }

    std::error_code ec;
    if (!fs::exists(fs::path(rutaArchivoCompleta), ec)) {
        std::wstring mensaje = L"El archivo seleccionado no se encuentra en el servidor:\n\n" +
                               rutaArchivoCompleta;
        MessageBoxW(
            hwndParent,
            mensaje.c_str(),
            L"Archivo no encontrado — Master Control",
            MB_ICONWARNING | MB_OK
        );
        return false;
    }

    HINSTANCE hResult = ShellExecuteW(
        hwndParent,
        L"open",
        rutaArchivoCompleta.c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );

    INT_PTR resultado = reinterpret_cast<INT_PTR>(hResult);
    if (resultado <= 32) {
        std::wstring msgError = L"No se pudo ejecutar el archivo o no hay ningún programa asociado.\n\nArchivo: " +
                                rutaArchivoCompleta +
                                L"\nCódigo de error: " + std::to_wstring(resultado);
        MessageBoxW(
            hwndParent,
            msgError.c_str(),
            L"Asociación de archivo no disponible",
            MB_ICONINFORMATION | MB_OK
        );
        return false;
    }

    return true;
}

} // namespace ShellMC
