#include "mapeo_rutas.hpp"

#include <cwctype>
#include <algorithm>
#include <vector>

namespace MC {

std::wstring normalizar_separadores(const std::wstring& ruta, bool formatoWindows) {
    if (ruta.empty()) {
        return L"";
    }

    std::wstring resultado;
    resultado.reserve(ruta.length() + 2);

    const wchar_t sep = formatoWindows ? L'\\' : L'/';
    size_t i = 0;
    const size_t n = ruta.length();

    if (formatoWindows) {
        // Detectar si la ruta inicia con prefijo de red UNC (\\ o //)
        bool esUNC = (n >= 2 && (ruta[0] == L'\\' || ruta[0] == L'/') &&
                                (ruta[1] == L'\\' || ruta[1] == L'/'));

        if (esUNC) {
            resultado.append(L"\\\\");
            // Saltar barras iniciales adicionales si las hubiera
            i = 2;
            while (i < n && (ruta[i] == L'\\' || ruta[i] == L'/')) {
                ++i;
            }
        } else if (ruta[0] == L'\\' || ruta[0] == L'/') {
            resultado.push_back(L'\\');
            i = 1;
            while (i < n && (ruta[i] == L'\\' || ruta[i] == L'/')) {
                ++i;
            }
        }

        for (; i < n; ++i) {
            wchar_t c = ruta[i];
            if (c == L'\\' || c == L'/') {
                if (resultado.empty() || resultado.back() != L'\\') {
                    resultado.push_back(L'\\');
                }
            } else {
                resultado.push_back(c);
            }
        }

        // Eliminar barra final redundante (respetando si solo es "\\")
        while (resultado.length() > 2 && resultado.back() == L'\\') {
            resultado.pop_back();
        }
    } else {
        // Formato POSIX / Linux
        if (ruta[0] == L'/' || ruta[0] == L'\\') {
            resultado.push_back(L'/');
            i = 1;
            while (i < n && (ruta[i] == L'/' || ruta[i] == L'\\')) {
                ++i;
            }
        }

        for (; i < n; ++i) {
            wchar_t c = ruta[i];
            if (c == L'/' || c == L'\\') {
                if (resultado.empty() || resultado.back() != L'/') {
                    resultado.push_back(L'/');
                }
            } else {
                resultado.push_back(c);
            }
        }

        // Eliminar barra final redundante (respetando si es la raíz "/")
        while (resultado.length() > 1 && resultado.back() == L'/') {
            resultado.pop_back();
        }
    }

    return resultado;
}

std::wstring unc_a_linux(const std::wstring& rutaUNC,
                         const std::wstring& baseUNC,
                         const std::wstring& baseLinux) {
    if (rutaUNC.empty()) {
        return L"";
    }

    std::wstring normRuta = normalizar_separadores(rutaUNC, true);
    std::wstring normBaseUNC = normalizar_separadores(baseUNC, true);
    std::wstring normBaseLinux = normalizar_separadores(baseLinux, false);

    if (normRuta.length() < normBaseUNC.length()) {
        return L"";
    }

    // Validación case-insensitive de prefijo UNC de Windows
    for (size_t i = 0; i < normBaseUNC.length(); ++i) {
        if (std::towupper(normRuta[i]) != std::towupper(normBaseUNC[i])) {
            return L"";
        }
    }

    // Verificar que no sea coincidencia de prefijo parcial (ej. BASE_UNC_EXTRA)
    if (normRuta.length() > normBaseUNC.length() && normRuta[normBaseUNC.length()] != L'\\') {
        return L"";
    }

    // Caso: la ruta es exactamente la raíz base
    if (normRuta.length() == normBaseUNC.length()) {
        return normBaseLinux;
    }

    // Extraer camino relativo preservando el casing original de los subdirectorios
    std::wstring relativo = normRuta.substr(normBaseUNC.length() + 1);

    // Convertir separadores de Windows a Linux
    for (wchar_t& ch : relativo) {
        if (ch == L'\\') {
            ch = L'/';
        }
    }

    std::wstring resultado = normBaseLinux + L"/" + relativo;
    return normalizar_separadores(resultado, false);
}

std::wstring linux_a_unc(const std::wstring& rutaLinux,
                         const std::wstring& baseLinux,
                         const std::wstring& baseUNC) {
    if (rutaLinux.empty()) {
        return L"";
    }

    std::wstring normRuta = normalizar_separadores(rutaLinux, false);
    std::wstring normBaseLinux = normalizar_separadores(baseLinux, false);
    std::wstring normBaseUNC = normalizar_separadores(baseUNC, true);

    if (normRuta.length() < normBaseLinux.length()) {
        return L"";
    }

    // Validación exacta (case-sensitive) para rutas en servidor Linux
    if (normRuta.compare(0, normBaseLinux.length(), normBaseLinux) != 0) {
        return L"";
    }

    // Verificar que no sea coincidencia de prefijo parcial de directorio
    if (normRuta.length() > normBaseLinux.length() && normRuta[normBaseLinux.length()] != L'/') {
        return L"";
    }

    // Caso: la ruta es exactamente la raíz base
    if (normRuta.length() == normBaseLinux.length()) {
        return normBaseUNC;
    }

    // Extraer camino relativo
    std::wstring relativo = normRuta.substr(normBaseLinux.length() + 1);

    // Convertir separadores de Linux a Windows
    for (wchar_t& ch : relativo) {
        if (ch == L'/') {
            ch = L'\\';
        }
    }

    std::wstring resultado = normBaseUNC + L"\\" + relativo;
    return normalizar_separadores(resultado, true);
}

} // namespace MC
