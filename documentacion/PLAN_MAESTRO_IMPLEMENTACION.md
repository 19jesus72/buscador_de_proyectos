# PLAN MAESTRO DE IMPLEMENTACIÓN Y ESPECIFICACIÓN TÉCNICA
## Aplicativo Nativo de Directorio Interno, Búsqueda y Gobernanza de Proyectos
### Entorno: C++17 / Win32 API Pura (Enlace Estático Monolítico)

---

### METADATOS DEL PROYECTO
* **Organización:** Master Control — Ingeniería y Proyectos GLP
* **Infraestructura Objetivo:** Servidor de Archivos Central ClearOS 7 (CentOS 7, Samba)
* **Dirección IP y Recurso SMB:** `\\192.168.1.242\mcontrol\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS`
* **Ruta Física en Servidor:** `/home/mcontrol/RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS`
* **Lenguaje y Stack:** C++17 / Win32 API nativa (`user32.lib`, `comctl32.lib`, `shell32.lib`, `shlwapi.lib`)
* **Modelo de Despliegue:** Binario monolítico `.exe` sin dependencias externas ni runtimes adicionales
* **Fecha de Emisión:** Octubre 2026

---

## 1. FUNDAMENTACIÓN TÉCNICA Y SELECCIÓN DEL STACK

### 1.1. Justificación del Enfoque Win32 API Pura frente a Frameworks
Para garantizar que la herramienta funcione de forma indefinida en cualquier estación de trabajo con Windows (10 u 11) sin mantenimiento recurrente ni requerimientos de instalación de paquetes, se adopta de forma categórica la **API Win32 Nativa** combinada con **C++17 estándar**:

| Dimensión Técnica | Enfoque Descartado (Frameworks / Electron / Qt) | Enfoque Aprobado: C++ / Win32 Pura |
| :--- | :--- | :--- |
| **Dependencias Externas** | Requiere runtimes (`vcruntime140.dll`, DLLs de Qt, Node.js) | **Cero dependencias**; consume las DLLs base del sistema operativo. |
| **Tamaño del Binario** | De $35\text{ MB}$ (Qt) a $>120\text{ MB}$ (Electron) | **$\sim 300\text{ KB} - 600\text{ KB}$** (Altamente compacto). |
| **Consumo de Memoria Cliente** | Entre $80\text{ MB}$ y $300\text{ MB}$ de RAM | **$< 15\text{ MB}$ de RAM** en ejecución activa. |
| **Tiempo de Inicio** | De 2 a 5 segundos | **Instantáneo ($< 50\text{ ms}$)**. |
| **Mantenimiento y Obsolescencia** | Requiere actualizar toolchains y paquetes externos | **Inmortal**; compatibilidad binaria garantizada por Microsoft. |

---

### 1.2. Mitigación de Puntos Críticos de Red SMB
Dado que el servidor ClearOS 7 cuenta estrictamente con $4.0\text{ GB}$ de RAM y los proyectos residen en un recurso compartido en red local, el software debe mitigar dos riesgos estructurales:

1. **Aislamiento de la Latencia de Red mediante Hilos Asíncronos (`std::thread`):**
   * **Problema:** Si el escaneo de directorios se ejecuta en el hilo principal de la interfaz (UI Thread), la ventana se congelará mostrando el estado *"No responde"* cada vez que Windows consulte los metadatos a través de la red SMB.
   * **Solución Técnica:** El rastreo del árbol se ejecuta en un hilo en segundo plano (`std::thread`). La UI se presenta inmediatamente al usuario. Al concluir la lectura, el hilo secundario notifica a la ventana principal mediante un mensaje personalizado (`WM_USER + 101`), poblando los controles sin interrupciones.

2. **Indexación en Memoria RAM Local para Búsqueda Instantánea:**
   * **Problema:** Consultar el servidor Samba con cada pulsación de tecla (`EN_CHANGE`) colapsaría el canal de red y ralentizaría la máquina.
   * **Solución Técnica:** El árbol se almacena en memoria local en un vector de estructuras (`std::vector<ProjectEntry>`). Las búsquedas y filtros por texto se realizan localmente en microsegundos sobre la memoria RAM del equipo cliente.

3. **Delegación Nativa al Shell de Windows (`ShellExecuteW`):**
   * En lugar de programar visores de archivos dentro del aplicativo, se invoca `ShellExecuteW`. Esto delega la apertura de carpetas directamente al Explorador de Windows (`explorer.exe`) y la apertura de planos o memorias técnicas a sus programas predeterminados (AutoCAD, Acrobat Reader, Excel).

---

## 2. ARQUITECTURA DE SOFTWARE MODULAR

El código fuente se estructurará bajo un diseño modular limpio y desacoplado, permitiendo compilar tanto en modo consola (depuración) como en modo ventana nativa (producción):

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        DIRECTORIO_MC.EXE                               │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
    ┌───────────────────────────────┼───────────────────────────────┐
    ▼                               ▼                               ▼
[ MÓDULO 1: MOTOR SMB ]     [ MÓDULO 2: GOBERNANZA ]    [ MÓDULO 3: INTERFAZ WIN32 ]
(Filesystem & Hilos)        (Reglas de Negocio MC)       (Ventanas y Controles)
  ├── std::thread scanner     ├── Sanitización strings    ├── Ventana Principal
  ├── Cache std::vector       ├── Prefijo CC obligatorio  ├── Edit (Búsqueda)
  ├── Detección de archivos   ├── Límite 100 caracteres   ├── ListView (Proyectos)
  └── Monitor de latencia     ├── Conversión UNC/Linux    ├── ListBox (Archivos)
                              └── Estructura plana        └── Modal de Alta
```

### 2.1. Estructura de Datos Base (`ProjectEntry`)
```cpp
struct ProjectEntry {
    std::wstring folder_name;      // Ej: CC4322_PAMPITE_ARTZ_DISTRITO_VIVO
    std::wstring category;         // Ej: 01_OBRAS_E_INSTALACIONES_(CC_4.XX_Y_4XXX)
    std::wstring division;         // Ej: 2026 o ENI_ECUADOR
    std::wstring unc_path;         // Ruta completa Windows (\\192.168.1.242\...)
    std::wstring linux_path;       // Ruta equivalente Linux (/home/mcontrol/...)
    std::vector<std::wstring> files; // Lista de entregables detectados
};
```

---

## 3. ESPECIFICACIÓN DE REGLAS DE NEGOCIO Y GOBERNANZA

El aplicativo incorporará de forma estricta las directrices establecidas en los informes maestros de Master Control:

### 3.1. Rutas Raíz Oficiales
* **Ruta Base Windows (UNC):** `\\192.168.1.242\mcontrol\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS`
* **Ruta Base Linux (POSIX):** `/home/mcontrol/RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS`

### 3.2. Taxonomía de las 6 Líneas de Negocio
1. `01_OBRAS_E_INSTALACIONES_(CC_4.XX_Y_4XXX)` $\rightarrow$ Divisiones anuales (`2023`, `2024`, `2025`, `2026`)
2. `02_MANTENIMIENTOS_Y_SERVICIOS_(CC_2.XX_Y_2XXX)` $\rightarrow$ Divisiones anuales (`2023`, `2024`, `2025`, `2026`)
3. `03_CONTRATOS_MARCO_RECURRENTES_(CC_1.XX_Y_11XX)` $\rightarrow$ Subcarpetas (`ENI_ECUADOR`, `VEPAGAS`, `OTROS`)
4. `04_LICITACIONES_(CC_3.XX)` $\rightarrow$ Divisiones anuales (`2023`, `2024`, `2025`, `2026`)
5. `05_PROPUESTAS_Y_COTIZACIONES` $\rightarrow$ Divisiones anuales (`2025`, `2026`)
6. `06_VENTA_MATERIALES_(CC_9.XX)` $\rightarrow$ División única (`2024_2026`)

### 3.3. Algoritmo de Normalización y Creación de Carpetas
Para evitar nombres ilegales y preservar la estabilidad frente al límite `MAX_PATH` de Windows:
1. **Fórmula Sintáctica:** `[PREFIJO_CC]_[CLIENTE_O_NOMBRE]_[TIPO_TRABAJO]`
2. **Formateo de Centro de Costo:**
   * Limpiar caracteres no alfanuméricos.
   * Si no inicia con `CC`, forzar la adición: `4322` $\rightarrow$ `CC4322`. Si está vacío, asignar `S_CC`.
3. **Sanitización de Cadenas:**
   * Supresión de tildes: $\text{Á, É, Í, Ó, Ú, Ñ} \rightarrow \text{A, E, I, O, U, N}$.
   * Reemplazo de caracteres ilegales (`/ \ : * ? " < > | & . , ;`) y espacios por un único guion bajo (`_`).
   * Conversión a mayúsculas estrictas.
4. **Truncado Preventivo de Longitud:**
   * El nombre de la carpeta resultante no debe superar los **100 caracteres** de longitud.
5. **Estructura Plana:**
   * La carpeta se crea como un contenedor único en el servidor (sin subcarpetas intermedias de ingeniería o comerciales).

---

## 4. PLAN DE IMPLEMENTACIÓN POR FASES

```text
[FASE 1]                        [FASE 2]                        [FASE 3]                        [FASE 4]
Prototipo CLI Base              Esqueleto Win32 Nativo          Multihilo y Alta                Compilación Estática
┌──────────────────────┐        ┌──────────────────────┐        ┌──────────────────────┐        ┌──────────────────────┐
│ • std::filesystem    │  ───►  │ • Ventana Win32      │  ───►  │ • std::thread worker │  ───►  │ • Flags /MT o -static│
│ • Sanitización C++   │        │ • ListView / Edit    │        │ • Diálogo Modal Alta │        │ • Manifiesto ComCtl6 │
│ • Validar lectura SMB│        │ • ShellExecuteW      │        │ • Búsqueda reactiva  │        │ • Binario .exe final │
└──────────────────────┘        └──────────────────────┘        └──────────────────────┘        └──────────────────────┘
```

---

### FASE 1: Motor Lógico de Archivos y Prototipo de Consola (Día 1)
**Objetivo:** Validar la lectura, recorrido y creación de carpetas en el servidor SMB sin interfaz gráfica.

* **Actividades Técnicas:**
  1. Implementar la función de escaneo usando `std::filesystem::directory_iterator` confinado a 3 niveles de profundidad (Categoría $\rightarrow$ Año/Cliente $\rightarrow$ Carpeta Proyecto).
  2. Implementar los algoritmos de sanitización de texto (`clean_string`) y normalización del Centro de Costo (`format_cc`).
  3. Programar la rutina de traducción de rutas Windows UNC $\iff$ Linux POSIX.
  4. Realizar pruebas de lectura y creación de carpetas de prueba directamente contra `\\192.168.1.242\mcontrol`.
* **Criterio de Aceptación:** El programa de consola lista las carpetas existentes, calcula correctamente la ruta física en ClearOS 7 y crea una carpeta normalizada respetando las reglas de caracteres y longitud.

---

### FASE 2: Interfaz Gráfica Nativa Win32 y Controles Base (Día 2)
**Objetivo:** Construir la ventana principal con controles estándar del sistema operativo y vincular las acciones del Shell.

* **Actividades Técnicas:**
  1. Registrar la clase de ventana (`RegisterClassExW`) y crear la ventana principal con `CreateWindowExW`.
  2. Integrar los controles comunes (`InitCommonControlsEx`):
     * **`WC_EDIT`:** Barra de búsqueda superior.
     * **`WC_LISTVIEW`:** Tabla central de proyectos en modo reporte (`LVS_REPORT`) con 3 columnas (Nombre de Carpeta, Categoría, Año/División).
     * **`WC_LISTBOX`:** Panel lateral derecho para visualizar los archivos del proyecto seleccionado.
     * **`WC_BUTTON`:** Botones para "Abrir Carpeta", "Copiar UNC", "Copiar Linux" y "Actualizar".
  3. Controlar el evento `WM_SIZE` para redimensionar los controles de forma proporcional cuando el usuario cambie el tamaño de la ventana.
  4. Implementar el evento `NM_DBLCLK` sobre el `ListView` para abrir la carpeta directamente en el Explorador de Windows mediante `ShellExecuteW`.
* **Criterio de Aceptación:** La aplicación muestra los proyectos en una tabla limpia, permite navegar entre ellos y abre el Explorador de Windows al presionar los botones o hacer doble clic.

---

### FASE 3: Asincronía Multihilo y Módulo de Alta Estandarizada (Día 3)
**Objetivo:** Garantizar que la interfaz nunca se congele durante el escaneo de red y añadir el formulario modal para crear proyectos.

* **Actividades Técnicas:**
  1. Mover la función de escaneo SMB a un hilo secundario (`std::thread`).
  2. Definir un mensaje de usuario (`#define WM_SCAN_COMPLETE (WM_USER + 101)`) que el hilo envía a la ventana principal al terminar de recopilar datos.
  3. Implementar el filtrado reactivo: al recibir el mensaje `EN_CHANGE` del cuadro de búsqueda, vaciar y repoblar el `ListView` a partir del vector local de memoria.
  4. Crear una ventana de diálogo modal para el alta de proyectos:
     * Menú desplegable (`WC_COMBOBOX`) con las 6 líneas de negocio.
     * Campos de texto para Centro de Costo, Nombre y Tipo de Trabajo.
     * Previsualización dinámica de la ruta final y cálculo de longitud en tiempo real.
     * Botón de creación física con validación de duplicados.
* **Criterio de Aceptación:** El programa abre de manera instantánea; el escaneo se realiza de forma transparente; la búsqueda filtra en tiempo real sin latencia y el diálogo de alta valida las reglas antes de tocar el disco del servidor.

---

### FASE 4: Optimización, Manifiesto Visual y Compilación Estática (Cierre)
**Objetivo:** Producir el ejecutable monolítico final, asegurando estilos visuales modernos y eliminando cualquier dependencia externa.

* **Actividades Técnicas:**
  1. **Incrustar Manifiesto de Windows (ComCtl32 v6):** Evitar que la interfaz use la apariencia plana y anticuada de Windows 95, activando el motor moderno de controles visuales mediante un archivo de recursos `.rc` o directiva de compilador:
     ```c
     #pragma comment(linker,"\"/manifestdependency:type='win32' \
     name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
     processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
     ```
  2. **Configuración de Enlace Estático:**
     * Garantizar que la biblioteca estándar de C y C++ quede incrustada dentro del propio binario (`/MT` en MSVC o `-static` en MinGW-w64).
  3. **Pruebas de Despliegue en Estación Limpia:**
     * Copiar el binario `.exe` en una PC cliente sin entornos de desarrollo instalados para comprobar arranque inmediato y conectividad con el servidor `192.168.1.242`.
* **Criterio de Aceptación:** Un único archivo `.exe` con un tamaño inferior a $1\text{ MB}$, con apariencia visual nativa moderna y que funciona de forma autónoma.

---

## 5. COMANDOS DE COMPILACIÓN ESTÁTICA

### Opción A: Compilación con Microsoft Visual C++ (MSVC / `cl.exe`)
Ejecutar desde el *x64 Native Tools Command Prompt for VS*:
```cmd
cl /O2 /std:c++17 /MT /DUNICODE /D_UNICODE main.cpp ^
   /link /SUBSYSTEM:WINDOWS ^
   user32.lib comctl32.lib shell32.lib shlwapi.lib ^
   /OUT:DirectorioMasterControl.exe
```
* `/O2`: Optimización de velocidad máxima.
* `/MT`: Multi-threaded Static CRT (incrusta el runtime; no pide DLLs externas).
* `/SUBSYSTEM:WINDOWS`: Elimina la consola negra tras la ventana.

### Opción B: Compilación con MinGW-w64 (`g++`)
Ejecutar desde terminal Windows con GCC en el PATH:
```bash
g++ -O2 -std=c++17 -municode main.cpp -o DirectorioMasterControl.exe \
    -mwindows -static -static-libgcc -static-libstdc++ \
    -lcomctl32 -lshlwapi -lshell32 -luser32
```
* `-mwindows`: Modo GUI sin ventana de terminal.
* `-static -static-libgcc -static-libstdc++`: Enlace estático total de librerías base.

---

## 6. CONCLUSIÓN Y BENEFICIOS OPERATIVOS
La adopción de este plan garantiza una solución técnica de muy alta fidelidad:
1. **Rendimiento Inmediato:** Búsquedas sobre cientos de expedientes técnicos en milisegundos gracias al filtrado en memoria local.
2. **Cero Impacto en el Servidor:** ClearOS 7 no asume procesos de renderizado ni memoria adicional; solo procesa transacciones SMB de lectura/escritura nativas.
3. **Mantenimiento Cero:** Al no existir librerías externas ni dependencias web, el archivo binario generado podrá ejecutarse de forma ininterrumpida a lo largo de los años.
