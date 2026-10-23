#include "diagnostico_red.hpp"

namespace RedMC {

EstadoSonda verificarAccesoRecurso(const std::wstring& rutaUNC) {
    EstadoSonda sonda;

    if (rutaUNC.empty()) {
        sonda.accesible = false;
        sonda.codigoErrorWin32 = ERROR_INVALID_PARAMETER;
        sonda.mensajeDiagnostico = L"Ruta UNC vacía o no configurada.";
        sonda.accionSugerida = L"Especifique una ruta de red válida.";
        return sonda;
    }

    // Sondeo Win32 no bloqueante sobre el recurso de red
    DWORD attrs = GetFileAttributesW(rutaUNC.c_str());

    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        sonda.accesible = true;
        sonda.codigoErrorWin32 = ERROR_SUCCESS;
        sonda.mensajeDiagnostico = L"Recurso SMB conectado y disponible.";
        sonda.accionSugerida = L"Operación normal.";
        return sonda;
    }

    sonda.accesible = false;
    sonda.codigoErrorWin32 = (attrs == INVALID_FILE_ATTRIBUTES) ? GetLastError() : ERROR_DIRECTORY;

    // Mapeo detallado de códigos de error nativos de red SMB
    switch (sonda.codigoErrorWin32) {
        case ERROR_BAD_NETPATH:          // 53: Servidor no alcanzable en red
        case ERROR_SEM_TIMEOUT:          // 121: Expiración del temporizador de semáforo
        case ERROR_NETWORK_UNREACHABLE:  // 1231: Red inaccesible
        case ERROR_HOST_UNREACHABLE:     // 1232: Host inaccesible
            sonda.mensajeDiagnostico = L"Servidor 192.168.1.242 no responde en la red local.";
            sonda.accionSugerida = L"Verifique que el servidor ClearOS esté encendido y el cable de red conectado.";
            break;

        case ERROR_BAD_NET_NAME:         // 67: Recurso compartido no encontrado
            sonda.mensajeDiagnostico = L"El recurso compartido 'mcontrol' no existe o Samba está detenido.";
            sonda.accionSugerida = L"Compruebe el servicio smbd en ClearOS 7 (systemctl status smb).";
            break;

        case ERROR_ACCESS_DENIED:        // 5: Permiso denegado
        case ERROR_LOGON_FAILURE:        // 1326: Nombre de usuario o contraseña incorrectos
        case ERROR_INVALID_PASSWORD:     // 86
        case ERROR_ACCOUNT_RESTRICTION:  // 1327
            sonda.mensajeDiagnostico = L"Acceso denegado o sesión SMB no iniciada.";
            sonda.accionSugerida = L"Autentíquese en Windows con el usuario 'mcontrol' (net use \\\\192.168.1.242\\mcontrol).";
            break;

        case ERROR_PATH_NOT_FOUND:       // 3: Ruta intermedia no encontrada
        case ERROR_FILE_NOT_FOUND:       // 2: Archivo/directorio no encontrado
            sonda.mensajeDiagnostico = L"Carpeta 'RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS' no encontrada.";
            sonda.accionSugerida = L"Compruebe que el directorio base exista en /home/mcontrol/ del servidor.";
            break;

        default:
            sonda.mensajeDiagnostico = L"Error de acceso de red Win32 (" + std::to_wstring(sonda.codigoErrorWin32) + L").";
            sonda.accionSugerida = L"Consulte al administrador de sistemas.";
            break;
    }

    return sonda;
}

} // namespace RedMC
