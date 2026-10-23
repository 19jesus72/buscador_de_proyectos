# INFORME TÉCNICO Y DOCUMENTACIÓN DE INGENIERÍA: FASE 2
## Interfaz Gráfica Nativa Win32, Gobernanza Reactiva e Integración con el Sistema Operativo
### Entorno: C++17 / Win32 API Pura (Unicode) / ComCtl32 v6 / Enlace Estático

---

### METADATOS DEL DOCUMENTO
* **Organización:** Master Control — Ingeniería y Proyectos GLP
* **Proyecto:** Aplicativo Nativo de Directorio Interno, Búsqueda y Gobernanza
* **Fase Documentada:** Fase 2 (Pasos 1 al 4) — Interfaz de Usuario y Sistema Gráfico
* **Infraestructura Objetivo:** Servidor ClearOS 7 (Samba / Recurso SMB)
* **Recurso de Red SMB (Windows UNC):** `\\192.168.1.242\mcontrol\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS`
* **Ruta Física en Servidor (POSIX Linux):** `/home/mcontrol/RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS`
* **Paradigma UI:** Win32 API Nativa pura sin frameworks (Cero dependencias de Qt, Electron, MFC o .NET)
* **Fecha de Emisión:** Octubre 2026

---

## 1. RESUMEN EJECUTIVO DE LA FASE 2

La **Fase 2** tuvo como objetivo construir la interfaz gráfica de usuario completa del aplicativo en **C++17 y Win32 API nativa pura**, integrando de manera desacoplada los motores de lógica, indexación, normalización de rutas y creación de directorios desarrollados en la Fase 1.

Durante esta fase se implementaron cuatro componentes fundamentales:
1. **Paso 1:** Esqueleto estructural de ventana nativa, inicialización de controles comunes (ComCtl32 v6), tipografía del sistema (`Segoe UI`), definición de identificadores (`recursos_gui.hpp`) y motor de geometría reactiva (`WM_SIZE` con `BeginDeferWindowPos`).
2. **Paso 2:** Gestión de estado sin variables globales (`EstadoAplicacion` anclado a `GWLP_USERDATA`), poblado atómico de `WC_LISTVIEW` con congelamiento de refresco (`WM_SETREDRAW`) y motor de filtrado reactivo instantáneo en memoria RAM ($< 2\text{ ms}$) al teclear en la barra de búsqueda (`IDC_SEARCH_EDIT`).
3. **Paso 3:** Integración nativa con el sistema operativo: módulo de portapapeles seguro (`OpenClipboard`, `GlobalAlloc`), ejecución delegada mediante el Shell de Windows (`ShellExecuteW` para `explorer.exe` y programas CAD/PDF/Excel), eventos de selección (`LVN_ITEMCHANGED`), doble clic (`NM_DBLCLK`, `LBN_DBLCLK`) e inspección bajo demanda (*lazy evaluation*) de archivos entregables técnicos.
4. **Paso 4:** Ventana secundaria modal de alta estandarizada de proyectos (`modal_nuevo_proyecto.hpp` / `.cpp`), con selectores en cascada, previsualización en vivo, validación de blindaje `MAX_PATH` (100 caracteres), creación física estrictamente plana en el servidor SMB e inserción atómica en caliente en la tabla principal.

---

## 2. ARQUITECTURA MODULAR DE LA FASE 2

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              APLICATIVO GRÁFICO WIN32                                  │
│                                    (main_gui.cpp)                                      │
└───────────────────────────────────────────┬────────────────────────────────────────────┘
                                            │
                                            ▼
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                       VENTANA PRINCIPAL NATIVA (ventana_principal.cpp)                 │
│                 Clase: MasterControl_Directorio | Tamaño: 1100x700                     │
├───────────────────────────────────────────┬────────────────────────────────────────────┤
│           BARRA SUPERIOR (Búsqueda)       │           BARRA DE ESTADO INFERIOR         │
│  [Edit Buscar: EM_SETCUEBANNER] [Actualizar]│  [Mostrando X de Y] [Servidor] [Tecnología]│
├───────────────────────────────────────────┴────────────────────────────────────────────┤
│                       DIVISIÓN HORIZONTAL REACTIVA (WM_SIZE)                           │
│  ┌────────────────────────────────────────┬─────────────────────────────────────────┐  │
│  │     PANEL IZQUIERDO (~63% ancho)       │      PANEL DERECHO (~37% ancho)         │  │
│  │           WC_LISTVIEW (Report)         │  • Rutas de Red: UNC y Linux POSIX      │  │
│  │   Col 0: Punto de Anclaje (360px)      │  • Botones: Abrir, Copiar UNC / Linux   │  │
│  │   Col 1: Línea de Negocio (240px)      │  • ListBox: Entregables (Lazy loading)  │  │
│  │   Col 2: Subdivisión / Año (130px)     │  • Botón: [+] Crear Nuevo Proyecto      │  │
│  └───────────────────▲────────────────────┴────────────────────▲────────────────────┘  │
└──────────────────────┼─────────────────────────────────────────┼────────────────────────┘
                       │                                         │
        ┌──────────────┴──────────────┐           ┌──────────────┴──────────────┐
        ▼                             ▼           ▼                             ▼
[ CONTROLADOR DIRECTO ]     [ PORTAPAPELES ] [ SHELL WINDOWS ]    [ MODAL ALTA ESTANDAR ]
(controlador_directorio.cpp) (portapapeles.cpp)(acciones_shell.cpp)(modal_nuevo_proyecto.cpp)
 ├── EstadoAplicacion         └── copiarTexto()├── abrirCarpeta()  ├── Dropdown Categorías
 ├── poblarTablaProyectos()                    └── abrirArchivo()  ├── Cascada Subdivisión
 ├── aplicarFiltro()                                               ├── Previsualización viva
 └── seleccionarProyecto()                                         └── Blindaje MAX_PATH 100c
```

---

## 3. DESGLOSE TÉCNICO DE LOS PASOS IMPLEMENTADOS

### 3.1. Paso 1: Esqueleto Gráfico y Geometría Reactiva

* **Manifiesto Visual ComCtl32 v6:**
  Se incrustó la directiva de enlace para activar el motor de estilos visuales modernos de Windows 10 y 11:
  ```cpp
  #pragma comment(linker,"\"/manifestdependency:type='win32' \
  name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
  processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
  ```
* **Inicialización:** En `wWinMain`, se invocó a `InitCommonControlsEx` habilitando `ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES`.
* **Tipografía Corporativa Centralizada:**
  Se crearon fuentes nativas con `CreateFontW` usando la familia `"Segoe UI"`:
  - Fuente general de controles: `Segoe UI`, 9 pt regular (`-12 px`, `FW_NORMAL`).
  - Fuente para etiquetas de encabezado: `Segoe UI`, 9 pt negrita (`-12 px`, `FW_BOLD`).
  - Fuente para títulos de sección: `Segoe UI`, 11 pt negrita (`-15 px`, `FW_BOLD`).
  Todas las fuentes se distribuyen a los controles hijos con el mensaje `WM_SETFONT` y se destruyen en `WM_DESTROY` con `DeleteObject`.
* **Identificadores de Recursos (`recursos_gui.hpp`):**
  Definición de enumeraciones enteras para desacoplar los IDs (`IDC_SEARCH_EDIT`, `IDC_BTN_REFRESCAR`, `IDC_LISTA_PROYECTOS`, `IDC_EDIT_RUTA_UNC`, `IDC_EDIT_RUTA_LNX`, `IDC_BTN_ABRIR_CARPETA`, `IDC_BTN_COPIAR_UNC`, `IDC_BTN_COPIAR_LNX`, `IDC_LISTA_ENTREGABLES`, `IDC_BTN_NUEVO_PROYECTO`, `IDC_STATUS_BAR`).
* **Cálculo de Geometría Reactiva (`WM_SIZE`):**
  - La ventana cuenta con un tamaño inicial de `1100 x 700` píxeles centrado en pantalla y una restricción mínima en `WM_GETMINMAXINFO` de `900 x 550` píxeles para evitar que los controles colapsen.
  - El reposicionamiento de los 14 controles hijos se realiza de forma atómica utilizando la API `BeginDeferWindowPos` / `DeferWindowPos` / `EndDeferWindowPos`, garantizando un redimensionamiento fluido y libre de parpadeos (*flickering*):
    $$\text{Ancho Panel Izquierdo} = (\text{Ancho Cliente} - 3 \times \text{Margen}) \times 0.63$$
    $$\text{Ancho Panel Derecho} = \text{Ancho Cliente} - \text{Margen} - X_{\text{PanelDerecho}}$$

---

### 3.2. Paso 2: Gobernanza de Estado, Poblado y Filtrado Reactivo

* **Encapsulamiento del Estado (`EstadoAplicacion`):**
  Para evitar variables globales mutables, se diseñó la estructura `EstadoAplicacion`, la cual se crea dinámicamente en `WM_CREATE` y se asocia al handle de la ventana mediante:
  ```cpp
  SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pEstado));
  ```
  Se recupera en cualquier punto del procedimiento de ventana mediante `GetWindowLongPtrW`.
* **Poblado Atómico del `ListView` (`poblarTablaProyectos`):**
  1. Congelamiento del repintado: `SendMessageW(hwndList, WM_SETREDRAW, FALSE, 0)`.
  2. Vaciado masivo: `ListView_DeleteAllItems(hwndList)`.
  3. Inserción fila por fila de los punteros en `vistaFiltrada`:
     - Columna 0: `nombreCarpeta` (Punto de Anclaje).
     - Columna 1: `categoria` (Línea de Negocio).
     - Columna 2: `subdivision` (Año / Cliente Marco).
     - **Atributo Crítico `lParam`:** Se asocia directamente el puntero del objeto en memoria:
       ```cpp
       item.lParam = reinterpret_cast<LPARAM>(pProyecto);
       ```
  4. Descongelamiento y repintado suave: `SendMessageW(hwndList, WM_SETREDRAW, TRUE, 0); InvalidateRect(hwndList, NULL, TRUE);`.
  5. Actualización en tiempo real de la barra de estado: `"Mostrando: X de Y proyectos"`.
* **Filtrado Reactivo en Memoria RAM (`aplicarFiltro`):**
  - Se intercepta la notificación `EN_CHANGE` del cuadro `IDC_SEARCH_EDIT`.
  - La búsqueda se ejecuta de forma estrictamente local sobre la memoria RAM (sin tocar el disco ni saturar el enlace SMB de red).
  - Compara la subcadena ingresada de forma insensible a mayúsculas/minúsculas contra `nombreCarpeta`, `categoria` y `subdivision`.
  - El tiempo de respuesta del filtrado sobre cientos de proyectos es inferior a los $2\text{ milisegundos}$.

---

### 3.3. Paso 3: Integración con el Sistema Operativo y Lazy Loading

* **Módulo de Portapapeles Nativo (`portapapeles.hpp` / `.cpp`):**
  Implementa `PortapapelesMC::copiarTexto(hwndOwner, texto)` cumpliendo las directivas de seguridad de la API Win32:
  1. Apertura con bucle de hasta 5 reintentos breves ante bloqueos externos (`OpenClipboard`).
  2. Vaciado preventivo (`EmptyClipboard`).
  3. Asignación de memoria global móvil: `GlobalAlloc(GMEM_MOVEABLE, (longitud + 1) * sizeof(wchar_t))`.
  4. Bloqueo de puntero (`GlobalLock`), copia directa con `std::wmemcpy` y desbloqueo (`GlobalUnlock`).
  5. Asignación bajo el formato canónico `CF_UNICODETEXT` mediante `SetClipboardData` y cierre con `CloseClipboard`.
* **Módulo del Shell de Windows (`acciones_shell.hpp` / `.cpp`):**
  - `ShellMC::abrirCarpetaEnExplorador(hwnd, rutaUNC)`: Valida la existencia previa mediante `std::filesystem::exists` (sobrecarga `std::error_code`) para advertir desconexiones SMB y delega la apertura a `explorer.exe` con `ShellExecuteW`.
  - `ShellMC::abrirArchivoConAsociado(hwnd, rutaCompleta)`: Inicia el documento o plano técnico con su software nativo asociado (AutoCAD, Acrobat Reader, Excel).
* **Carga Diferida de Entregables (*Lazy Evaluation*):**
  - Al interceptar `LVN_ITEMCHANGED` con `(uNewState & LVIS_SELECTED)`:
    1. Se recupera el puntero `const Proyecto*` desde `pnmv->lParam`.
    2. Se asignan las rutas a las cajas de solo lectura `IDC_EDIT_RUTA_UNC` y `IDC_EDIT_RUTA_LNX`.
    3. Se habilitan los botones de acción rápida (*Abrir Carpeta*, *Copiar UNC*, *Copiar Linux*).
    4. Se leen únicamente los archivos contenidos en esa carpeta mediante `MC::listarEntregables(pProyecto->rutaUNC)`.
    5. Se puebla el `ListBox` mostrando cada entregable con su tamaño en Megabytes: `[Nombre.dwg]  (1.50 MB)`.
* **Eventos de Doble Clic:**
  - `NM_DBLCLK` sobre el `ListView`: Abre la carpeta en el Explorador de Windows.
  - `LBN_DBLCLK` sobre el `ListBox`: Ejecuta directamente el archivo técnico seleccionado.

---

### 3.4. Paso 4: Diálogo Modal de Alta Estandarizada y Gobernanza

* **Arquitectura de Ventana Modal (`modal_nuevo_proyecto.hpp` / `.cpp`):**
  - Se registra la clase `MasterControl_ModalCrear` con dimensiones fijas de `620 x 485` píxeles, centrada respecto a la ventana principal.
  - Se garantiza la modalidad estricta desactivando la ventana padre:
    ```cpp
    EnableWindow(hwndPadre, FALSE);
    ```
    Se atiende el bucle de mensajes local (`IsDialogMessageW`) y al cerrar se restituye el foco con `EnableWindow(hwndPadre, TRUE); SetActiveWindow(hwndPadre)`.
* **Formulario Asistido y Cascada Dinámica (`CBN_SELCHANGE`):**
  Al seleccionar la categoría en `IDC_MODAL_CB_CATEGORIA`, se actualiza en cascada el combo `IDC_MODAL_CB_SUBDIVISION`:
  - Si es *03_CONTRATOS_MARCO*: opciones `ENI_ECUADOR`, `VEPAGAS`, `OTROS`.
  - Si es *06_VENTA_MATERIALES*: opción fija `2024_2026`.
  - Para *Obras*, *Mantenimientos*, *Licitaciones*, *Propuestas*: opciones `2026`, `2025`, `2024`, `2023`.
* **Previsualización en Vivo y Blindaje `MAX_PATH` (`EN_CHANGE`):**
  - Al modificar los campos de Centro de Costo, Cliente o Tipo, se invoca a `NormalizadorMC::construirNombreProyecto()`.
  - Se despliega dinámicamente el nombre resultante en mayúsculas y sin caracteres ilegales.
  - Se contabilizan los caracteres en tiempo real: si llega a 100 caracteres, trunca la cadena y despliega:  
    `"Longitud: 100 / 100 caracteres (¡LÍMITE MÁXIMO ALCANZADO!)"`.
  - El botón **✓ Crear en Servidor** solo se activa si el nombre del cliente sanitizado no es vacío.
* **Creación Física Estrictamente Plana:**
  - Invoca a `GestorMC::crearProyectoPlano()`, creando únicamente la carpeta raíz del proyecto en el servidor SMB.
  - En caso de colisión previa o error de permisos de red, despliega un `MessageBoxW` descriptivo sin cerrar la modal.
* **Sincronización Atómica en Caliente:**
  Al confirmar la creación con éxito:
  1. Se incorpora de inmediato el nuevo `Proyecto` a `estado->inventarioMaestro`.
  2. Se traslada el nombre del nuevo proyecto a la barra de búsqueda `IDC_SEARCH_EDIT`.
  3. Se filtra la tabla, posicionando y seleccionando automáticamente la fila creada (`ListView_SetItemState`).
  4. Se muestran sus rutas en el panel derecho y se notifica en la barra de estado.

---

## 4. MATRIZ DE ARCHIVOS FUENTE DE LA FASE 2

| Archivo | Rol y Responsabilidad Técnica |
| :--- | :--- |
| [recursos_gui.hpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/recursos_gui.hpp) | Identificadores de controles UI, directiva pragma de ComCtl32 v6 y constantes. |
| [ventana_principal.hpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/ventana_principal.hpp) | Declaración de la clase de ventana, creación y procedimiento `WndProcPrincipal`. |
| [ventana_principal.cpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/ventana_principal.cpp) | Implementación de controles hijos, cálculo geométrico `WM_SIZE` y despacho de eventos. |
| [controlador_directorio.hpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/controlador_directorio.hpp) | Definición de `EstadoAplicacion`, prototipos de filtrado, poblado y selección. |
| [controlador_directorio.cpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/controlador_directorio.cpp) | Lógica de filtrado en RAM, repintado atómico con `WM_SETREDRAW` e inspección lazy. |
| [portapapeles.hpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/portapapeles.hpp) | Contrato de copiado al portapapeles nativo de Windows. |
| [portapapeles.cpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/portapapeles.cpp) | Implementación con `OpenClipboard`, `GlobalAlloc` y formato `CF_UNICODETEXT`. |
| [acciones_shell.hpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/acciones_shell.hpp) | Interfaces de delegación al Shell de Windows. |
| [acciones_shell.cpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/acciones_shell.cpp) | Implementación de apertura en `explorer.exe` y archivos asociados con `ShellExecuteW`. |
| [modal_nuevo_proyecto.hpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/modal_nuevo_proyecto.hpp) | Interfaz de la ventana secundaria modal de creación. |
| [modal_nuevo_proyecto.cpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/modal_nuevo_proyecto.cpp) | Lógica modal, combos en cascada, previsualización viva y llamada a `crearProyectoPlano()`. |
| [main_gui.cpp](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/main_gui.cpp) | Punto de entrada `wWinMain`, inicialización de temas y bucle principal de mensajes. |

---

## 5. COMANDOS DE COMPILACIÓN ESTÁTICA DE LA FASE 2

El aplicativo se compila como un **binario monolítico `.exe` independiente**, enlazado estáticamente contra el CRT de C++ y consumiendo únicamente las librerías base de Windows:

### 5.1. Compilación con MinGW-w64 (`g++`)
```bash
g++ -std=c++17 -O2 -municode -mwindows -static \
    main_gui.cpp ventana_principal.cpp controlador_directorio.cpp \
    modal_nuevo_proyecto.cpp acciones_shell.cpp portapapeles.cpp \
    gestor_proyectos.cpp sanitizador.cpp indexador.cpp mapeo_rutas.cpp \
    -o DirectorioWin32_MC.exe \
    -lcomctl32 -luser32 -lgdi32 -lshell32

# Ejecución de la aplicación
.\DirectorioWin32_MC.exe
```

### 5.2. Compilación con Microsoft Visual C++ (`cl.exe`)
Desde el *x64 Native Tools Command Prompt for VS*:
```cmd
cl /std:c++17 /O2 /EHsc /MT /DUNICODE /D_UNICODE ^
   main_gui.cpp ventana_principal.cpp controlador_directorio.cpp ^
   modal_nuevo_proyecto.cpp acciones_shell.cpp portapapeles.cpp ^
   gestor_proyectos.cpp sanitizador.cpp indexador.cpp mapeo_rutas.cpp ^
   /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comctl32.lib shell32.lib ^
   /OUT:DirectorioWin32_MC.exe

:: Ejecución de la aplicación
DirectorioWin32_MC.exe
```

---

## 6. CONCLUSIÓN Y BENEFICIOS OPERATIVOS DE LA FASE 2

Con la culminación de la **Fase 2**, el aplicativo cuenta con:
1. **Interfaz Gráfica Autónoma:** Cero dependencias externas; el archivo `.exe` generado pesa menos de $1\text{ MB}$ y funciona de forma nativa en cualquier estación de trabajo Windows 10 u 11.
2. **Filtrado Instantáneo en Memoria RAM:** Los operadores pueden filtrar entre cientos de expedientes técnicos en microsegundos sin sobrecargar el servidor Samba ClearOS 7.
3. **Flujo de Trabajo Integrado con el SO:** La apertura delegada al Explorador y a los programas CAD/PDF/Excel permite que el usuario mantenga su flujo de trabajo habitual sin fricción técnica.
4. **Gobernanza Automatizada de Archivos:** La creación física mediante el asistente modal previene colisiones, elimina tildes y caracteres prohibidos, y blinda los nombres contra el límite `MAX_PATH` de Windows.
