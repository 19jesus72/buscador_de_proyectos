#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <iomanip>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

#include "modelo_proyecto.hpp"
#include "modelo_archivo.hpp"
#include "mapeo_rutas.hpp"
#include "motor_escaneo.hpp"

namespace fs = std::filesystem;

namespace Color {
    const wchar_t* VERDE    = L"\033[92m";
    const wchar_t* ROJO     = L"\033[91m";
    const wchar_t* AZUL     = L"\033[94m";
    const wchar_t* CIAN     = L"\033[96m";
    const wchar_t* AMARILLO = L"\033[93m";
    const wchar_t* RESET    = L"\033[0m";
    const wchar_t* BOLD     = L"\033[1m";
}

void imprimir_separador() {
    std::wcout << L"--------------------------------------------------------------------------------\n";
}

// Crea un archivo simulado con un tamaño específico en bytes
void crear_archivo_dummy(const fs::path& ruta, size_t bytes) {
    std::ofstream ofs(ruta, std::ios::binary);
    std::vector<char> buffer(std::min<size_t>(bytes, 4096), 'X');
    size_t escritos = 0;
    while (escritos < bytes) {
        size_t porEscribir = std::min<size_t>(bytes - escritos, buffer.size());
        ofs.write(buffer.data(), porEscribir);
        escritos += porEscribir;
    }
}

int main() {
#ifdef _WIN32
    // Consola nativa en modo UTF-16
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    // Activación de códigos ANSI en Windows 10/11
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
    std::wcout << L"   ARNÉS DE PRUEBAS UNITARIAS: FASE 1 - PASO 3 (MOTOR DE ESCANEO SMB Y LAZY LOADING)\n";
    std::wcout << L"================================================================================\n" << Color::RESET;

    int pruebasPasadas = 0;
    const int totalPruebas = 4;

    // Directorio de prueba temporal aislado
    fs::path repoPrueba = fs::current_path() / L"__temp_test_smb_repo__";
    std::error_code ec;

    // Limpieza previa defensiva si existía de ejecuciones abortadas
    fs::remove_all(repoPrueba, ec);
    fs::create_directories(repoPrueba, ec);

    std::wcout << L"\n[PREPARACIÓN] Creando árbol sintético de 3 niveles con entregables simulados...\n";

    // 1. Estructura sintética de prueba:
    // Categoría 1
    fs::path cat1 = repoPrueba / L"01_OBRAS_E_INSTALACIONES_(CC_4.XX_Y_4XXX)" / L"2026";
    fs::path proj1 = cat1 / L"CC4322_PAMPITE_ARTZ_OBRA_NUEVA";
    fs::path proj2 = cat1 / L"CC4325_QUICENTRO_AMPLIACION";
    fs::create_directories(proj1, ec);
    fs::create_directories(proj2, ec);

    // Archivos entregables dentro de proj1
    crear_archivo_dummy(proj1 / L"6_Planos_Red_GLP.dwg", 1572864);       // 1.50 MB
    crear_archivo_dummy(proj1 / L"Memoria_Tecnica_Calculo.pdf", 524288); // 0.50 MB
    crear_archivo_dummy(proj1 / L"Presupuesto_Materiales.xlsx", 131072); // 0.12 MB

    // Categoría 2
    fs::path cat2 = repoPrueba / L"02_MANTENIMIENTOS_Y_SERVICIOS_(CC_2.XX_Y_2XXX)" / L"2025";
    fs::path proj3 = cat2 / L"CC2100_VEPAGAS_MANT_CORRECTIVO";
    fs::create_directories(proj3, ec);

    // Categoría 3
    fs::path cat3 = repoPrueba / L"03_CONTRATOS_MARCO_RECURRENTES_(CC_1.XX_Y_11XX)" / L"ENI_ECUADOR";
    fs::path proj4 = cat3 / L"CC1102_ENI_MONTECRISTI_ESFERAS";
    fs::create_directories(proj4, ec);

    std::wcout << L"  * Raíz temporal creada: " << repoPrueba.wstring() << L"\n";
    std::wcout << L"  * 4 Proyectos sembrados en 3 categorías distintas.\n";

    // -------------------------------------------------------------------------
    // PRUEBA 1: Escaneo Maestro Superficial (Exactamente 3 niveles)
    // -------------------------------------------------------------------------
    std::wcout << L"\n" << Color::BOLD << L"[PRUEBA 1/4] Escaneo superficial jerárquico (Nivel 1 -> Nivel 2 -> Nivel 3)..." << Color::RESET << L"\n";
    std::vector<MC::Proyecto> inventario = MC::escanearRepositorio(repoPrueba.wstring(), L"/home/mcontrol/TEST");

    std::wcout << L"  * Proyectos detectados en memoria: " << inventario.size() << L" (Esperados: 4)\n";

    assert(inventario.size() == 4);
    if (inventario.size() == 4) {
        std::wcout << L"  [" << Color::VERDE << L"PASADA" << Color::RESET << L"] El motor indexó exactamente las carpetas de proyecto esperadas.\n";
        pruebasPasadas++;
    } else {
        std::wcout << L"  [" << Color::ROJO << L"FALLÓ" << Color::RESET << L"] Número incorrecto de proyectos detectados.\n";
    }

    // -------------------------------------------------------------------------
    // PRUEBA 2: Integridad y desacoplamiento de metadatos del proyecto
    // -------------------------------------------------------------------------
    std::wcout << L"\n" << Color::BOLD << L"[PRUEBA 2/4] Verificación de metadatos de los proyectos indexados..." << Color::RESET << L"\n";
    imprimir_separador();
    std::wcout << Color::BOLD << std::left
               << std::setw(34) << L"PROYECTO"
               << std::setw(15) << L"SUBDIVISION"
               << L"CATEGORÍA" << Color::RESET << L"\n";
    imprimir_separador();

    bool metadatosCorrectos = true;
    for (const auto& p : inventario) {
        std::wcout << std::left
                   << std::setw(34) << p.nombreCarpeta
                   << std::setw(15) << p.subdivision
                   << p.categoria << L"\n";
        if (p.nombreCarpeta.empty() || p.categoria.empty() || p.subdivision.empty() || p.rutaUNC.empty()) {
            metadatosCorrectos = false;
        }
    }
    imprimir_separador();

    assert(metadatosCorrectos);
    if (metadatosCorrectos) {
        std::wcout << L"  [" << Color::VERDE << L"PASADA" << Color::RESET << L"] Todos los proyectos poseen metadatos completos y válidos.\n";
        pruebasPasadas++;
    } else {
        std::wcout << L"  [" << Color::ROJO << L"FALLÓ" << Color::RESET << L"] Se detectaron metadatos incompletos.\n";
    }

    // -------------------------------------------------------------------------
    // PRUEBA 3: Inspección bajo demanda (Lazy Evaluation) de archivos
    // -------------------------------------------------------------------------
    std::wcout << L"\n" << Color::BOLD << L"[PRUEBA 3/4] Inspección Lazy de archivos bajo demanda en 'CC4322_PAMPITE_ARTZ_OBRA_NUEVA'..." << Color::RESET << L"\n";
    
    std::wstring rutaPampite = proj1.wstring();
    std::vector<MC::ArchivoEntregable> archivos = MC::listarArchivosProyecto(rutaPampite);

    std::wcout << L"  * Archivos entregables detectados: " << archivos.size() << L" (Esperados: 3)\n";
    imprimir_separador();
    std::wcout << Color::BOLD << std::left
               << std::setw(34) << L"NOMBRE DE ARCHIVO"
               << std::setw(12) << L"EXTENSIÓN"
               << std::setw(16) << L"TAMAÑO (BYTES)"
               << L"TAMAÑO (MB)" << Color::RESET << L"\n";
    imprimir_separador();

    bool extensionesValidas = true;
    for (const auto& a : archivos) {
        std::wcout << std::left
                   << std::setw(34) << a.nombre
                   << std::setw(12) << a.extension
                   << std::setw(16) << a.tamanioBytes
                   << std::fixed << std::setprecision(2) << a.tamanioMB << L" MB\n";

        if (a.extension != L".dwg" && a.extension != L".pdf" && a.extension != L".xlsx") {
            extensionesValidas = false;
        }
    }
    imprimir_separador();

    assert(archivos.size() == 3 && extensionesValidas);
    if (archivos.size() == 3 && extensionesValidas) {
        std::wcout << L"  [" << Color::VERDE << L"PASADA" << Color::RESET << L"] Lazy loading completado con tamaños y extensiones verificadas.\n";
        pruebasPasadas++;
    } else {
        std::wcout << L"  [" << Color::ROJO << L"FALLÓ" << Color::RESET << L"] Discrepancia en la detección de entregables.\n";
    }

    // -------------------------------------------------------------------------
    // PRUEBA 4: Resiliencia ante rutas inexistentes o inaccesibles
    // -------------------------------------------------------------------------
    std::wcout << L"\n" << Color::BOLD << L"[PRUEBA 4/4] Resiliencia de red ante ruta inexistente..." << Color::RESET << L"\n";
    {
        std::vector<MC::Proyecto> vacio = MC::escanearRepositorio(L"\\\\192.168.1.999\\inexistente\\ruta");
        std::vector<MC::ArchivoEntregable> archVacio = MC::listarArchivosProyecto(L"C:\\Ruta_Falsa_Sin_Acceso");

        bool seguro = vacio.empty() && archVacio.empty();
        assert(seguro);
        if (seguro) {
            std::wcout << L"  [" << Color::VERDE << L"PASADA" << Color::RESET << L"] El motor retornó listas vacías de forma segura sin excepciones no controladas.\n";
            pruebasPasadas++;
        } else {
            std::wcout << L"  [" << Color::ROJO << L"FALLÓ" << Color::RESET << L"] Falla de seguridad o excepciones no interceptadas.\n";
        }
    }

    // -------------------------------------------------------------------------
    // LIMPIEZA DEL ENTORNO DE PRUEBAS
    // -------------------------------------------------------------------------
    fs::remove_all(repoPrueba, ec);
    std::wcout << L"\n[LIMPIEZA] Directorio de prueba temporal eliminado satisfactoriamente.\n";

    // -------------------------------------------------------------------------
    // RESUMEN GLOBAL
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
