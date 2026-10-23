# INFORME TÉCNICO Y DOCUMENTACIÓN DE INGENIERÍA: FASE 1
## Motor Lógico de Archivos, Gobernanza de Directorios y Prototipo CLI
### Entorno: C++17 / Windows Native API & STL / Enlace Estático

---

### METADATOS DEL DOCUMENTO
* **Organización:** Master Control — Ingeniería y Proyectos GLP
* **Proyecto:** Aplicativo Nativo de Directorio Interno, Búsqueda y Gobernanza
* **Fase Documentada:** Fase 1 (Pasos 1 al 4) — Motor de Archivos y Prototipo CLI
* **Servidor de Archivos Central:** ClearOS 7 (CentOS 7, Samba / XFS)
* **Recurso de Red SMB (Windows UNC):** `\\192.168.1.242\mcontrol\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS`
* **Ruta Física en Servidor (POSIX Linux):** `/home/mcontrol/RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS`
* **Estándar de Lenguaje:** C++17 puro (ISO/IEC 14882:2017)
* **Fecha de Emisión:** Octubre 2026

---

## 1. RESUMEN EJECUTIVO Y OBJETIVOS ALCANZADOS

La **Fase 1** tuvo como objetivo primordial establecer los cimientos lógicos, algoritmos de gobernanza de datos y el motor de interacción con el sistema de archivos de red (SMB/CIFS) para la infraestructura de Master Control. 

Se completó con éxito el desarrollo e integración de los cuatro pasos previstos:
1. **Paso 1:** Modelado de entidades del dominio y conversión bidireccional de rutas (Windows UNC $\iff$ Linux POSIX).
2. **Paso 2:** Motor de sanitización de cadenas fonéticas/ASCII, normalización de Centros de Costo (CC) y blindaje preventivo contra `MAX_PATH` (límite de 100 caracteres).
3. **Paso 3:** Motor de escaneo superficial confinado a 3 niveles taxonómicos e inspección bajo demanda (*lazy evaluation*) de archivos entregables técnicos.
4. **Paso 4:** Módulo de creación física de carpetas estrictamente planas en el servidor y ensamblado del ejecutable interactivo por consola (`DirectorioCLI_MC.exe`).

---

## 2. ARQUITECTURA MODULAR DE LA FASE 1

La arquitectura implementada es modular, altamente desacoplada y con cero dependencias externas o de terceros, utilizando de forma estricta las facilidades de la STL de C++17 (`<filesystem>`, `<string>`, `<vector>`, `<chrono>`, `<cwctype>`):

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        DIRECTORIO_CLI_MC.EXE                           │
│                              (main.cpp)                                │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
    ┌───────────────────────────────┼───────────────────────────────┐
    ▼                               ▼                               ▼
[ MÓDULO DE GOBERNANZA ]    [ MÓDULO DE RED SMB ]       [ GESTOR FÍSICO ]
 (sanitizador.hpp / .cpp)    (indexador.hpp / .cpp)     (gestor_proyectos.hpp / .cpp)
  ├── sanitizarCadena()       ├── indexarRepositorio()    └── crearProyectoPlano()
  ├── formatearCC()           │     (3 niveles en RAM)         (Contenedor plano)
  └── construirNombre()       └── listarEntregables()
                                    (Lazy loading)
                                    │
                                    ▼
                          [ MODELO & RUTAS ]
                           (modelo.hpp / mapeo_rutas.hpp / .cpp)
                            ├── struct Proyecto
                            ├── struct ArchivoEntregable
                            ├── unc_a_linux()
                            └── linux_a_unc()
```

---

## 3. DESGLOSE DETALLADO DE COMPONENTES IMPLEMENTADOS

### 3.1. Modelo de Dominio Unificado (`modelo_proyecto.hpp`, `modelo_archivo.hpp`, `modelo.hpp`)

* **`MC::Proyecto`:**
  Entidad que representa un proyecto en memoria. Almacena:
  - `nombreCarpeta`: Nombre plano del proyecto / anclaje (ej. `CC4322_PAMPITE_ARTZ_OBRA_NUEVA`).
  - `categoria`: Línea de negocio oficial (ej. `01_OBRAS_E_INSTALACIONES_(CC_4.XX_Y_4XXX)`).
  - `subdivision`: Año o cliente marco (ej. `2026`, `ENI_ECUADOR`).
  - `rutaUNC`: Ruta absoluta Windows de acceso SMB (`\\192.168.1.242\...`).
  - `rutaLinux`: Ruta absoluta física en ClearOS 7 (`/home/mcontrol/...`).
  - Método `esValido()` para comprobación de integridad.
  - Implementación con semántica de movimiento (`std::move`) para optimización de memoria.

* **`MC::ArchivoEntregable`:**
  Entidad para la representación bajo demanda de documentos y planos:
  - `nombre`: Nombre del archivo con extensión.
  - `extension`: Extensión en minúsculas (ej. `.dwg`, `.pdf`, `.xlsx`).
  - `tamanioBytes`: Tamaño exacto en disco (`uintmax_t`).
  - `tamanioMB`: Tamaño calculado automáticamente en Megabytes con precisión de coma flotante.

---

### 3.2. Mapeo y Normalización de Rutas (`mapeo_rutas.hpp` / `mapeo_rutas.cpp`)

* **Constantes Corporativas:**
  - `MC::BASE_UNC_DEFAULT`: `L"\\\\192.168.1.242\\mcontrol\\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS"`
  - `MC::BASE_LINUX_DEFAULT`: `L"/home/mcontrol/RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS"`

* **Funciones Principales:**
  1. `normalizar_separadores(ruta, formatoWindows)`:
     - Colapsa barras repetidas consecutivas.
     - Preserva obligatoriamente el prefijo de red inicial `\\` de Windows UNC.
     - Preserva la raíz `/` en rutas absolutas Linux.
     - Elimina barras residuales al final.
  2. `unc_a_linux(rutaUNC, baseUNC, baseLinux)`:
     - Normaliza y valida que la ruta inicie con `baseUNC` de forma insensible a mayúsculas (tolerancia del subsistema SMB de Windows).
     - Previene falsos positivos por coincidencia parcial de nombres de carpeta.
     - Extrae el subárbol relativo, reemplaza contrabarras `\` por barras `/` y concatena con la raíz POSIX de ClearOS 7.
     - Si la ruta no pertenece al repositorio corporativo, retorna una cadena vacía `L""` de forma segura.
  3. `linux_a_unc(rutaLinux, baseLinux, baseUNC)`:
     - Valida de forma estricta (sensible a mayúsculas, propio de Linux/XFS) el prefijo `baseLinux`.
     - Convierte los separadores a contrabarras `\` y genera la ruta UNC canónica.

---

### 3.3. Motor de Sanitización y Gobernanza (`sanitizador.hpp` / `sanitizador.cpp`)

Implementado bajo el namespace `NormalizadorMC`:

1. **`sanitizarCadena(entrada)`:**
   - **Mapeo Fonético & ASCII:** Suprime tildes y caracteres especiales:
     - $\text{Á, À, Ä, Â, á, à, ä, â} \longrightarrow \text{A}$
     - $\text{É, È, Ë, Ê, é, è, ë, ê} \longrightarrow \text{E}$
     - $\text{Í, Ì, Ï, Î, í, ì, ï, î} \longrightarrow \text{I}$
     - $\text{Ó, Ò, Ö, Ô, ó, ò, ö, ô} \longrightarrow \text{O}$
     - $\text{Ú, Ù, Ü, Û, ú, ù, ü, û} \longrightarrow \text{U}$
     - $\text{Ñ, ñ} \longrightarrow \text{N}$
   - **Mayúsculas Estrictas:** Conversión forzosa de todo carácter alfabético a mayúsculas.
   - **Sustitución de Caracteres Prohibidos:** Todo carácter no alfanumérico ASCII (`/ \ : * ? " < > | & . , ; -` y espacios) se convierte en guion bajo (`_`).
   - **Colapso y Trim:** Secuencias como `___` se colapsan a un único `_`. Se suprimen los guiones bajos al inicio y al final.

2. **`formatearCC(rawCC)`:**
   - Limpia caracteres no alfanuméricos pero **preserva el punto decimal** para centros de costo fraccionarios (ej. `2.50`, `4.01`).
   - Convierte a mayúsculas y garantiza que inicie con `CC`. Si ya lo posee (ej. `cc1101`), no lo duplica (`CC1101`).
   - Si la entrada es vacía o nula, asigna el código de contingencia: `S_CC`.

3. **`construirNombreProyecto(rawCC, rawCliente, rawTipo)`:**
   - Fórmula: `[PREFIJO_CC]_[CLIENTE]_[TIPO]` (o `[PREFIJO_CC]_[CLIENTE]` si el tipo está ausente).
   - Sanitiza cada segmento de forma independiente antes de ensamblar.
   - **Blindaje `MAX_PATH` (100 caracteres):** Si el resultado excede los 100 caracteres, es truncado preventivamente a 100 caracteres y se eliminan guiones bajos residuales al final.

---

### 3.4. Motor de Indexación SMB y Lazy Loading (`indexador.hpp` / `indexador.cpp`)

Para proteger el rendimiento del servidor ClearOS 7 (que cuenta con $4\text{ GB}$ de RAM) y mitigar la latencia del protocolo SMB sobre la red local:

1. **Escaneo Confinado a 3 Niveles (`indexarRepositorio`):**
   - **Nivel 1:** Categorías maestras (Líneas de Negocio):
     1. `01_OBRAS_E_INSTALACIONES_(CC_4.XX_Y_4XXX)`
     2. `02_MANTENIMIENTOS_Y_SERVICIOS_(CC_2.XX_Y_2XXX)`
     3. `03_CONTRATOS_MARCO_RECURRENTES_(CC_1.XX_Y_11XX)`
     4. `04_LICITACIONES_(CC_3.XX)`
     5. `05_PROPUESTAS_Y_COTIZACIONES`
     6. `06_VENTA_MATERIALES_(CC_9.XX)`
   - **Nivel 2:** Subdivisiones temporales (`2023`, `2024`, `2025`, `2026`) o comerciales (`ENI_ECUADOR`, `VEPAGAS`, `OTROS`, `2024_2026`).
   - **Nivel 3:** Carpetas de proyecto (Contenedores finales).
   - **Detención Estricta:** Durante el escaneo inicial **nunca** se recorren los archivos hoja. Esto permite indexar cientos de proyectos en menos de $150\text{ ms}$ en la memoria RAM del cliente.
   - **Resiliencia:** Uso de `fs::directory_options::skip_permission_denied` y `std::error_code` para tolerar archivos del sistema o permisos restringidos sin lanzar excepciones.

2. **Inspección Bajo Demanda (`listarEntregables`):**
   - Se ejecuta únicamente cuando el usuario solicita ver los detalles de un proyecto específico.
   - Lee los metadatos de los archivos directos (nombre, extensión en minúsculas, tamaño físico en Bytes y Megabytes) y los ordena alfabéticamente.

---

### 3.5. Creación Física Plana de Proyectos (`gestor_proyectos.hpp` / `gestor_proyectos.cpp`)

Implementa `GestorMC::crearProyectoPlano()` bajo las siguientes directrices:
* **Estructura Estrictamente Plana:** Solo crea el directorio raíz del proyecto. **Queda expresamente prohibido** crear subcarpetas automáticas (`01_Ingenieria`, `02_Comercial`, etc.), manteniendo la simplicidad del almacenamiento.
* **Control de Longitud:** Valida que el nombre normalizado no supere los 100 caracteres antes de enviar la instrucción al sistema de archivos.
* **Prevención de Colisión:** Verifica mediante `fs::exists()` si la carpeta ya existe en el servidor. Si existe, aborta la operación y entrega un mensaje descriptivo sin sobreescribir datos.
* **Captura de Errores del Sistema:** En caso de fallas de permisos de red o cuota en el servidor, reporta el código numérico y el mensaje textual de `std::error_code`.

---

### 3.6. Aplicativo de Consola CLI Interactivo (`main.cpp`)

Punto de entrada integral que expone todas las capacidades del sistema en una interfaz por terminal con soporte Unicode UTF-16 (`_O_U16TEXT`) y secuencias de color ANSI:

* **Soporte de Argumentos CLI:** Permite recibir `argv[1]` para sobreescribir la ruta UNC base durante pruebas locales o despliegues en servidores de contingencia.
* **Menú Interactivo:**
  1. *Indexar / Re-escanear:* Mide la latencia en milisegundos con `std::chrono::high_resolution_clock`, almacena el inventario en memoria RAM y muestra el desglose del conteo por cada línea de negocio.
  2. *Buscar en RAM:* Búsqueda instantánea en microsegundos contra `nombreCarpeta`, `categoria` y `subdivision` (case-insensitive).
  3. *Inspeccionar Entregables:* Muestra tabla de archivos directos con cálculo de tamaño en MB.
  4. *Alta Estandarizada:* Asistente interactivo guiado, previsualización de longitud `X / 100`, confirmación `[S/N]`, creación física y adición en caliente al vector en memoria RAM.
  5. *Self-Test:* Batería integrada de pruebas unitarias automáticas.
  6. *Salir.*

---

## 4. REGISTRO DE PRUEBAS UNITARIAS Y RESULTADOS DE ACEPTACIÓN

Durante el desarrollo de la Fase 1 se implementaron tres arneses de pruebas unitarias independientes más el *Self-Test* embebido en la CLI:

| Arnés de Pruebas | Archivo Fuente | Cobertura de Casos | Resultado |
| :--- | :--- | :--- | :--- |
| **Pruebas Paso 1** | `test_paso1.cpp` | Traducción UNC $\rightarrow$ Linux, traducción inversa, prueba de ida y vuelta (*round-trip*), rechazo de rutas foráneas y modelo en memoria. | **100% Superado (5/5)** |
| **Pruebas Paso 2** | `test_paso2.cpp` | Mapeo fonético de tildes/eñes, caracteres prohibidos, formateo de CC (enteros, decimales, minúsculas, vacíos), ensamblado y truncado a 100 caracteres. | **100% Superado (9/9)** |
| **Pruebas Paso 3** | `test_paso3.cpp` | Escaneo jerárquico superficial de 3 niveles sobre árbol sintético, integridad de metadatos, inspección lazy de entregables y resiliencia ante rutas inexistentes. | **100% Superado (4/4)** |
| **Self-Test CLI** | `main.cpp` (Opción 5) | Validación de sanitización, formateo CC, blindaje a 100 caracteres, simetría UNC $\leftrightarrow$ POSIX y seguridad de rutas. | **100% Superado (5/5)** |

---

## 5. GUÍA DE COMPILACIÓN ESTÁTICA Y DESPLIEGUE

Para garantizar la autonomía del binario y asegurar que no requiera instalación de DLLs externas ni runtimes de Visual C++, se definen los siguientes comandos de compilación estática:

### 5.1. Compilación con MinGW-w64 (`g++`)
Desde PowerShell o terminal Windows con GCC en el `PATH`:
```bash
# Compilar el ejecutable principal CLI
g++ -std=c++17 -O2 -Wall -Wextra -static main.cpp gestor_proyectos.cpp indexador.cpp sanitizador.cpp mapeo_rutas.cpp -o DirectorioCLI_MC.exe

# Ejecutar la aplicación CLI
.\DirectorioCLI_MC.exe
```

### 5.2. Compilación con Microsoft Visual C++ (`cl.exe`)
Desde el *x64 Native Tools Command Prompt for VS*:
```cmd
:: Compilar el ejecutable principal CLI con CRT estático (/MT)
cl /std:c++17 /O2 /EHsc /MT /W4 /DUNICODE /D_UNICODE ^
   main.cpp gestor_proyectos.cpp indexador.cpp sanitizador.cpp mapeo_rutas.cpp ^
   /Fe:DirectorioCLI_MC.exe

:: Ejecutar la aplicación CLI
DirectorioCLI_MC.exe
```

---

## 6. CONCLUSIÓN Y CRITERIOS DE TRANSICIÓN A LA FASE 2

Con la culminación de la **Fase 1**, el proyecto cuenta con:
1. Una base lógica y matemática completamente sólida y validada mediante pruebas automatizadas.
2. Un motor de red altamente eficiente que no degrada el servidor ClearOS 7 ni el canal local SMB.
3. Un estándar de gobernanza corporativa que previene errores tipográficos, colisiones y problemas con la longitud de ruta `MAX_PATH`.
4. Una herramienta funcional por línea de comandos que puede ser utilizada inmediatamente para tareas de indexación y creación controlada de proyectos.

**Estado:** Aprobado para transición inmediata a la **Fase 2: Interfaz Gráfica Nativa Win32 (Ventana Principal, ListView en modo reporte, barra de búsqueda reactiva y ShellExecuteW)**.
