#include "sanitizador.hpp"

#include <cwctype>
#include <vector>

namespace NormalizadorMC {

namespace {

/**
 * @brief Convierte caracteres acentuados, diéresis y la eñe a su equivalente ASCII en mayúsculas.
 * @param ch Carácter ancho de entrada.
 * @return wchar_t Carácter ASCII normalizado, o L'\0' si no es un carácter con acentuación especial.
 */
inline wchar_t mapearAcentoASCII(wchar_t ch) noexcept {
    switch (ch) {
        // Variantes de A
        case L'Á': case L'À': case L'Ä': case L'Â':
        case L'á': case L'à': case L'ä': case L'â':
            return L'A';

        // Variantes de E
        case L'É': case L'È': case L'Ë': case L'Ê':
        case L'é': case L'è': case L'ë': case L'ê':
            return L'E';

        // Variantes de I
        case L'Í': case L'Ì': case L'Ï': case L'Î':
        case L'í': case L'ì': case L'ï': case L'î':
            return L'I';

        // Variantes de O
        case L'Ó': case L'Ò': case L'Ö': case L'Ô':
        case L'ó': case L'ò': case L'ö': case L'ô':
            return L'O';

        // Variantes de U
        case L'Ú': case L'Ù': case L'Ü': case L'Û':
        case L'ú': case L'ù': case L'ü': case L'û':
            return L'U';

        // Eñe
        case L'Ñ': case L'ñ':
            return L'N';

        default:
            return L'\0';
    }
}

} // namespace anónimo

std::wstring sanitizarCadena(const std::wstring& entrada) {
    if (entrada.empty()) {
        return L"";
    }

    std::wstring acumulador;
    acumulador.reserve(entrada.length());

    for (wchar_t ch : entrada) {
        // 1. Mapeo de acentos y eñes a ASCII
        wchar_t normAcento = mapearAcentoASCII(ch);
        wchar_t transformado = L'_';

        if (normAcento != L'\0') {
            transformado = normAcento;
        } else if (ch >= L'a' && ch <= L'z') {
            transformado = static_cast<wchar_t>(ch - L'a' + L'A');
        } else if ((ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9')) {
            transformado = ch;
        } else {
            // Todo carácter no alfanumérico (espacios, guiones, símbolos prohibidos)
            // se convierte en guion bajo.
            transformado = L'_';
        }

        // 2. Colapso de secuencias de guiones bajos repetidos y trim inicial
        if (transformado == L'_') {
            if (!acumulador.empty() && acumulador.back() != L'_') {
                acumulador.push_back(L'_');
            }
        } else {
            acumulador.push_back(transformado);
        }
    }

    // 3. Trim final de guiones bajos residuales
    while (!acumulador.empty() && acumulador.back() == L'_') {
        acumulador.pop_back();
    }

    return acumulador;
}

std::wstring formatearCC(const std::wstring& rawCC) {
    if (rawCC.empty()) {
        return L"S_CC";
    }

    std::wstring limpio;
    limpio.reserve(rawCC.length() + 2);

    bool contieneAlfanumerico = false;

    for (wchar_t ch : rawCC) {
        if (ch >= L'a' && ch <= L'z') {
            limpio.push_back(static_cast<wchar_t>(ch - L'a' + L'A'));
            contieneAlfanumerico = true;
        } else if ((ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9')) {
            limpio.push_back(ch);
            contieneAlfanumerico = true;
        } else if (ch == L'.') {
            // Permitir puntos decimales (ej. CC4.01 o CC2.50)
            limpio.push_back(L'.');
        }
    }

    // Si está vacío o solo contiene caracteres no alfanuméricos (ej. solo puntos o espacios)
    if (limpio.empty() || !contieneAlfanumerico) {
        return L"S_CC";
    }

    // Si ya inicia con la subcadena "CC", se retorna tal cual en mayúsculas
    if (limpio.rfind(L"CC", 0) == 0) {
        return limpio;
    }

    // De lo contrario, anteponer el prefijo "CC"
    return L"CC" + limpio;
}

std::wstring construirNombreProyecto(
    const std::wstring& rawCC,
    const std::wstring& rawCliente,
    const std::wstring& rawTipo) {

    std::wstring cc = formatearCC(rawCC);
    std::wstring cliente = sanitizarCadena(rawCliente);
    std::wstring tipo = sanitizarCadena(rawTipo);

    if (cliente.empty()) {
        cliente = L"PROYECTO";
    }

    // Fórmula: [PREFIJO_CC]_[CLIENTE]_[TIPO] o [PREFIJO_CC]_[CLIENTE]
    std::wstring resultado = cc + L"_" + cliente;

    if (!tipo.empty()) {
        resultado += L"_" + tipo;
    }

    // Control de longitud máxima preventiva para blindar contra MAX_PATH (100 caracteres)
    constexpr size_t LIMITE_LONGITUD = 100;
    if (resultado.length() > LIMITE_LONGITUD) {
        resultado = resultado.substr(0, LIMITE_LONGITUD);
        // Recortar guion bajo residual si el corte ocurrió justo en uno
        while (!resultado.empty() && resultado.back() == L'_') {
            resultado.pop_back();
        }
    }

    return resultado;
}

} // namespace NormalizadorMC
