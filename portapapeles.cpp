#include "portapapeles.hpp"
#include <cwchar>

namespace PortapapelesMC {

bool copiarTexto(HWND hwndOwner, const std::wstring& texto) {
    if (texto.empty()) {
        return false;
    }

    // 1. Intentar abrir el portapapeles con reintentos breves (tolerancia ante aplicaciones externas)
    constexpr int MAX_INTENTOS = 5;
    bool abierto = false;
    for (int i = 0; i < MAX_INTENTOS; ++i) {
        if (OpenClipboard(hwndOwner)) {
            abierto = true;
            break;
        }
        Sleep(10);
    }

    if (!abierto) {
        return false;
    }

    // 2. Vaciar el contenido actual
    if (!EmptyClipboard()) {
        CloseClipboard();
        return false;
    }

    // 3. Reservar memoria global móvil para la cadena UTF-16 con terminador nulo
    const size_t bytes = (texto.length() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!hMem) {
        CloseClipboard();
        return false;
    }

    // 4. Bloquear y copiar los datos
    wchar_t* pDestino = reinterpret_cast<wchar_t*>(GlobalLock(hMem));
    if (!pDestino) {
        GlobalFree(hMem);
        CloseClipboard();
        return false;
    }

    std::wmemcpy(pDestino, texto.c_str(), texto.length() + 1);
    GlobalUnlock(hMem);

    // 5. Transferir posesión del objeto al portapapeles de Windows
    if (!SetClipboardData(CF_UNICODETEXT, hMem)) {
        GlobalFree(hMem);
        CloseClipboard();
        return false;
    }

    // 6. Cerrar el portapapeles
    CloseClipboard();
    return true;
}

} // namespace PortapapelesMC
