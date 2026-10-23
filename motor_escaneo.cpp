#include "motor_escaneo.hpp"

#include <filesystem>
#include <cwctype>
#include <algorithm>

namespace fs = std::filesystem;

namespace MC {

std::vector<Proyecto> escanearRepositorio(
    const std::wstring& rutaBaseUNC,
    const std::wstring& baseLinux) {

    std::vector<Proyecto> inventario;
    std::error_code ec;

    fs::path raizPath(rutaBaseUNC);
    if (!fs::exists(raizPath, ec) || !fs::is_directory(raizPath, ec)) {
        return inventario;
    }

    // Opciones para omitir carpetas del sistema sin permisos sin interrumpir el escaneo
    const auto dirOpts = fs::directory_options::skip_permission_denied;

    // NIVEL 1: Categorías / Líneas de Negocio
    for (const auto& catEntry : fs::directory_iterator(raizPath, dirOpts, ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        if (!catEntry.is_directory(ec)) {
            continue;
        }

        std::wstring catNombre = catEntry.path().filename().wstring();
        if (catNombre.empty() || catNombre[0] == L'.') {
            continue; // Omitir directorios ocultos o temporales
        }

        // NIVEL 2: Subdivisiones (Años o Contratos Marco)
        for (const auto& subEntry : fs::directory_iterator(catEntry.path(), dirOpts, ec)) {
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
            // DETENCIÓN ESTRICTA: No se penetra en archivos para no saturar la red SMB
            for (const auto& projEntry : fs::directory_iterator(subEntry.path(), dirOpts, ec)) {
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
            }
        }
    }

    return inventario;
}

std::vector<ArchivoEntregable> listarArchivosProyecto(
    const std::wstring& rutaProyectoUNC) {

    std::vector<ArchivoEntregable> listaArchivos;
    std::error_code ec;

    fs::path projPath(rutaProyectoUNC);
    if (!fs::exists(projPath, ec) || !fs::is_directory(projPath, ec)) {
        return listaArchivos;
    }

    const auto dirOpts = fs::directory_options::skip_permission_denied;

    // Inspección directa únicamente del directorio de proyecto solicitado
    for (const auto& entry : fs::directory_iterator(projPath, dirOpts, ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        if (entry.is_regular_file(ec)) {
            std::wstring nombre = entry.path().filename().wstring();
            if (nombre.empty() || nombre[0] == L'.') {
                continue; // Omitir archivos ocultos o del sistema
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

    // Ordenamiento alfabético natural para visualización en ListBox
    std::sort(listaArchivos.begin(), listaArchivos.end(),
              [](const ArchivoEntregable& a, const ArchivoEntregable& b) {
                  return a.nombre < b.nombre;
              });

    return listaArchivos;
}

} // namespace MC
