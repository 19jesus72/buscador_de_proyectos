#include <iostream>
#include <string>
#include <cassert>
#include <iomanip>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

#include "sanitizador.hpp"

namespace Color {
    const wchar_t* VERDE    = L"\033[92m";
    const wchar_t* ROJO     = L"\033[91m";
    const wchar_t* AZUL     = L"\033[94m";
    const wchar_t* CIAN     = L"\033[96m";
    const wchar_t* AMARILLO = L"\033[93m";
    const wchar_t* RESET    = L"\033[0m";
    const wchar_t* BOLD     = L"\033[1m";
}

void imprimir_linea() {
    std::wcout << L"--------------------------------------------------------------------------------\n";
}

int main() {
#ifdef _WIN32
    // Activar modo UTF-16 para la consola nativa de Windows
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    // Habilitar códigos de escape ANSI
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif

    std::wcout << Color::BOLD << Color::CIAN;
    std::wcout << L"================================================================================\n";
    std::wcout << L"   MASTER CONTROL - MOTOR DE DIRECTORIO Y GOBERNANZA DE PROYECTOS\n";
    std::wcout << L"   ARNÉS DE PRUEBAS UNITARIAS: FASE 1 - PASO 2 (SANITIZACIÓN Y GOBERNANZA)\n";
    std::wcout << L"================================================================================\n" << Color::RESET;

    int exitosas = 0;
    const int totalPruebas = 9;

    auto verificar = [&](int num, const std::wstring& descripcion,
                         const std::wstring& obtenido, const std::wstring& esperado) {
        std::wcout << L"\n" << Color::BOLD << L"[CASO " << num << L"/9] " << descripcion << Color::RESET << L"\n";
        std::wcout << L"  * Obtenido: \"" << Color::AMARILLO << obtenido << Color::RESET << L"\" (Longitud: " << obtenido.length() << L")\n";
        std::wcout << L"  * Esperado: \"" << Color::AZUL << esperado << Color::RESET << L"\"\n";

        bool ok = (obtenido == esperado);
        assert(ok);

        if (ok) {
            std::wcout << L"  [" << Color::VERDE << L"PASADA" << Color::RESET << L"] Transformación correcta.\n";
            exitosas++;
        } else {
            std::wcout << L"  [" << Color::ROJO << L"FALLÓ" << Color::RESET << L"] Discrepancia detectada.\n";
        }
    };

    // CASO 1: Tildes y eñes
    verificar(1, L"Mapeo de tildes, diéresis y eñes a ASCII mayúsculas",
              NormalizadorMC::sanitizarCadena(L"Dirección Técnica de Operación - Año 2026"),
              L"DIRECCION_TECNICA_DE_OPERACION_ANO_2026");

    // CASO 2: Caracteres prohibidos en sistemas de archivos
    verificar(2, L"Sustitución de caracteres ilegales y colapso de guiones bajos",
              NormalizadorMC::sanitizarCadena(L"Cliente: Pro&Gas / Tanque 5000 gl <Norte>"),
              L"CLIENTE_PRO_GAS_TANQUE_5000_GL_NORTE");

    // CASO 3: Centro de Costo numérico simple
    verificar(3, L"Normalización de Centro de Costo entero simple",
              NormalizadorMC::formatearCC(L"4322"),
              L"CC4322");

    // CASO 4: Centro de Costo decimal
    verificar(4, L"Preservación de puntos decimales en Centro de Costo",
              NormalizadorMC::formatearCC(L"4.01"),
              L"CC4.01");

    // CASO 5: Centro de Costo preexistente o en minúsculas
    verificar(5, L"Conversión a mayúsculas y no duplicación de prefijo CC",
              NormalizadorMC::formatearCC(L"cc2520"),
              L"CC2520");

    // CASO 6: Centro de Costo vacío o solo espacios
    verificar(6, L"Asignación segura de 'S_CC' ante entrada vacía",
              NormalizadorMC::formatearCC(L"   "),
              L"S_CC");

    // CASO 7: Ensamblado completo estándar
    verificar(7, L"Ensamblado completo [CC]_[CLIENTE]_[TIPO]",
              NormalizadorMC::construirNombreProyecto(L"4322", L"Pampite Artz Distrito Vivo", L"Obra Nueva"),
              L"CC4322_PAMPITE_ARTZ_DISTRITO_VIVO_OBRA_NUEVA");

    // CASO 8: Ensamblado sin tipo de trabajo
    verificar(8, L"Ensamblado sin tipo de trabajo [CC]_[CLIENTE]",
              NormalizadorMC::construirNombreProyecto(L"2457", L"Ceviches de la 10", L""),
              L"CC2457_CEVICHES_DE_LA_10");

    // CASO 9: Truncado preventivo a 100 caracteres (Blindaje MAX_PATH)
    {
        std::wcout << L"\n" << Color::BOLD << L"[CASO 9/9] Truncado preventivo a 100 caracteres (Blindaje MAX_PATH)..." << Color::RESET << L"\n";
        const std::wstring rawClienteLargo =
            L"Empresa Constructora de Infraestructuras y Gas Licuado de Petroleo del Austro Ecuatoriano Sociedad Anonima de Responsabilidad Limitada";
        
        std::wstring resultadoLargo = NormalizadorMC::construirNombreProyecto(L"4322", rawClienteLargo, L"Auditoria Integral");
        
        std::wcout << L"  * Longitud resultante : " << resultadoLargo.length() << L" caracteres\n";
        std::wcout << L"  * Cadena resultante   : \"" << Color::AMARILLO << resultadoLargo << Color::RESET << L"\"\n";
        
        bool okLongitud = (resultadoLargo.length() <= 100);
        bool okPrefijo = (resultadoLargo.rfind(L"CC4322_", 0) == 0);
        bool okSinGuionFinal = (!resultadoLargo.empty() && resultadoLargo.back() != L'_');

        assert(okLongitud && okPrefijo && okSinGuionFinal);
        if (okLongitud && okPrefijo && okSinGuionFinal) {
            std::wcout << L"  [" << Color::VERDE << L"PASADA" << Color::RESET << L"] Truncado efectivo a <= 100 caracteres sin terminar en guion bajo residual.\n";
            exitosas++;
        } else {
            std::wcout << L"  [" << Color::ROJO << L"FALLÓ" << Color::RESET << L"] Error en el control de longitud máxima.\n";
        }
    }

    // -------------------------------------------------------------------------
    // RESUMEN GLOBAL
    // -------------------------------------------------------------------------
    imprimir_linea();
    std::wcout << Color::BOLD;
    if (exitosas == totalPruebas) {
        std::wcout << Color::VERDE << L"RESULTADO FINAL: TODAS LAS PRUEBAS (" << exitosas << L"/" << totalPruebas << L") FUERON SUPERADAS CON ÉXITO.\n";
    } else {
        std::wcout << Color::ROJO << L"RESULTADO FINAL: ALGUNAS PRUEBAS FALLARON (" << exitosas << L"/" << totalPruebas << L").\n";
    }
    std::wcout << Color::RESET;
    imprimir_linea();

    return (exitosas == totalPruebas) ? 0 : 1;
}
