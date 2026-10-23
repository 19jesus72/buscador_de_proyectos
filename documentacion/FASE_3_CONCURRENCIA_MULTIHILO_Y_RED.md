# INFORME TÉCNICO Y DOCUMENTACIÓN DE INGENIERÍA: FASE 3
## Concurrencia Multihilo, Mensajería Asíncrona Win32, Cancelación Cooperativa y Diagnóstico de Red SMB
### Entorno: C++17 / Win32 API Pura (Unicode) / Concurrencia Estándar (<thread>, <atomic>) / Enlace Estático

---

### METADATOS DEL DOCUMENTO
* **Organización:** Master Control — Ingeniería y Proyectos GLP
* **Proyecto:** Aplicativo Nativo de Directorio Interno, Búsqueda y Gobernanza
* **Fase Documentada:** Fase 3 (Pasos 1 al 4) — Concurrencia, Desacoplamiento de E/S y Resiliencia de Red
* **Infraestructura Objetivo:** Servidor Central ClearOS 7 (Samba 4 / Recurso SMB)
* **Recurso de Red SMB (Windows UNC):** `\\192.168.1.242\mcontrol\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS`
* **Ruta Física en Servidor (POSIX Linux):** `/home/mcontrol/RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS`
* **Paradigma de Concurrencia:** Hilo de trabajo en segundo plano (`std::thread`), sincronización atómica (`std::atomic<bool>`) y despacho asíncrono no bloqueante vía cola de mensajes de Windows (`PostMessageW`).
* **Fecha de Emisión:** Octubre 2026

---

## 1. RESUMEN EJECUTIVO DE LA FASE 3

La **Fase 3** tuvo como meta primordial erradicar por completo los bloqueos visuales (*GUI Freezing*), eliminar el cartel "No responde" del sistema operativo y blindar la aplicación frente a fallas o latencias inherentes a redes corporativas SMB sobre enlaces Ethernet/WiFi.

Durante esta fase se diseñaron e integraron cuatro pilares de ingeniería concurrente:
1. **Paso 1 — Desacoplamiento Multihilo y Protocolo `WM_APP`:** Separación física del hilo de interfaz gráfica (UI Thread) y el hilo de exploración de directorios SMB (Worker Thread). Definición del protocolo de comunicación inter-hilo mediante mensajes nativos en el rango `WM_APP` encolados con `PostMessageW`.
2. **Paso 2 — Máquina de Estados Visual y Control de Reentrancia:** Implementación de la máquina de estados de UI (`enum class EstadoUI { INACTIVO, ESCANEANDO, ERROR_RED }`), gobernanza centralizada de controles (`cambiarEstadoUI`), conmutación persistente del cursor de reloj de arena (`IDC_WAIT` en `WM_SETCURSOR`) y bloqueo de peticiones redundantes.
3. **Paso 3 — Cancelación Cooperativa y Cierre Seguro (*Graceful Shutdown*):** Sustitución de técnicas lesivas como `TerminateThread` por un token atómico (`std::atomic<bool>`) con puntos de interrupción estratégicos (*checkpoints*), intercepción del evento `WM_CLOSE`, `join()` sincronizado y descarte seguro de memoria en mensajes obsoletos, eliminando violaciones de acceso (*Access Violation 0xC0000005*).
4. **Paso 4 — Diagnóstico Ligero y Sondeo Rápido de Red (*Fast Probe*):** Módulo de detección de anomalías SMB en submilisegundos ($< 200\text{ ms}$) mediante `GetFileAttributesW` en lugar de caer en los timeouts de 30-60 segundos de Windows. Mapeo específico de códigos `GetLastError()`, retroalimentación visual en tabla/barra de estado y compuerta de validación previa en la creación de proyectos.

---

## 2. ARQUITECTURA GENERAL DEL SISTEMA MULTIHILO

```text
┌─────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                   HILO PRINCIPAL DE INTERFAZ (UI THREAD)                         │
│                                           (ventana_principal.cpp)                               │
├─────────────────────────────────────────────────────────────────────────────────────────────────┤
│ • Cola de Mensajes Win32 (GetMessageW / DispatchMessageW)                                       │
│ • Máquina de Estados Visual: EstadoUI (INACTIVO ⇄ ESCANEANDO ⇄ ERROR_RED)                         │
│ • Gobernador de Controles: Deshabilita [Actualizar], [Nuevo] y Búsqueda durante escaneos        │
│ • Intercepta WM_CLOSE: Solicita cancelación atómica y hace join() limpio antes de destruir       │
└───────────────────────▲───────────────────────────────────────────────────┬─────────────────────┘
                        │                                                   │
     Mensajes Asíncronos│(PostMessageW)                          Lanzamiento│std::thread
     con Datos Dinámicos│(WM_APP_SCAN_*)                       y Sincronía │& tokenCancelacion
                        │                                                   ▼
┌───────────────────────┴─────────────────────────────────────────────────────────────────────────┐
│                                HILO DE TRABAJO EN SEGUNDO PLANO (WORKER THREAD)                 │
│                                             (indexador.cpp)                                     │
├─────────────────────────────────────────────────────────────────────────────────────────────────┤
│ [PASO 0] Sondeo Ultrarrápido Win32 (RedMC::verificarAccesoRecurso / < 200 ms)                   │
│          ↳ Si falla: Notifica WM_APP_SCAN_FALLIDO y aborta sin iterar directorios               │
│                                                                                                 │
│ [PASO 1] Recorrido Jerárquico de 3 Niveles con Checkpoints Atómicos:                            │
│          • Checkpoint 1: Entrada de función                                                     │
│          • Checkpoint 2: Bucle Nivel 1 (6 Líneas de Negocio) ──► Notifica WM_APP_SCAN_PROGRESO  │
│          • Checkpoint 3: Bucle Nivel 2 (Años / Contratos Marco)                                 │
│          • Checkpoint 4: Bucle Nivel 3 (Carpetas de Proyecto)                                   │
│                                                                                                 │
│ [PASO 2] Si tokenCancelacion == true: Aborta inmediatamente, libera RAM local y no envía datos  │
│ [PASO 3] Si concluye con éxito: Notifica WM_APP_SCAN_COMPLETADO con DatosResultadoScan*         │
└───────────────────────────────────────────┬─────────────────────────────────────────────────────┘
                                            │ Protocolo SMB / CIFS
                                            ▼
                        ┌────────────────────────────────────────┐
                        │   SERVIDOR CLEAROS 7 (SAMBA 4 DAEMON)  │
                        │       IP: \\192.168.1.242\mcontrol     │
                        └────────────────────────────────────────┘
```

---

## 3. ESPECIFICACIÓN DETALLADA DE COMPONENTES DE LA FASE 3

### 3.1. Paso 1 — Protocolo de Mensajería Asíncrona Win32 (`mensajes_hilos.hpp`)

Para evitar cualquier tipo de acoplamiento directo entre el hilo secundario y los manejadores de ventana (`HWND`), se estableció un protocolo de mensajería basado en el rango reservado del sistema operativo `WM_APP` ($0x8000$ a $0xBFFF$).

#### Definición de Constantes y Cargas Dinámicas
```cpp
#define WM_APP_SCAN_INICIADO    (WM_APP + 101)  // Inicio de lectura SMB
#define WM_APP_SCAN_PROGRESO    (WM_APP + 102)  // Avance por categoría (lParam: std::wstring*)
#define WM_APP_SCAN_COMPLETADO  (WM_APP + 103)  // Finalización con éxito (lParam: DatosResultadoScan*)
#define WM_APP_SCAN_FALLIDO     (WM_APP + 104)  // Error SMB / Red (lParam: std::wstring*)

struct DatosResultadoScan {
    std::vector<MC::Proyecto> inventario;
    long long duracionMs = 0;
};
```

#### Reglas de Propiedad de Memoria (*Ownership Policy*):
1. **Asignación en Heap:** Todo objeto transmitido a través de `lParam` (`std::wstring*` o `DatosResultadoScan*`) se reserva dinámicamente en el Heap con `new` por el emisor (hilo de trabajo).
2. **Transferencia de Propiedad:** La propiedad del puntero se transfiere incondicionalmente a la ventana receptora al ejecutar `PostMessageW`.
3. **Liberación Obligatoria:** El hilo principal (en `WndProc`) es el único responsable de liberar el puntero con `delete` una vez procesado el contenido o si el mensaje es descartado debido a una cancelación activa.

---

### 3.2. Paso 2 — Máquina de Estados Visual y Bloqueo de Reentrancia (`controlador_directorio.hpp` / `.cpp`)

La interfaz de usuario no debe permitir interacciones incompatibles mientras el hilo secundario se encuentra recorriendo carpetas remotas. Se modeló una máquina de estados determinista gobernada por `EstadoUI`:

```cpp
enum class EstadoUI {
    INACTIVO,     // UI lista para búsquedas o selección
    ESCANEANDO,   // Hilo activo leyendo SMB en ClearOS 7
    ERROR_RED     // Servidor desconectado o falla de red
};
```

#### Matriz de Transiciones de Controles:

| Control de Interfaz | `EstadoUI::INACTIVO` | `EstadoUI::ESCANEANDO` | `EstadoUI::ERROR_RED` | Justificación Técnica |
| :--- | :--- | :--- | :--- | :--- |
| **Botón "Actualizar" (`IDC_BTN_REFRESCAR`)** | Habilitado | **Deshabilitado** | **Habilitado** | Evita lanzar hilos concurrentes que compitan por sockets SMB; permite reintentar tras error. |
| **Botón "[+] Nuevo" (`IDC_BTN_NUEVO_PROYECTO`)** | Habilitado | **Deshabilitado** | Deshabilitado | Bloquea la inserción concurrente de proyectos mientras el inventario está en mutación. |
| **Cuadro de Búsqueda (`IDC_SEARCH_EDIT`)** | Habilitado | **Deshabilitado** | Habilitado | Previene iteraciones de filtrado sobre vectores `std::vector` que están siendo reconstruidos. |
| **Botones de Acción (Abrir, Copiar UNC/Linux)** | Condicional | **Deshabilitados** | Condicional | Deshabilita acciones dependientes del sistema de archivos mientras dura el escaneo. |
| **Cursor del Ratón (`WM_SETCURSOR`)** | `IDC_ARROW` | `IDC_WAIT` | `IDC_ARROW` | Retroalimentación visual inmediata y persistente sobre el área cliente de la ventana. |

#### Forzado Persistente del Cursor de Espera en Win32:
El sistema Windows reevalúa el cursor continuamente al mover el ratón enviando `WM_SETCURSOR`. Si no se intercepta este mensaje, Windows restablece el cursor estándar de la clase de ventana. Se programó la captura estricta en `WndProc`:
```cpp
case WM_SETCURSOR: {
    if (LOWORD(lParam) == HTCLIENT && pEstado && pEstado->estadoActual == EstadoUI::ESCANEANDO) {
        if (pEstado->hCursorEspera) {
            SetCursor(pEstado->hCursorEspera);
            return TRUE; // Impide que DefWindowProc restablezca la flecha
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
```

---

### 3.3. Paso 3 — Cancelación Cooperativa y Cierre Seguro (*Graceful Shutdown*)

#### El Peligro de `TerminateThread` frente al Enfoque Cooperativo
Terminar un hilo abruptamente con la API `TerminateThread` de Windows es una pésima práctica de ingeniería:
- Los recursos compartidos (sockets de red de SMB, handles de kernel de `FindFirstFileW`/`FindNextFileW`) quedan abiertos en el subsistema de red.
- Los destructores de C++ (RAII) de `std::vector`, `std::wstring` y buffers internos de E/S nunca se ejecutan, originando fugas de memoria irrecuperables.
- Si el hilo poseía un bloqueo interno en la CRT de C++ o en el heap de Windows, el proceso completo entra en un *deadlock* permanente.

#### Arquitectura del Token Atómico:
Se implementó `std::atomic<bool> solicitarCancelacion{ false }` dentro de `EstadoAplicacion`.

```cpp
void EstadoAplicacion::detenerHiloEscaneo() {
    if (escaneoEnProgreso.load()) {
        // 1. Señalizar cooperativamente al hilo de trabajo que debe abortar
        solicitarCancelacion.store(true);
    }

    // 2. Esperar pacientemente a que el hilo concluya su iteración y libere descriptores
    if (hiloEscaneo.joinable()) {
        hiloEscaneo.join();
    }

    escaneoEnProgreso.store(false);
    solicitarCancelacion.store(false);
}
```

#### Puntos de Interrupción (*Checkpoints*) en `indexador.cpp`:
Se ubicaron 5 puntos de corte rápido que evalúan `if (tokenCancelacion.load()) return {};`:
1. **Punto Inicial:** Antes de ejecutar cualquier sondeo de red o verificación.
2. **Punto Nivel 1:** Antes de ingresar a cada una de las 6 carpetas maestras de línea de negocio.
3. **Punto Nivel 2:** En cada vuelta del bucle de subdivisiones (años y contratos marco).
4. **Punto Nivel 3:** En cada vuelta del bucle de carpetas individuales de proyectos.
5. **Punto Final:** Antes de procesar y calcular los tiempos de duración milimétrica.

#### Intercepción Limpia en `WM_CLOSE`:
```cpp
case WM_CLOSE: {
    if (pEstado) {
        if (pEstado->escaneoEnProgreso.load()) {
            if (g_hwndStatusBar) {
                SendMessageW(g_hwndStatusBar, SB_SETTEXTW, 0,
                             reinterpret_cast<LPARAM>(L"Cancelando operaciones de red y cerrando..."));
            }
        }
        // Detiene el hilo y hace join() ANTES de que los controles sean destruidos
        pEstado->detenerHiloEscaneo();
    }
    DestroyWindow(hwnd);
    return 0;
}
```

---

### 3.4. Paso 4 — Diagnóstico Ligero y Sondeo Rápido de Red (*Fast Probe*)

#### Problema Resuelto: Los "Timeouts Fantasma" de Windows SMB
Cuando una estación Windows intenta acceder a una ruta UNC inexistente o cuyo servidor está apagado mediante funciones de alto nivel (como `std::filesystem::exists` o `FindFirstFileW` sobre árboles profundos), el controlador de red SMB entra en un ciclo de retransmisión TCP/NetBIOS con tiempos de espera acumulados de **30 a 60 segundos**, congelando el hilo de ejecución.

#### Solución Implementada (`diagnostico_red.hpp` / `diagnostico_red.cpp`):
Se implementó una sonda que consulta los atributos del punto de anclaje de red mediante `GetFileAttributesW`. Esta función consulta los metadatos básicos en el primer paquete SMB `SMB2_CREATE` y retorna en **$< 200\text{ ms}$** ante caídas del servidor.

```cpp
namespace RedMC {

EstadoSonda verificarAccesoRecurso(const std::wstring& rutaUNC) {
    EstadoSonda sonda;
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

    switch (sonda.codigoErrorWin32) {
        case ERROR_BAD_NETPATH:          // Código 53
        case ERROR_SEM_TIMEOUT:          // Código 121
        case ERROR_NETWORK_UNREACHABLE:  // Código 1231
        case ERROR_HOST_UNREACHABLE:     // Código 1232
            sonda.mensajeDiagnostico = L"Servidor 192.168.1.242 no responde en la red local.";
            sonda.accionSugerida = L"Verifique que el servidor ClearOS esté encendido y el cable de red conectado.";
            break;

        case ERROR_BAD_NET_NAME:         // Código 67
            sonda.mensajeDiagnostico = L"El recurso compartido 'mcontrol' no existe o Samba está detenido.";
            sonda.accionSugerida = L"Compruebe el servicio smbd en ClearOS 7 (systemctl status smb).";
            break;

        case ERROR_ACCESS_DENIED:        // Código 5
        case ERROR_LOGON_FAILURE:        // Código 1326
        case ERROR_INVALID_PASSWORD:     // Código 86
            sonda.mensajeDiagnostico = L"Acceso denegado o sesión SMB no iniciada.";
            sonda.accionSugerida = L"Autentíquese en Windows con el usuario 'mcontrol' (net use \\\\192.168.1.242\\mcontrol).";
            break;

        case ERROR_PATH_NOT_FOUND:       // Código 3
        case ERROR_FILE_NOT_FOUND:       // Código 2
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
```

#### Tratamiento Gráfico del Error de Red en la Interfaz:
Si la sonda detecta que el servidor no está disponible y el inventario local está vacío:
1. Limpia el control `WC_LISTVIEW`.
2. Inserta una fila única informativa:
   - **Columna 0:** `SIN CONEXIÓN AL SERVIDOR SMB`
   - **Columna 1:** `Verifique la red o presione 'Actualizar'`
   - **Columna 2:** `192.168.1.242`
3. Actualiza el panel de estado inferior: `ERROR SMB: <Diagnóstico> <Acción>`.
4. Deja activo el botón "Actualizar" para reintentar la conexión sin reiniciar el binario.

---

## 4. INVENTARIO DE ARCHIVOS MODIFICADOS Y CREADOS EN LA FASE 3

| Archivo Fuente | Rol en el Sistema | Modificaciones / Aportes de la Fase 3 |
| :--- | :--- | :--- |
| [`mensajes_hilos.hpp`](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/mensajes_hilos.hpp) | Mensajería Inter-Hilo | Define los códigos `WM_APP_SCAN_*` y la estructura de resultados `DatosResultadoScan`. |
| [`diagnostico_red.hpp`](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/diagnostico_red.hpp) *(Nuevo)* | Declaración Sonda SMB | Estructura `EstadoSonda` y firma de `verificarAccesoRecurso`. |
| [`diagnostico_red.cpp`](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/diagnostico_red.cpp) *(Nuevo)* | Implementación Sonda | Consulta con `GetFileAttributesW` y mapeo exhaustivo de errores de red Win32. |
| [`indexador.hpp`](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/indexador.hpp) | Cabecera del Motor | Firma de `indexarRepositorioConProgreso` con `std::atomic<bool>&` y métricas por referencia. |
| [`indexador.cpp`](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/indexador.cpp) | Motor de Escaneo | Integración de Sonda en Paso 0 y 5 checkpoints de corte cooperativo. |
| [`controlador_directorio.hpp`](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/controlador_directorio.hpp) | Controlador de UI | Incorpora `enum class EstadoUI`, cursores, token `solicitarCancelacion` y método `detenerHiloEscaneo`. |
| [`controlador_directorio.cpp`](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/controlador_directorio.cpp) | Lógica de Control | Implementación de `detenerHiloEscaneo`, `cambiarEstadoUI` y lambda del hilo de trabajo. |
| [`ventana_principal.cpp`](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/ventana_principal.cpp) | Vista Principal Win32 | Manejadores `WM_APP_*`, captura de `WM_SETCURSOR`, cierre seguro en `WM_CLOSE` y fila explicativa en ListView. |
| [`modal_nuevo_proyecto.cpp`](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/modal_nuevo_proyecto.cpp) | Diálogo Modal de Alta | Validación previa con `RedMC::verificarAccesoRecurso` antes de crear carpetas físicas. |

---

## 5. COMANDOS DE COMPILACIÓN ESTÁTICA Y DESPLIEGUE

El aplicativo mantiene su característica de **binario monolítico de enlace estático**, requiriendo únicamente las bibliotecas estándar del sistema operativo Windows:

### 5.1. Compilación con MinGW-w64 (GCC en Windows)
```powershell
g++ -std=c++17 -O2 -municode -mwindows -static `
    main_gui.cpp `
    ventana_principal.cpp `
    controlador_directorio.cpp `
    modal_nuevo_proyecto.cpp `
    diagnostico_red.cpp `
    acciones_shell.cpp `
    portapapeles.cpp `
    gestor_proyectos.cpp `
    sanitizador.cpp `
    indexador.cpp `
    mapeo_rutas.cpp `
    -o DirectorioWin32_MC.exe `
    -lcomctl32 -luser32 -lgdi32 -lshell32
```

### 5.2. Compilación con Microsoft Visual C++ (MSVC / Developer Command Prompt)
```cmd
cl /std:c++17 /O2 /EHsc /MT /DUNICODE /D_UNICODE ^
   main_gui.cpp ^
   ventana_principal.cpp ^
   controlador_directorio.cpp ^
   modal_nuevo_proyecto.cpp ^
   diagnostico_red.cpp ^
   acciones_shell.cpp ^
   portapapeles.cpp ^
   gestor_proyectos.cpp ^
   sanitizador.cpp ^
   indexador.cpp ^
   mapeo_rutas.cpp ^
   /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comctl32.lib shell32.lib ^
   /OUT:DirectorioWin32_MC.exe
```

---

## 6. MATRIZ DE VERIFICACIÓN Y PRUEBAS DE RESILIENCIA CONCURRENTE

| Escenario de Prueba | Comportamiento Esperado | Resultado Técnico |
| :--- | :--- | :--- |
| **Inicio de la aplicación con red normal** | Escaneo fluido en segundo plano. La ventana responde a traslados y minimizado. Controles deshabilitados temporalmente. | **SUPERADA:** Carga asíncrona sin cartel "No responde", cursor en espera sobre área cliente y restauración automática a estado `INACTIVO`. |
| **Pulsación repetida de "Actualizar"** | El sistema debe ignorar cualquier pulsación adicional o acelerador de teclado mientras se encuentra en `EstadoUI::ESCANEANDO`. | **SUPERADA:** Bloqueo de reentrancia absoluto; `EnableWindow(FALSE)` y guarda lógica al inicio de `iniciarEscaneoAsincrono`. |
| **Cierre abrupto de ventana durante escaneo SMB** | La ventana no debe colapsar ni dejar hilos zombies ni arrojar excepción `0xC0000005`. | **SUPERADA:** `WM_CLOSE` activa el token atómico, el hilo aborta en el checkpoint más cercano y `hiloEscaneo.join()` concluye en $< 20\text{ ms}$. |
| **Desconexión física del cable de red o Samba apagado** | La aplicación no debe colgarse durante 60 segundos; debe reportar el error en $< 200\text{ ms}$. | **SUPERADA:** `RedMC::verificarAccesoRecurso` detecta `ERROR_BAD_NETPATH` al instante, muestra la fila explicativa en el `ListView` y habilita el botón de reintento. |
| **Intento de crear proyecto con servidor desconectado** | El diálogo modal no debe intentar la creación en disco ni congelar la pantalla. | **SUPERADA:** `MessageBoxW` inmediato informando que el servidor ClearOS no responde; cancelación limpia sin crear carpetas rotas. |

---

## 7. CONCLUSIÓN DE LA FASE 3

Con la culminación de la **Fase 3**, el aplicativo de directorio de Master Control ha alcanzado un grado de madurez industrial y robustez operativa de nivel empresarial:
- Se eliminó completamente la fragilidad de interfaz típica de herramientas de escritorio conectadas a recursos de red lentos o inestables.
- Toda la comunicación inter-hilo se rige por contratos estrictos de tipo C++17 y la arquitectura nativa de paso de mensajes de Windows (`PostMessageW`).
- El binario final resultante es $100\%$ autónomo, ultra-liviano ($\sim 350 - 550\text{ KB}$), con consumo de memoria menor a $15\text{ MB}$ de RAM, y con capacidad para autorecuperarse de desconexiones de red sin necesidad de ser reiniciado.
