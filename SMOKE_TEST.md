# PROTOCOLO DE VERIFICACIÓN EN LIMPIO (SMOKE TEST)
## Auditoría de Calidad (QA), Certificación y Criterios de Aceptación Pre-Despliegue
### Aplicativo: Master Control — Directorio Interno y Gobernanza de Proyectos GLP
### Binario Objetivo: `DirectorioMC.exe` (Monolito Win32 Nativo C++17)

---

### METADATOS DE CERTIFICACIÓN
* **Organización:** Master Control — Ingeniería y Proyectos GLP
* **Infraestructura:** Servidor ClearOS 7 (Samba 4) en `\\192.168.1.242\mcontrol`
* **Estaciones Cliente Objetivo:** Windows 10 Pro / Enterprise (x64) y Windows 11 (x64)
* **Entorno de Prueba:** Puesto de trabajo sin herramientas de desarrollo ni Visual Studio instalado (Instalación limpia de Windows).
* **Versión Evaluada:** 1.0.0.0 (Release Monolítico Estático)

---

## 1. OBJETIVO DEL PROTOCOLO

Este documento define la batería formal de pruebas de aceptación (*Smoke Test*) que el equipo de sistemas o control de calidad debe ejecutar sobre una estación de trabajo limpia antes de autorizar el despliegue general en los equipos de los ingenieros y proyectistas de **Master Control**.

El objetivo es validar que el binario `DirectorioMC.exe`:
1. No requiera instalación previa de ningún runtime (`vcruntime140.dll`, `.NET`, `Java`, librerías MinGW).
2. Tenga un arranque instantáneo ($< 100\text{ ms}$) y consumo mínimo de memoria RAM ($< 15\text{ MB}$).
3. Sea completamente tolerante a desconexiones de red SMB sin congelarse.
4. Cumpla de forma inquebrantable las reglas de gobernanza documental y estructura plana.

---

## 2. CHECKLIST DE EJECUCIÓN PASO A PASO

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                        CHECKLIST DE CERTIFICACIÓN PRE-DESPLIEGUE                       │
├────┬─────────────────────────────┬──────────────────────────────────────────┬──────────┤
│ ID │ Fase de Prueba              │ Criterio de Aceptación                   │ Estado   │
├────┼─────────────────────────────┼──────────────────────────────────────────┼──────────┤
│ T1 │ Auditoría IAT / Dependencias│ 100% DLLs nativas de Windows             │ [ ] PASS │
│ T2 │ Huella en Disco y Arranque  │ Tamaño < 1 MB | Arranque < 100 ms        │ [ ] PASS │
│ T3 │ Consumo de Recursos en RAM  │ RAM en reposo < 15 MB | CPU 0.0%         │ [ ] PASS │
│ T4 │ Aislamiento ante Caída Red  │ Detección en < 200 ms | Sin GUI Freeze   │ [ ] PASS │
│ T5 │ Indexación SMB Conectada    │ Lectura de 6 categorías maestras         │ [ ] PASS │
│ T6 │ Filtrado Reactivo en RAM    │ Búsqueda instantánea (< 2 ms)            │ [ ] PASS │
│ T7 │ Lazy Loading de Entregables │ Carga bajo demanda en ListBox            │ [ ] PASS │
│ T8 │ Integración con el Shell    │ Apertura en Explorer / AutoCAD / Acrobat │ [ ] PASS │
│ T9 │ Gobernanza y Alta de Proy.  │ Límite 100 car. | Creación plana en SMB  │ [ ] PASS │
└────┴─────────────────────────────┴──────────────────────────────────────────┴──────────┘
```

---

### PASO 1: AUDITORÍA ESTÁTICA DE DEPENDENCIAS (IAT)
* **Procedimiento:**
  1. Abrir una consola de comandos en la carpeta de compilación.
  2. Ejecutar el script:
     ```cmd
     auditar_dependencias.bat
     ```
* **Criterio de Aceptación:**
  - El script debe inspeccionar la tabla de importaciones del ejecutable y certificar en verde:
    `[CERTIFICADO] El binario es 100% portatil y autonomo.`
  - **DLLs autorizadas detectadas:** `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `COMCTL32.dll`, `SHELL32.dll`, `ADVAPI32.dll`.
  - **DLLs prohibidas:** No debe figurar ninguna entrada como `VCRUNTIME140.dll`, `MSVCP140.dll`, `libgcc_s_seh-1.dll` ni `libstdc++-6.dll`.

---

### PASO 2: TAMAÑO DE ARCHIVO Y ARRANQUE EN FRÍO
* **Procedimiento:**
  1. Verificar el tamaño físico del archivo `DirectorioMC.exe` en el Explorador de Windows o mediante `Get-Item`.
  2. Hacer doble clic en el ejecutable y medir visualmente el tiempo de aparición de la ventana.
* **Criterio de Aceptación:**
  - El tamaño debe encontrarse estrictamente entre **$380\text{ KB}$ y $550\text{ KB}$** (ampliamente inferior al límite de $1\text{ MB}$).
  - La ventana principal de $1100 \times 700$ px debe renderizarse de forma instantánea ($< 100\text{ ms}$), sin pantallas de carga ni consolas negras parpadeantes de fondo.

---

### PASO 3: CONSUMO DE RECURSOS EN REPOSO
* **Procedimiento:**
  1. Abrir el **Administrador de Tareas de Windows** (`Ctrl + Shift + Esc`).
  2. Localizar el proceso `DirectorioMC.exe` en la pestaña "Procesos" o "Detalles".
* **Criterio de Aceptación:**
  - **Memoria de Trabajo Privada (RAM):** Entre **$6\text{ MB}$ y $14\text{ MB}$**.
  - **Uso de Procesador (CPU):** **$0.0\%$** constante en estado inactivo.
  - **Hilos Activos:** Exactamente $1$ hilo en reposo (hilo de interfaz Win32).

---

### PASO 4: TOLERANCIA A FALLAS Y DESCONEXIÓN DE RED (FAST PROBE)
* **Procedimiento:**
  1. Desconectar físicamente el cable de red del equipo de prueba o apagar la tarjeta WiFi.
  2. Iniciar `DirectorioMC.exe` (o presionar el botón "Actualizar" si ya estaba abierto).
* **Criterio de Aceptación:**
  - La aplicación **NO debe congelarse** ni mostrar el cartel "No responde" del sistema operativo.
  - En menos de **$200\text{ ms}$**, la barra de estado inferior debe reportar en texto claro:
    `ERROR SMB: Servidor 192.168.1.242 no responde en la red local. Verifique que el servidor ClearOS esté encendido y el cable de red conectado.`
  - La tabla principal debe desplegar la fila informativa:
    - *Columna 0:* `SIN CONEXIÓN AL SERVIDOR SMB`
    - *Columna 1:* `Verifique la red o presione 'Actualizar'`
    - *Columna 2:* `192.168.1.242`
  - El botón "Actualizar" debe quedar habilitado para reintentar cuando se restablezca la conexión.

---

### PASO 5: INDEXACIÓN SMB ASÍNCRONA
* **Procedimiento:**
  1. Reconectar el cable de red hacia el switch local de Master Control.
  2. Presionar el botón **"Actualizar"** en la interfaz.
* **Criterio de Aceptación:**
  - El cursor se transforma en reloj de arena (`IDC_WAIT`) de forma persistente sobre el área cliente.
  - Los botones "Actualizar", "[+] Crear Proyecto" y el cuadro de búsqueda se deshabilitan temporalmente para impedir reentrancia.
  - La barra de estado refleja el progreso en vivo (`Indexando: 01_OBRAS_E_INSTALACIONES...`).
  - Al concluir, la barra de estado indica:
    `Listo: X proyectos indexados en Y ms.` (típicamente $< 800\text{ ms}$ para redes Gigabit).
  - La tabla `WC_LISTVIEW` queda completamente poblada con los puntos de anclaje, líneas de negocio y años.

---

### PASO 6: BÚSQUEDA REACTIVA EN MEMORIA RAM
* **Procedimiento:**
  1. Teclear términos en el cuadro de búsqueda superior:
     - Prueba A: Código de centro de costo (ej. `CC43` o `4322`).
     - Prueba B: Nombre de cliente o palabra clave (ej. `PAMPITE`, `GLP`, `OBRA`).
     - Prueba C: Año o subdivisión (ej. `2026`).
  2. Borrar el contenido del cuadro con la tecla `Backspace`.
* **Criterio de Aceptación:**
  - El filtrado debe responder en tiempo real con cada pulsación de tecla ($< 2\text{ ms}$).
  - La búsqueda debe ser insensible a mayúsculas/minúsculas y acentos.
  - Al vaciar el cuadro, la tabla debe restaurar inmediatamente la totalidad de los proyectos indexados.

---

### PASO 7: INSPECCIÓN BAJO DEMANDA (*LAZY LOADING*) Y PORTAPAPELES
* **Procedimiento:**
  1. Hacer clic simple sobre cualquier proyecto de la tabla.
  2. Observar el panel derecho de detalles.
  3. Hacer clic en **"Copiar Ruta Windows (UNC)"** y pegar en el Bloc de Notas (`Ctrl + V`).
  4. Hacer clic en **"Copiar Ruta Linux"** y pegar en el Bloc de Notas (`Ctrl + V`).
* **Criterio de Aceptación:**
  - Los cuadros de solo lectura deben mostrar las rutas completas en tiempo real:
    - UNC: `\\192.168.1.242\mcontrol\RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS\...`
    - Linux: `/home/mcontrol/RESPALDOS_PROYECTOS_Y_MANTENIMIENTOS/...`
  - La lista de entregables técnicos (`ListBox`) debe inspeccionar la carpeta en red bajo demanda y listar los archivos con su tamaño formateado en megabytes (`.dwg`, `.pdf`, `.xlsx`).
  - Si la carpeta está vacía, debe mostrar: `(Carpeta vacía / Sin entregables respaldados)`.
  - El portapapeles del sistema operativo debe recibir el texto exacto sin caracteres nulos ni saltos espurios.

---

### PASO 8: INTEGRACIÓN CON EL SHELL DE WINDOWS
* **Procedimiento:**
  1. Con un proyecto seleccionado, hacer clic en el botón **"Abrir Carpeta"** (o presionar la tecla `Enter` sobre el proyecto seleccionado).
  2. En la lista de entregables técnicos, hacer doble clic sobre un plano `.dwg` o documento `.pdf`.
* **Criterio de Aceptación:**
  - "Abrir Carpeta" debe invocar `explorer.exe` posicionando al usuario directamente en la carpeta compartida del servidor.
  - El doble clic sobre un entregable debe lanzar el software predeterminado asociado en la estación cliente (AutoCAD para `.dwg`, Adobe Acrobat Reader para `.pdf`, Microsoft Excel para `.xlsx`).

---

### PASO 9: GOBERNANZA DOCUMENTAL Y CREACIÓN ASISTIDA
* **Procedimiento:**
  1. Hacer clic en el botón verde **"[+] Crear Nuevo Proyecto"**.
  2. Verificar que se despliegue la ventana modal centrada y bloqueando la ventana principal.
  3. Probar el selector de Línea de Negocio (las 6 categorías maestras).
  4. Ingresar un Centro de Costos sin formato (ej. `4567` o `s/cc`).
  5. Ingresar un Cliente (ej. `Pampite Artz`).
  6. Escribir un texto excesivamente largo en "Tipo de Trabajo" hasta superar los 100 caracteres.
  7. Hacer clic en **"Crear en Servidor"**.
* **Criterio de Aceptación:**
  - El campo CC debe normalizarse automáticamente en mayúsculas (`CC4567` o `S_CC`).
  - Las tildes y caracteres especiales deben reemplazarse fonéticamente (Á $\rightarrow$ A, Ñ $\rightarrow$ N).
  - El contador dinámico debe advertir en color rojo al alcanzar el límite preventivo de 100 caracteres y truncar el nombre respetando palabras completas sin guiones bajos finales.
  - Al confirmar la creación:
    1. Se debe crear físicamente la carpeta plana en `\\192.168.1.242\mcontrol\...`.
    2. El nuevo proyecto debe insertarse atómicamente en el inventario en memoria sin requerir un re-escaneo completo de la red.
    3. La tabla principal debe seleccionarlo automáticamente.

---

### PASO 10: CIERRE SEGURO (*GRACEFUL SHUTDOWN*)
* **Procedimiento:**
  1. Presionar "Actualizar" para iniciar un escaneo sobre SMB.
  2. Inmediatamente después de pulsar el botón (mientras el hilo está leyendo carpetas), hacer clic en el botón de cerrar `[X]` de la ventana principal.
* **Criterio de Aceptación:**
  - La ventana no debe colgarse ni arrojar mensaje de error de Windows.
  - En la barra de estado se aprecia brevemente `"Cancelando operaciones de red y cerrando..."`.
  - El proceso `DirectorioMC.exe` debe desaparecer de inmediato del Administrador de Tareas en menos de $50\text{ ms}$, sin dejar hilos zombies ni excepciones de violación de acceso (`0xC0000005`).

---

## 3. CONCLUSIÓN Y DICTAMEN DE CERTIFICACIÓN

Una vez cumplimentados y verificados con éxito los **10 pasos** anteriores, el instalador desatendido [`instalar_cliente.bat`](file:///d:/proyectos/MASTERCONTROL/buscador%20de%20proyectos/instalar_cliente.bat) queda formalmente autorizado para su distribución y ejecución masiva en todas las estaciones de trabajo de la oficina técnica de Master Control.
