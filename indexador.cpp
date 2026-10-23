#include "indexador.hpp"
#include "diagnostico_red.hpp"

#include <filesystem>
#include <cwctype>
#include <algorithm>
#include <chrono>

namespace fs = std::filesystem;

namespace MC {

std::vector<Proyecto> indexarRepositorioConProgreso(
    const std::wstring& rutaBaseUNC,
    HWND hwndDestino,
    std::atomic<bool>& tokenCancelacion,
    long long& outDuracionMs,
    const std::wstring& baseLinux) {

    outDuracionMs = 0;

    // CHECKPOINT 1: Evaluación inmediata antes de cualquier acceso al sistema de archivos
    if (tokenCancelacion.load()) {
        return {};
    }

    // PASO 0: Sondeo ultrarrápido de conectividad de red SMB (Fast Probe Win32 < 200 ms)
    RedMC::EstadoSonda sonda = RedMC::verificarAccesoRecurso(rutaBaseUNC);
    if (!sonda.accesible) {
        outDuracionMs = -1;
        if (hwndDestino && !tokenCancelacion.load()) {
            std::wstring errorCompleto = sonda.mensajeDiagnostico + L" " + sonda.accionSugerida;
            auto* pErr = new std::wstring(errorCompleto);
            PostMessageW(hwndDestino, WM_APP_SCAN_FALLIDO, 0, reinterpret_cast<LPARAM>(pErr));
        }
        return {};
    }

    auto tInicio = std::chrono::high_resolution_clock::now();
    std::vector<Proyecto> inventario;
    std::error_code ec;

    fs::path raizPath(rutaBaseUNC);

    try {
        if (!fs::exists(raizPath, ec) || !fs::is_directory(raizPath, ec)) {
            outDuracionMs = -1;
            if (!tokenCancelacion.load() && hwndDestino) {
                auto* pError = new std::wstring(L"Ruta inaccesible en el servidor SMB: " + rutaBaseUNC);
                PostMessageW(hwndDestino, WM_APP_SCAN_FALLIDO, 0, reinterpret_cast<LPARAM>(pError));
            }
            return inventario;
        }

        const auto dirOpts = fs::directory_options::skip_permission_denied;

        // NIVEL 1: Categorías / Líneas de Negocio Maestras
        for (const auto& catEntry : fs::directory_iterator(raizPath, dirOpts, ec)) {
            // CHECKPOINT 2: Evaluación antes de cada categoría principal
            if (tokenCancelacion.load()) {
                return {};
            }

            if (ec) {
                ec.clear();
                continue;
            }

            if (!catEntry.is_directory(ec)) {
                continue;
            }

            std::wstring catNombre = catEntry.path().filename().wstring();
            if (catNombre.empty() || catNombre[0] == L'.') {
                continue;
            }

            // Notificar progreso por categoría si la cancelación no ha sido solicitada
            if (hwndDestino && !tokenCancelacion.load()) {
                auto* pNombreCat = new std::wstring(catNombre);
                PostMessageW(hwndDestino, WM_APP_SCAN_PROGRESO, 0, reinterpret_cast<LPARAM>(pNombreCat));
            }

            // NIVEL 2: Subdivisiones (Años o Contratos Marco)
            for (const auto& subEntry : fs::directory_iterator(catEntry.path(), dirOpts, ec)) {
                // CHECKPOINT 3: Evaluación en el recorrido de subdivisiones
                if (tokenCancelacion.load()) {
                    return {};
                }

                if (ec) {
                    ec.clear();
                    continue;
                }

                if (!subEntry.is_directory(ec)) {
                    continue;
                }

                std::wstring subNombre = subEntry.path().filename().wstring();
                if (subNombre.empty() || subNombre[0] == L'.') {
                    continue;
                }

                // NIVEL 3: Carpetas de Proyecto (Puntos de anclaje)
                for (const auto& projEntry : fs::directory_iterator(subEntry.path(), dirOpts, ec)) {
                    // CHECKPOINT 4: Evaluación tras cada proyecto detectado en red
                    if (tokenCancelacion.load()) {
                        return {};
                    }

                    if (ec) {
                        ec.clear();
                        continue;
                    }

                    if (!projEntry.is_directory(ec)) {
                        continue;
                    }

                    std::wstring projNombre = projEntry.path().filename().wstring();
                    if (projNombre.empty() || projNombre[0] == L'.') {
                        continue;
                    }

                    std::wstring projUNC = projEntry.path().wstring();
                    std::wstring projLinux = unc_a_linux(projUNC, rutaBaseUNC, baseLinux);

                    inventario.emplace_back(
                        std::move(projNombre),
                        catNombre,
                        subNombre,
                        std::move(projUNC),
                        std::move(projLinux)
                    );

                    if (tokenCancelacion.load()) {
                        return {};
                    }
                }
            }
        }

        // CHECKPOINT FINAL: No calcular métricas si fue cancelado
        if (tokenCancelacion.load()) {
            return {};
        }

        auto tFin = std::chrono::high_resolution_clock::now();
        outDuracionMs = std::chrono::duration_cast<std::chrono::milliseconds>(tFin - tInicio).count();

    } catch (const std::exception& e) {
        outDuracionMs = -1;
        if (!tokenCancelacion.load() && hwndDestino) {
            std::string msg = e.what();
            auto* pError = new std::wstring(msg.begin(), msg.end());
            PostMessageW(hwndDestino, WM_APP_SCAN_FALLIDO, 0, reinterpret_cast<LPARAM>(pError));
        }
        return {};
    } catch (...) {
        outDuracionMs = -1;
        if (!tokenCancelacion.load() && hwndDestino) {
            auto* pError = new std::wstring(L"Excepción crítica no controlada durante el escaneo SMB.");
            PostMessageW(hwndDestino, WM_APP_SCAN_FALLIDO, 0, reinterpret_cast<LPARAM>(pError));
        }
        return {};
    }

    return inventario;
}

std::vector<ArchivoEntregable> listarEntregables(
    const std::wstring& rutaProyectoUNC) {

    std::vector<ArchivoEntregable> listaArchivos;
    std::error_code ec;

    fs::path projPath(rutaProyectoUNC);
    if (!fs::exists(projPath, ec) || !fs::is_directory(projPath, ec)) {
        return listaArchivos;
    }

    const auto dirOpts = fs::directory_options::skip_permission_denied;

    for (const auto& entry : fs::directory_iterator(projPath, dirOpts, ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        if (entry.is_regular_file(ec)) {
            std::wstring nombre = entry.path().filename().wstring();
            if (nombre.empty() || nombre[0] == L'.') {
                continue;
            }

            std::wstring ext = entry.path().extension().wstring();
            for (wchar_t& ch : ext) {
                ch = std::towlower(ch);
            }

            uintmax_t bytes = entry.file_size(ec);
            if (ec) {
                bytes = 0;
                ec.clear();
            }

            listaArchivos.emplace_back(std::move(nombre), std::move(ext), bytes);
        }
    }

    std::sort(listaArchivos.begin(), listaArchivos.end(),
              [](const ArchivoEntregable& a, const ArchivoEntregable& b) {
                  return a.nombre < b.nombre;
              });

    return listaArchivos;
}

} // namespace MC
