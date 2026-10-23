#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cwctype>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <shellapi.h>
#endif

#include "modelo.hpp"
#include "mapeo_rutas.hpp"
#include "sanitizador.hpp"
#include "indexador.hpp"
#include "gestor_proyectos.hpp"

namespace fs = std::filesystem;

namespace Color {
    const wchar_t* VERDE    = L"\033[92m";
    const wchar_t* ROJO     = L"\033[91m";
    const wchar_t* AZUL     = L"\033[94m";
    const wchar_t* CIAN     = L"\033[96m";
    const wchar_t* AMARILLO = L"\033[93m";
    const wchar_t* BLANCO   = L"\033[97m";
    const wchar_t* RESET    = L"\033[0m";
    const wchar_t* BOLD     = L"\033[1m";
}

// Utilitario para leer una línea limpia de la consola
std::wstring leerLinea() {
    std::wstring linea;
    std::getline(std::wcin, linea);
    // Eliminar retornos de carro si existieran
    while (!linea.empty() && (linea.back() == L'\r' || linea.back() == L'\n')) {
        linea.pop_back();
    }
    return linea;
}

// Convertir cadena a mayúsculas para búsquedas insensibles
std::wstring aMayusculas(const std::wstring& str) {
    std::wstring res;
    res.reserve(str.length());
    for (wchar_t ch : str) {
        res.push_back(std::towupper(ch));
    }
    return res;
}

void pausar() {
    std::wcout << L"\nPresione " << Color::AMARILLO << L"[ENTER]" << Color::RESET << L" para continuar...";
    leerLinea();
}

void imprimirBanner(const std::wstring& rutaBase, size_t totalIndexados) {
    std::wcout << Color::BOLD << Color::CIAN;
    std::wcout << L"======================================================================\n";
    std::wcout << L"    MASTER CONTROL - GOBERNANZA Y DIRECTORIO DE PROYECTOS (CLI)       \n";
    std::wcout << L"======================================================================\n" << Color::RESET;
    std::wcout << Color::BLANCO << L" Recurso actual: " << Color::AMARILLO << rutaBase << Color::RESET << L"\n";
    std::wcout << Color::BLANCO << L" Proyectos en RAM: " << Color::VERDE << totalIndexados << Color::RESET << L"\n";
    std::wcout << L"----------------------------------------------------------------------\n";
    std::wcout << L" 1. Indexar / Re-escanear repositorio en servidor SMB\n";
    std::wcout << L" 2. Buscar proyectos en memoria (Filtro instantáneo CC/Cliente)\n";
    std::wcout << L" 3. Inspeccionar entregables de un proyecto específico\n";
    std::wcout << L" 4. Crear nuevo proyecto estandarizado (Alta física plana)\n";
    std::wcout << L" 5. Ejecutar suite de validaciones automáticas (Self-Test)\n";
    std::wcout << L" 6. Salir\n";
    std::wcout << Color::BOLD << Color::CIAN;
    std::wcout << L"======================================================================\n" << Color::RESET;
    std::wcout << Color::BOLD << L"Seleccione una opción [1-6]: " << Color::RESET;
}

// Ejecuta el Self-Test de la Opción 5
void ejecutarSelfTest() {
    std::wcout << Color::BOLD << Color::CIAN;
    std::wcout << L"\n--- INICIANDO SUITE DE VALIDACIONES AUTOMÁTICAS (SELF-TEST) ---\n" << Color::RESET;
    int pasadas = 0;
    const int total = 5;

    // 1. Sanitización de tildes
    {
        std::wstring s = NormalizadorMC::sanitizarCadena(L"Operación & Mant. - Cañería 2026");
        if (s == L"OPERACION_MANT_CANERIA_2026") {
            std::wcout << L" [1/5] Sanitización básica: [" << Color::VERDE << L"OK" << Color::RESET << L"]\n";
            pasadas++;
        } else {
            std::wcout << L" [1/5] Sanitización básica: [" << Color::ROJO << L"FALLÓ" << Color::RESET << L"] (" << s << L")\n";
        }
    }

    // 2. Normalización de CC
    {
        bool cc1 = (NormalizadorMC::formatearCC(L"4322") == L"CC4322");
        bool cc2 = (NormalizadorMC::formatearCC(L"4.01") == L"CC4.01");
        bool cc3 = (NormalizadorMC::formatearCC(L"cc1105") == L"CC1105");
        bool cc4 = (NormalizadorMC::formatearCC(L"   ") == L"S_CC");
        if (cc1 && cc2 && cc3 && cc4) {
            std::wcout << L" [2/5] Normalización de Centro de Costos: [" << Color::VERDE << L"OK" << Color::RESET << L"]\n";
            pasadas++;
        } else {
            std::wcout << L" [2/5] Normalización de Centro de Costos: [" << Color::ROJO << L"FALLÓ" << Color::RESET << L"]\n";
        }
    }

    // 3. Blindaje MAX_PATH (100 caracteres)
    {
        std::wstring largo = NormalizadorMC::construirNombreProyecto(
            L"4322",
            L"Cliente con denominación exageradamente extensa para sobrepasar el límite de cien caracteres",
            L"Estudio de Factibilidad"
        );
        if (largo.length() <= 100 && !largo.empty() && largo.back() != L'_') {
            std::wcout << L" [3/5] Blindaje MAX_PATH (<= 100 chars): [" << Color::VERDE << L"OK" << Color::RESET << L"] (" << largo.length() << L" chars)\n";
            pasadas++;
        } else {
            std::wcout << L" [3/5] Blindaje MAX_PATH: [" << Color::ROJO << L"FALLÓ" << Color::RESET << L"]\n";
        }
    }

    // 4. Mapeo bidireccional UNC <-> Linux
    {
        std::wstring unc = std::wstring(MC::BASE_UNC_DEFAULT) + L"\\01_OBRAS_E_INSTALACIONES_(CC_4.XX_Y_4XXX)\\2026\\CC4322_TEST";
        std::wstring lnx = MC::unc_a_linux(unc);
        std::wstring ret = MC::linux_a_unc(lnx);
        if (ret == unc && lnx.rfind(L"/home/mcontrol", 0) == 0) {
            std::wcout << L" [4/5] Mapeo bidireccional UNC <-> POSIX: [" << Color::VERDE << L"OK" << Color::RESET << L"]\n";
            pasadas++;
        } else {
            std::wcout << L" [4/5] Mapeo bidireccional: [" << Color::ROJO << L"FALLÓ" << Color::RESET << L"]\n";
        }
    }

    // 5. Rechazo de rutas foráneas
    {
        std::wstring f1 = MC::unc_a_linux(L"C:\\Windows\\System32");
        std::wstring f2 = MC::linux_a_unc(L"/etc/passwd");
        if (f1.empty() && f2.empty()) {
            std::wcout << L" [5/5] Seguridad de rechazo a rutas foráneas: [" << Color::VERDE << L"OK" << Color::RESET << L"]\n";
            pasadas++;
        } else {
            std::wcout << L" [5/5] Seguridad de rutas: [" << Color::ROJO << L"FALLÓ" << Color::RESET << L"]\n";
        }
    }

    std::wcout << L"----------------------------------------------------------------------\n";
    if (pasadas == total) {
        std::wcout << Color::VERDE << Color::BOLD << L"RESULTADO: 100% DE VALIDACIONES SUPERADAS (" << pasadas << L"/" << total << L").\n" << Color::RESET;
    } else {
        std::wcout << Color::ROJO << Color::BOLD << L"RESULTADO: SE DETECTARON ERRORES (" << pasadas << L"/" << total << L").\n" << Color::RESET;
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Habilitar modo UTF-16 para E/S nativa de Windows
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);
    _setmode(_fileno(stdin),  _O_U16TEXT);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif

    // Determinación de la ruta base UNC por argumento o predeterminada
    std::wstring rutaBaseUNC = MC::BASE_UNC_DEFAULT;
    if (argc > 1) {
        std::string argStr = argv[1];
        rutaBaseUNC = std::wstring(argStr.begin(), argStr.end());
    }

    std::vector<MC::Proyecto> inventarioMemoria;
    std::vector<MC::Proyecto> ultimosResultados;

    bool ejecutando = true;

    while (ejecutando) {
        imprimirBanner(rutaBaseUNC, inventarioMemoria.size());
        std::wstring opcionStr = leerLinea();

        if (opcionStr == L"1") {
            // =================================================================
            // OPCIÓN 1: Indexar / Re-escanear repositorio SMB
            // =================================================================
            std::wcout << L"\nIniciando escaneo superficial de 3 niveles sobre " << Color::AMARILLO << rutaBaseUNC << Color::RESET << L"...\n";
            auto tInicio = std::chrono::high_resolution_clock::now();

            inventarioMemoria = MC::indexarRepositorio(rutaBaseUNC);

            auto tFin = std::chrono::high_resolution_clock::now();
            auto duracionMs = std::chrono::duration_cast<std::chrono::milliseconds>(tFin - tInicio).count();

            std::wcout << Color::VERDE << L"✓ Escaneo completado en " << duracionMs << L" ms.\n" << Color::RESET;
            std::wcout << L"Total de proyectos indexados en RAM: " << Color::BOLD << inventarioMemoria.size() << Color::RESET << L"\n\n";

            // Desglose por categoría oficial
            std::wcout << Color::BOLD << L"Desglose por Línea de Negocio:\n" << Color::RESET;
            for (size_t i = 0; i < MC::LINEAS_NEGOCIO_OFICIALES.size(); ++i) {
                const auto& catOficial = MC::LINEAS_NEGOCIO_OFICIALES[i];
                size_t conteo = 0;
                for (const auto& p : inventarioMemoria) {
                    if (p.categoria == catOficial) {
                        conteo++;
                    }
                }
                std::wcout << L"  [" << (i + 1) << L"] " << std::left << std::setw(50) << catOficial
                           << L": " << Color::AMARILLO << conteo << Color::RESET << L" proyectos\n";
            }
            pausar();

        } else if (opcionStr == L"2") {
            // =================================================================
            // OPCIÓN 2: Buscar proyectos en memoria RAM
            // =================================================================
            if (inventarioMemoria.empty()) {
                std::wcout << Color::AMARILLO << L"\n[AVISO] El inventario en memoria está vacío. Ejecute primero la Opción 1.\n" << Color::RESET;
                pausar();
                continue;
            }

            std::wcout << L"\nIngrese término de búsqueda (Centro de Costo o Cliente): ";
            std::wstring termino = leerLinea();
            std::wstring terminoMayus = aMayusculas(termino);

            ultimosResultados.clear();
            auto tInicio = std::chrono::high_resolution_clock::now();

            for (const auto& p : inventarioMemoria) {
                if (aMayusculas(p.nombreCarpeta).find(terminoMayus) != std::wstring::npos ||
                    aMayusculas(p.categoria).find(terminoMayus) != std::wstring::npos ||
                    aMayusculas(p.subdivision).find(terminoMayus) != std::wstring::npos) {
                    ultimosResultados.push_back(p);
                }
            }

            auto tFin = std::chrono::high_resolution_clock::now();
            auto duracionMicro = std::chrono::duration_cast<std::chrono::microseconds>(tFin - tInicio).count();

            std::wcout << L"\nResultados encontrados: " << Color::VERDE << ultimosResultados.size()
                       << Color::RESET << L" (Búsqueda en " << duracionMicro << L" µs)\n";
            std::wcout << L"----------------------------------------------------------------------\n";

            for (size_t i = 0; i < ultimosResultados.size(); ++i) {
                const auto& p = ultimosResultados[i];
                std::wcout << Color::BOLD << Color::CIAN << L"#" << (i + 1) << L" - " << p.nombreCarpeta << Color::RESET << L"\n";
                std::wcout << L"  Categoría   : " << p.categoria << L"\n";
                std::wcout << L"  Subdivisión : " << p.subdivision << L"\n";
                std::wcout << L"  Ruta Windows: " << Color::AZUL << p.rutaUNC << Color::RESET << L"\n";
                std::wcout << L"  Ruta Linux  : " << Color::AMARILLO << p.rutaLinux << Color::RESET << L"\n\n";
            }
            pausar();

        } else if (opcionStr == L"3") {
            // =================================================================
            // OPCIÓN 3: Inspeccionar entregables de un proyecto (Lazy Loading)
            // =================================================================
            std::wstring rutaSeleccionada;

            if (!ultimosResultados.empty()) {
                std::wcout << L"\n¿Desea seleccionar un proyecto de la última búsqueda por número [1-"
                           << ultimosResultados.size() << L"], o ingresar una ruta directa? (Escriba el número o la ruta): ";
                std::wstring entrada = leerLinea();

                try {
                    size_t idx = std::stoul(entrada);
                    if (idx >= 1 && idx <= ultimosResultados.size()) {
                        rutaSeleccionada = ultimosResultados[idx - 1].rutaUNC;
                    } else {
                        rutaSeleccionada = entrada;
                    }
                } catch (...) {
                    rutaSeleccionada = entrada;
                }
            } else {
                std::wcout << L"\nIngrese la ruta UNC o el nombre de la carpeta del proyecto a inspeccionar: ";
                rutaSeleccionada = leerLinea();
            }

            if (rutaSeleccionada.empty()) {
                std::wcout << Color::ROJO << L"Ruta no válida.\n" << Color::RESET;
                pausar();
                continue;
            }

            std::wcout << L"\nInspeccionando entregables en: " << Color::AMARILLO << rutaSeleccionada << Color::RESET << L"...\n";
            auto archivos = MC::listarEntregables(rutaSeleccionada);

            if (archivos.empty()) {
                std::wcout << Color::AMARILLO << L"No se detectaron archivos entregables o el directorio no existe.\n" << Color::RESET;
            } else {
                std::wcout << Color::VERDE << L"Entregables detectados: " << archivos.size() << Color::RESET << L"\n";
                std::wcout << L"--------------------------------------------------------------------------------\n";
                std::wcout << Color::BOLD << std::left
                           << std::setw(40) << L"NOMBRE DE ARCHIVO"
                           << std::setw(12) << L"EXTENSIÓN"
                           << std::setw(16) << L"TAMAÑO (BYTES)"
                           << L"TAMAÑO (MB)" << Color::RESET << L"\n";
                std::wcout << L"--------------------------------------------------------------------------------\n";

                for (const auto& a : archivos) {
                    std::wcout << std::left
                               << std::setw(40) << a.nombre
                               << std::setw(12) << a.extension
                               << std::setw(16) << a.tamanioBytes
                               << std::fixed << std::setprecision(2) << a.tamanioMB << L" MB\n";
                }
                std::wcout << L"--------------------------------------------------------------------------------\n";
            }
            pausar();

        } else if (opcionStr == L"4") {
            // =================================================================
            // OPCIÓN 4: Crear nuevo proyecto estandarizado (Alta física plana)
            // =================================================================
            std::wcout << Color::BOLD << Color::CIAN;
            std::wcout << L"\n--- ASISTENTE DE CREACIÓN DE PROYECTO (ESTRUCTURA PLANA) ---\n" << Color::RESET;

            std::wcout << L"Seleccione la Línea de Negocio:\n";
            for (size_t i = 0; i < MC::LINEAS_NEGOCIO_OFICIALES.size(); ++i) {
                std::wcout << L"  " << (i + 1) << L". " << MC::LINEAS_NEGOCIO_OFICIALES[i] << L"\n";
            }
            std::wcout << L"Opción [1-6]: ";
            std::wstring catOpStr = leerLinea();
            size_t catIdx = 0;
            try {
                catIdx = std::stoul(catOpStr);
            } catch (...) {
                catIdx = 0;
            }

            if (catIdx < 1 || catIdx > MC::LINEAS_NEGOCIO_OFICIALES.size()) {
                std::wcout << Color::ROJO << L"Selección inválida.\n" << Color::RESET;
                pausar();
                continue;
            }

            std::wstring categoriaElegida = MC::LINEAS_NEGOCIO_OFICIALES[catIdx - 1];
            std::wstring subdivisionSugerida = L"2026";

            if (catIdx == 3) {
                std::wcout << L"\nSeleccione Subdivisión de Contrato Marco:\n";
                std::wcout << L"  1. ENI_ECUADOR\n  2. VEPAGAS\n  3. OTROS\nOpción [1-3]: ";
                std::wstring subOp = leerLinea();
                if (subOp == L"1") subdivisionSugerida = L"ENI_ECUADOR";
                else if (subOp == L"2") subdivisionSugerida = L"VEPAGAS";
                else subdivisionSugerida = L"OTROS";
            } else if (catIdx == 6) {
                subdivisionSugerida = L"2024_2026";
            }

            std::wcout << L"Subdivisión / Año [" << Color::AMARILLO << subdivisionSugerida << Color::RESET << L"] (ENTER para aceptar o escriba): ";
            std::wstring subEntrada = leerLinea();
            if (!subEntrada.empty()) {
                subdivisionSugerida = subEntrada;
            }

            std::wcout << L"Centro de Costo (ej. 4322 o 4.01): ";
            std::wstring rawCC = leerLinea();

            std::wcout << L"Nombre del Cliente o Denominación: ";
            std::wstring rawCliente = leerLinea();

            std::wcout << L"Tipo de Trabajo (Opcional, ej. Obra Nueva, Mantenimiento): ";
            std::wstring rawTipo = leerLinea();

            // Sanitización y gobernanza oficial
            std::wstring nombreNormalizado = NormalizadorMC::construirNombreProyecto(rawCC, rawCliente, rawTipo);

            std::wcout << L"\n" << Color::BOLD << L"PREVISUALIZACIÓN DE GOBERNANZA:" << Color::RESET << L"\n";
            std::wcout << L"  * Nombre resultante : " << Color::VERDE << nombreNormalizado << Color::RESET << L"\n";
            std::wcout << L"  * Longitud de nombre: " << nombreNormalizado.length() << L" / 100 caracteres\n";
            std::wcout << L"  * Categoría destino : " << categoriaElegida << L"\n";
            std::wcout << L"  * Subdivisión       : " << subdivisionSugerida << L"\n";
            std::wcout << L"  * Tipo estructura   : " << Color::AMARILLO << L"PLANA (Sin subcarpetas internas)" << Color::RESET << L"\n";

            std::wcout << L"\n¿Desea crear físicamente este contenedor en el servidor? [S/N]: ";
            std::wstring confirmacion = aMayusculas(leerLinea());

            if (confirmacion == L"S" || confirmacion == L"SI") {
                std::wstring rutaCreada;
                std::wstring errorMsg;
                bool ok = GestorMC::crearProyectoPlano(
                    rutaBaseUNC,
                    categoriaElegida,
                    subdivisionSugerida,
                    nombreNormalizado,
                    rutaCreada,
                    errorMsg
                );

                if (ok) {
                    std::wcout << L"\n" << Color::VERDE << Color::BOLD << L"✓ PROYECTO CREADO CON ÉXITO EN EL SERVIDOR." << Color::RESET << L"\n";
                    std::wcout << L"Ruta Windows UNC : " << Color::AZUL << rutaCreada << Color::RESET << L"\n";
                    std::wstring rutaLinux = MC::unc_a_linux(rutaCreada, rutaBaseUNC);
                    std::wcout << L"Ruta Linux POSIX : " << Color::AMARILLO << rutaLinux << Color::RESET << L"\n";

                    // Incorporar de inmediato al inventario en RAM
                    inventarioMemoria.emplace_back(
                        nombreNormalizado,
                        categoriaElegida,
                        subdivisionSugerida,
                        rutaCreada,
                        rutaLinux
                    );
                } else {
                    std::wcout << L"\n" << Color::ROJO << Color::BOLD << L"✗ ERROR EN LA CREACIÓN:" << Color::RESET << L" " << errorMsg << L"\n";
                }
            } else {
                std::wcout << Color::AMARILLO << L"Operación cancelada por el usuario.\n" << Color::RESET;
            }
            pausar();

        } else if (opcionStr == L"5") {
            // =================================================================
            // OPCIÓN 5: Self-Test de validaciones automáticas
            // =================================================================
            ejecutarSelfTest();
            pausar();

        } else if (opcionStr == L"6") {
            // =================================================================
            // OPCIÓN 6: Salir
            // =================================================================
            std::wcout << L"\nCerrando aplicación. Hasta pronto.\n";
            ejecutando = false;

        } else {
            std::wcout << Color::ROJO << L"Opción no reconocida. Intente nuevamente.\n" << Color::RESET;
            pausar();
        }
    }

    return 0;
}
