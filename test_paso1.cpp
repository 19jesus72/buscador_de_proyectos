#include <iostream>
#include <vector>
#include <cassert>
#include <iomanip>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

#include "modelo_proyecto.hpp"
#include "mapeo_rutas.hpp"

// Colores ANSI para salida en consola (compatibles con Windows 10/11)
namespace Color {
    const wchar_t* VERDE  = L"\033[92m";
    const wchar_t* ROJO   = L"\033[91m";
    const wchar_t* AZUL   = L"\033[94m";
    const wchar_t* CIAN   = L"\033[96m";
    const wchar_t* AMARILLO = L"\033[93m";
    const wchar_t* RESET  = L"\033[0m";
    const wchar_t* BOLD   = L"\033[1m";
}

void imprimir_separador() {
    std::wcout << L"--------------------------------------------------------------------------------\n";
}

int main() {
#ifdef _WIN32
    // Habilitar modo UTF-16 para la consola nativa de Windows
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);
    
    // Activar procesamiento de secuencias de escape virtuales ANSI
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
    std::wcout << L"   ARNÉS DE PRUEBAS UNITARIAS: FASE 1 - PASO 1 (MODELO Y MAPEO DE RUTAS)\n";
    std::wcout << L"================================================================================\n" << Color::RESET;

    int pruebasPasadas = 0;
    const int totalPruebas = 5;

    // -------------------------------------------------------------------------
    // PRUEBA 1: Traducción de ruta UNC a Linux con subdirectorios profundos
    // -------------------------------------------------------------------------
    std::wcout << L"\n" << Color::BOLD << L"[PRUEBA 1/5] Traducción de ruta UNC a Linux (Ruta Profunda)..." << Color::RESET << L"\n";
    {
        const std::wstring rutaUNC =
            L"\\\\192.168.1.242\\mcontrol\\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS\\"
            L"01_OBRAS_E_INSTALACIONES_(CC_4.XX_Y_4XXX)\\2026\\CC4322_PAMPITE_ARTZ_OBRA_NUEVA\\PLANO_GENERAL.dwg";

        const std::wstring esperadoLinux =
            L"/home/mcontrol/RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS/"
            L"01_OBRAS_E_INSTALACIONES_(CC_4.XX_Y_4XXX)/2026/CC4322_PAMPITE_ARTZ_OBRA_NUEVA/PLANO_GENERAL.dwg";

        std::wstring resultadoLinux = MC::unc_a_linux(rutaUNC);

        std::wcout << L"  * Entrada UNC    : " << rutaUNC << L"\n";
        std::wcout << L"  * Resultado Linux: " << resultadoLinux << L"\n";

        assert(resultadoLinux == esperadoLinux);
        if (resultadoLinux == esperadoLinux) {
            std::wcout << L"  [" << Color::VERDE << L"OK - PASADA" << Color::RESET << L"] Coincidencia exacta con la ruta esperada de ClearOS 7.\n";
            pruebasPasadas++;
        } else {
            std::wcout << L"  [" << Color::ROJO << L"FALLO" << Color::RESET << L"] Esperado: " << esperadoLinux << L"\n";
        }
    }

    // -------------------------------------------------------------------------
    // PRUEBA 2: Traducción inversa (Linux a UNC)
    // -------------------------------------------------------------------------
    std::wcout << L"\n" << Color::BOLD << L"[PRUEBA 2/5] Traducción Inversa: Linux POSIX a Windows UNC..." << Color::RESET << L"\n";
    {
        const std::wstring rutaLinux =
            L"/home/mcontrol/RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS/"
            L"03_CONTRATOS_MARCO_RECURRENTES_(CC_1.XX_Y_11XX)/ENI_ECUADOR/CC1105_ENI_MANTENIMIENTO_TERMINAL";

        const std::wstring esperadoUNC =
            L"\\\\192.168.1.242\\mcontrol\\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS\\"
            L"03_CONTRATOS_MARCO_RECURRENTES_(CC_1.XX_Y_11XX)\\ENI_ECUADOR\\CC1105_ENI_MANTENIMIENTO_TERMINAL";

        std::wstring resultadoUNC = MC::linux_a_unc(rutaLinux);

        std::wcout << L"  * Entrada Linux  : " << rutaLinux << L"\n";
        std::wcout << L"  * Resultado UNC  : " << resultadoUNC << L"\n";

        assert(resultadoUNC == esperadoUNC);
        if (resultadoUNC == esperadoUNC) {
            std::wcout << L"  [" << Color::VERDE << L"OK - PASADA" << Color::RESET << L"] Coincidencia exacta con la ruta UNC sobre SMB.\n";
            pruebasPasadas++;
        } else {
            std::wcout << L"  [" << Color::ROJO << L"FALLO" << Color::RESET << L"] Esperado: " << esperadoUNC << L"\n";
        }
    }

    // -------------------------------------------------------------------------
    // PRUEBA 3: Integridad de ida y vuelta (Round-Trip)
    // -------------------------------------------------------------------------
    std::wcout << L"\n" << Color::BOLD << L"[PRUEBA 3/5] Integridad de Ida y Vuelta: linux_a_unc(unc_a_linux(x)) == x..." << Color::RESET << L"\n";
    {
        const std::wstring rutaOriginal =
            L"\\\\192.168.1.242\\mcontrol\\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS\\"
            L"02_MANTENIMIENTOS_Y_SERVICIOS_(CC_2.XX_Y_2XXX)\\2025\\CC2088_REPSOL_CALIBRACION_VALVULAS";

        std::wstring enLinux = MC::unc_a_linux(rutaOriginal);
        std::wstring vueltaUNC = MC::linux_a_unc(enLinux);

        std::wcout << L"  * Original UNC : " << rutaOriginal << L"\n";
        std::wcout << L"  * Paso a Linux : " << enLinux << L"\n";
        std::wcout << L"  * Regreso a UNC: " << vueltaUNC << L"\n";

        assert(vueltaUNC == rutaOriginal);
        if (vueltaUNC == rutaOriginal) {
            std::wcout << L"  [" << Color::VERDE << L"OK - PASADA" << Color::RESET << L"] La transformación es 100% simétrica y sin pérdida de datos.\n";
            pruebasPasadas++;
        } else {
            std::wcout << L"  [" << Color::ROJO << L"FALLO" << Color::RESET << L"] La ruta de retorno difiere de la original.\n";
        }
    }

    // -------------------------------------------------------------------------
    // PRUEBA 4: Manejo de rutas fuera del repositorio base (Error controlado)
    // -------------------------------------------------------------------------
    std::wcout << L"\n" << Color::BOLD << L"[PRUEBA 4/5] Manejo de Rutas Foráneas / Inválidas (Seguridad)..." << Color::RESET << L"\n";
    {
        const std::wstring rutaLocalInvalida = L"C:\\Users\\Admin\\Desktop\\Proyecto_Local";
        const std::wstring rutaOtroServidor = L"\\\\10.0.0.50\\compartido\\datos";
        const std::wstring rutaLinuxInvalida = L"/var/www/html/sistema_web";

        std::wstring res1 = MC::unc_a_linux(rutaLocalInvalida);
        std::wstring res2 = MC::unc_a_linux(rutaOtroServidor);
        std::wstring res3 = MC::linux_a_unc(rutaLinuxInvalida);

        bool errorControlado = res1.empty() && res2.empty() && res3.empty();

        std::wcout << L"  * Intento 'C:\\Users\\...'     -> Retorno: \"" << res1 << L"\" (Esperado: vacio)\n";
        std::wcout << L"  * Intento '\\\\10.0.0.50\\...'    -> Retorno: \"" << res2 << L"\" (Esperado: vacio)\n";
        std::wcout << L"  * Intento '/var/www/...'      -> Retorno: \"" << res3 << L"\" (Esperado: vacio)\n";

        assert(errorControlado);
        if (errorControlado) {
            std::wcout << L"  [" << Color::VERDE << L"OK - PASADA" << Color::RESET << L"] Todas las rutas fuera de la raíz corporativa fueron rechazadas de forma segura.\n";
            pruebasPasadas++;
        } else {
            std::wcout << L"  [" << Color::ROJO << L"FALLO" << Color::RESET << L"] No se rechazaron adecuadamente las rutas no autorizadas.\n";
        }
    }

    // -------------------------------------------------------------------------
    // PRUEBA 5: Instanciación de std::vector<Proyecto> con datos de prueba
    // -------------------------------------------------------------------------
    std::wcout << L"\n" << Color::BOLD << L"[PRUEBA 5/5] Instanciación y Formato del Modelo de Dominio (Proyecto)..." << Color::RESET << L"\n";
    {
        std::vector<MC::Proyecto> inventario;

        // Simulamos el poblamiento de registros que vendrán del motor de escaneo SMB
        const struct Muestra {
            std::wstring nombre;
            std::wstring cat;
            std::wstring sub;
        } muestras[] = {
            { L"CC4322_PAMPITE_ARTZ_OBRA_NUEVA", L"01_OBRAS_E_INSTALACIONES_(CC_4.XX_Y_4XXX)", L"2026" },
            { L"CC2100_VEPAGAS_MANT_CORRECTIVO", L"02_MANTENIMIENTOS_Y_SERVICIOS_(CC_2.XX_Y_2XXX)", L"2025" },
            { L"CC1102_ENI_MONTECRISTI_ESFERAS", L"03_CONTRATOS_MARCO_RECURRENTES_(CC_1.XX_Y_11XX)", L"ENI_ECUADOR" }
        };

        for (const auto& m : muestras) {
            std::wstring unc = std::wstring(MC::BASE_UNC_DEFAULT) + L"\\" + m.cat + L"\\" + m.sub + L"\\" + m.nombre;
            std::wstring lnx = MC::unc_a_linux(unc);
            inventario.emplace_back(m.nombre, m.cat, m.sub, std::move(unc), std::move(lnx));
        }

        imprimir_separador();
        std::wcout << Color::BOLD << std::left
                   << std::setw(35) << L"PROYECTO (NOMBRE CARPETA)"
                   << std::setw(15) << L"SUBDIVISION"
                   << L"VALIDO?" << Color::RESET << L"\n";
        imprimir_separador();

        bool todosValidos = true;
        for (const auto& p : inventario) {
            todosValidos = todosValidos && p.esValido();
            std::wcout << std::left
                       << std::setw(35) << p.nombreCarpeta
                       << std::setw(15) << p.subdivision
                       << (p.esValido() ? Color::VERDE : Color::ROJO)
                       << (p.esValido() ? L"SI" : L"NO") << Color::RESET << L"\n";
            std::wcout << Color::AZUL << L"  UNC  : " << Color::RESET << p.rutaUNC << L"\n";
            std::wcout << Color::AMARILLO << L"  Linux: " << Color::RESET << p.rutaLinux << L"\n\n";
        }
        imprimir_separador();

        assert(inventario.size() == 3 && todosValidos);
        if (inventario.size() == 3 && todosValidos) {
            std::wcout << L"  [" << Color::VERDE << L"OK - PASADA" << Color::RESET << L"] Colección std::vector<Proyecto> gestionada con éxito en memoria.\n";
            pruebasPasadas++;
        } else {
            std::wcout << L"  [" << Color::ROJO << L"FALLO" << Color::RESET << L"] Falló la instanciación del modelo de dominio.\n";
        }
    }

    // -------------------------------------------------------------------------
    // RESUMEN GLOBAL DE EJECUCIÓN
    // -------------------------------------------------------------------------
    imprimir_separador();
    std::wcout << Color::BOLD;
    if (pruebasPasadas == totalPruebas) {
        std::wcout << Color::VERDE << L"RESULTADO FINAL: TODAS LAS PRUEBAS (" << pruebasPasadas << L"/" << totalPruebas << L") FUERON SUPERADAS CON ÉXITO.\n";
    } else {
        std::wcout << Color::ROJO << L"RESULTADO FINAL: ALGUNAS PRUEBAS FALLARON (" << pruebasPasadas << L"/" << totalPruebas << L").\n";
    }
    std::wcout << Color::RESET;
    imprimir_separador();

    return (pruebasPasadas == totalPruebas) ? 0 : 1;
}
