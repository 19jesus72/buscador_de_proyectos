#include "gestor_proyectos.hpp"

#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace GestorMC {

bool crearProyectoPlano(
    const std::wstring& rutaBaseUNC,
    const std::wstring& categoria,
    const std::wstring& subdivision,
    const std::wstring& nombreProyectoNormalizado,
    std::wstring& outRutaCreadaUNC,
    std::wstring& outError) {

    outRutaCreadaUNC.clear();
    outError.clear();

    // 1. Validaciones preventivas de entrada
    if (rutaBaseUNC.empty()) {
        outError = L"La ruta base UNC del servidor no está definida.";
        return false;
    }

    if (categoria.empty()) {
        outError = L"La categoría de línea de negocio no puede estar vacía.";
        return false;
    }

    if (subdivision.empty()) {
        outError = L"La subdivisión (año o cliente marco) no puede estar vacía.";
        return false;
    }

    if (nombreProyectoNormalizado.empty()) {
        outError = L"El nombre normalizado del proyecto no puede estar vacío.";
        return false;
    }

    constexpr size_t LIMITE_LONGITUD = 100;
    if (nombreProyectoNormalizado.length() > LIMITE_LONGITUD) {
        outError = L"El nombre de la carpeta excede el límite preventivo de 100 caracteres (Longitud: " +
                   std::to_wstring(nombreProyectoNormalizado.length()) + L").";
        return false;
    }

    // 2. Construcción de la ruta física absoluta de destino
    fs::path rutaDestino = fs::path(rutaBaseUNC) / categoria / subdivision / nombreProyectoNormalizado;
    std::error_code ec;

    // 3. Verificación de colisión previa
    if (fs::exists(rutaDestino, ec)) {
        outError = L"Colisión detectada: El directorio del proyecto ya existe en el servidor: " + rutaDestino.wstring();
        return false;
    }

    // 4. Creación física del directorio contenedor (Estrictamente Plano)
    // No se generan subcarpetas (01_Ingenieria, 02_Comercial, etc.)
    bool creado = fs::create_directories(rutaDestino, ec);
    if (ec || !creado) {
        std::string msg = ec.message();
        std::wstring wmsg(msg.begin(), msg.end());
        outError = L"Error del sistema de archivos al crear la carpeta en el servidor: " + wmsg +
                   L" (Código: " + std::to_wstring(ec.value()) + L")";
        return false;
    }

    // 5. Asignación de salida exitosa
    outRutaCreadaUNC = rutaDestino.wstring();
    return true;
}

} // namespace GestorMC
