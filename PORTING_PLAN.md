# Plan de Port — ADVENA: Legend of Emeris (PS Vita)

> Documento de Arquitectura, Análisis Estático Verificado de `libgameDSO.so`, `decompiled/apk_jadx/`, Mapeo de Símbolos JNI y Hoja de Ruta para el port de **ADVENA** (Android) a **PS Vita**.

---

## 0. Contexto y Estrategia General

* **Juego:** Advena: Legend of Emeris (아드베나)
* **Desarrollador / Publisher:** Gamevil
* **Paquete Java:** `com.gamevil.advena.global`
* **Activity Principal:** `com.gamevil.advena.global.AdvenaLauncher` (hereda de `com.gamevil.nexus2.NexusGLActivity`)
* **Librerías Nativas:**
  * `libgameDSO.so` (1.56 MB, arquitectura ARM 32-bit `armeabi`, ARMv6) — Motor C++ de Action-RPG (Gamevil Nexus2 / GxPZx engine) con renderizado OpenGL ES 1.1
* **TITLEID asignado:** `ADVENA001`
* **Ruta de Datos en PS Vita:** `ux0:data/advena/`
* **Ruta de la Librería:** `ux0:data/advena/libgameDSO.so`
* **Ruta de Assets:** `ux0:data/advena/assets/`
* **Ruta de Sonido / Audio:** `ux0:data/advena/sound/` (archivos `s000.ogg` .. `s076.ogg`)
* **Ruta de Guardado:** `ux0:data/advena/saves/`

### Arquitectura del Motor (Gamevil Nexus2 / GxPZx Engine)
1. **Motor C++ de Action RPG (GxPZx):**
   * Advena utiliza el motor C++ de alto rendimiento de Gamevil (familias de clases `CGxPZxMgr`, `CGxPZxFrame`, `CCharObject`, `CBattleUI`, `CGameET`, `CB15InputKey`).
   * El pipeline gráfico se apoya en **OpenGL ES 1.1 Fixed-Function** (`glClearColorx`, `glTexParameterx`, `glTexCoordPointer`, `glNormalPointer`, `glDrawArrays`, `glOrthof`).
2. **Resolución de Renderizado:**
   * La resolución lógica base del juego es **400 x 240**.
   * Se inicializa mediante `NativeInitWithBufferSize(400, 240)` y `NativeInitDeviceInfo(400, 240)`, configurando el viewport a 960 x 544 en PS Vita con `vitaGL`.
3. **Ciclo de Vida JNI:**
   * `Java_com_gamevil_nexus2_Natives_InitializeJNIGlobalRef`: Guarda la referencia de `JavaVM*` y referencias JNI globales del motor.
   * `Java_com_gamevil_nexus2_Natives_NativeInitWithBufferSize(400, 240)`: Inicializa el gestor de memoria interno del motor (`Gcx_MM_Init` / `startClet`). Debe llamarse **antes** de `NativeInitDeviceInfo`.
   * `Java_com_gamevil_nexus2_Natives_NativeInitDeviceInfo(400, 240)`: Configura las dimensiones del dispositivo y aloca los buffers de dibujo.
   * `Java_com_gamevil_nexus2_Natives_NativeResize(960, 544)`: Configura la matriz ortográfica y el viewport en la pantalla de la consola.
   * `Java_com_gamevil_nexus2_Natives_handleCletEvent(event, p1, p2, pointerId)`: Inyecta eventos de entrada (táctil y teclas de control).
   * `Java_com_gamevil_nexus2_Natives_NativeRender()`: Ejecuta 1 tick del bucle de juego y renderiza el frame actual.
   * `Java_com_gamevil_nexus2_Natives_NativePauseClet()`, `NativeResumeClet()`, `NativeDestroyClet()`.
4. **Carga de Assets y Guardado:**
   * Los assets binarios (`.pzx`, `.zt1`, `data/`, `font/`, `mapdata/`, `pzx/`, `script/`) se cargan mediante callbacks JNI `readAssete(String)` / `isAssetExist(String)` y llamadas directas de I/O libc (`fopen`, `stat`).
   * Las partidas guardadas se gestionan mediante callbacks JNI `saveFile(String, byte[])`, `loadFile(String)`, `isFileExist(String)` y `deleteFile(String)` (redirigidos a `ux0:data/advena/saves/`).

---

## 1. Detección de Arquitectura y Gráficos

* **Arquitectura Binaria:** ARMv5TE / ARMv6 (`armeabi`), código ARM/Thumb en modo `softfp`. Totalmente compatible de forma nativa con el CPU Cortex-A9 de la PS Vita.
* **Gráficos:** OpenGL ES 1.1 Fixed-Function soportado directamente mediante `vitaGL` (`USE_SCELIBC_IO`, modo GLSL o traducción de primitivas).
* **Resolución PS Vita:** 960 x 544 (escalado desde 400x240 manteniendo proporción o pantalla completa con viewport `glViewport(0, 0, 960, 544)`).

---

## 2. Catálogo de Símbolos JNI

### A. Funciones Nativas Exportadas (`libgameDSO.so`)

| Símbolo Exportado | Firma JNI / Descripción |
|---|---|
| `Java_com_gamevil_nexus2_Natives_InitializeJNIGlobalRef` | `()V` — Inicializa referencias globales JNI y guarda el `JavaVM*` |
| `Java_com_gamevil_nexus2_Natives_NativeInitWithBufferSize` | `(II)V` — Inicializa el heap global del motor C (`Gcx_MM_Init` / `startClet`) |
| `Java_com_gamevil_nexus2_Natives_NativeInitDeviceInfo` | `(II)V` — Configura dimensiones internas (400x240) y framebuffers |
| `Java_com_gamevil_nexus2_Natives_NativeResize` | `(II)V` — Notifica cambio de resolución y actualiza matrices GL |
| `Java_com_gamevil_nexus2_Natives_handleCletEvent` | `(IIII)V` — Envía eventos de control (tipo, param1, param2, pointerID) |
| `Java_com_gamevil_nexus2_Natives_NativeRender` | `()V` — Ejecuta un tick de juego y dibuja el frame actual |
| `Java_com_gamevil_nexus2_Natives_NativePauseClet` | `()V` — Pausa la ejecución del motor |
| `Java_com_gamevil_nexus2_Natives_NativeResumeClet` | `()V` — Reanuda la ejecución del motor |
| `Java_com_gamevil_nexus2_Natives_NativeDestroyClet` | `()V` — Libera recursos al salir |
| `Java_com_gamevil_nexus2_Natives_NativeAsyncTimerCallBack` | `(I)V` — Callback de timers asíncronos del motor |
| `Java_com_gamevil_nexus2_Natives_NativeAsyncTimerCallBackTimeStemp` | `(II)V` — Callback de timer con timestamp |
| `Java_com_gamevil_nexus2_Natives_NativeGetPublicKey` | `()Ljava/lang/String;` — Consulta de clave pública para DRM/IAP |
| `Java_com_gamevil_nexus2_Natives_NativeHandleInAppBiiling` | `(Ljava/lang/String;II)V` — Manejador de respuestas de compra in-app |
| `Java_com_gamevil_nexus2_Natives_NativeResponseIAP` | `(Ljava/lang/String;I)V` — Respuesta de IAP |
| `Java_com_gamevil_nexus2_Natives_NativeIsNexusOne` | `(Z)V` — Flag de detección de dispositivo |
| `Java_com_gamevil_nexus2_Natives_NativeNetTimeOut` | `()V` — Notificación de timeout de red |

### B. Métodos Java Invocados por el Motor (`com/gamevil/nexus2/Natives`)

| Método Java | Firma JNI | Implementación en FalsoJNI / Port |
|---|---|---|
| `isAssetExist` | `(Ljava/lang/String;)I` | Consulta existencia y tamaño del asset en `ux0:data/advena/assets/<name>` |
| `readAssete` / `readAssets` | `(Ljava/lang/String;)[B` | Lee y retorna array de bytes del asset desde `ux0:data/advena/assets/<name>` |
| `loadFileFromStorage` | `(Ljava/lang/String;)[B` | Lee archivo desde almacenamiento local |
| `isFileExist` | `(Ljava/lang/String;)I` | Comprueba si existe la partida guardada en `ux0:data/advena/saves/<name>` |
| `saveFile` | `(Ljava/lang/String;[B)I` | Guarda datos en `ux0:data/advena/saves/<name>` |
| `loadFile` | `(Ljava/lang/String;)[B` | Carga datos desde `ux0:data/advena/saves/<name>` |
| `deleteFile` | `(Ljava/lang/String;)I` | Elimina partida guardada en `ux0:data/advena/saves/<name>` |
| `OnSoundPlay` | `(IIZ)V` | Reproduce efecto de sonido o BGM según índice `sndID` (0..76) |
| `OnStopSound` / `stopAndroidSound` | `()V` | Detiene reproducción de sonido |
| `OnVibrate` / `vibrateAndroid` | `(I)V` | Vibración háptica de la consola (`sceCtrlSetActuator` o stub) |
| `OnUIStatusChange` / `changeUIStatus` | `(I)V` | Notificación de cambio de estado de UI |
| `OnEvent` | `(I)V` | Evento genérico de UI/sistema |
| `OnAsyncTimerSet` | `(II)V` / `(III)V` | Configura timer asíncrono para el motor |
| `getGLOptionLinear` | `()I` | Retorna modo de filtrado de texturas (1 = GL_LINEAR) |
| `getLocaleID` | `()I` | Retorna identificador de idioma (0 = Coreano, 1 = Inglés) |
| `getLanguage` | `(I)V` | Callback con el idioma seleccionado |
| `getPhoneModel` / `getDeviceID` / `getAndroidID` | `()[B` | Retorna cadenas identificadoras para el perfil de dispositivo |
| `getMacAddress` / `getPhoneNumber` / `getSimSerialNumber` | `()[B` | Retorna identificadores de red / SIM (stubs seguros) |
| `getTotalMemory` / `getFreeMemory` / `getUsedMemory` | `()J` | Métricas de memoria para el heap de Dalvik / sistema |
| `getAbsolueFilePath` | `()Ljava/lang/String;` | Retorna ruta base de almacenamiento `ux0:data/advena/` |

---

## 3. Guía de Decompilación Reproducible

```bash
# 1. Decompilación Java con jadx
jadx -d "decompiled/apk_jadx" "Advena-1.0.1.apk"

# 2. Decompilar .so con Ghidra y Angr (Docker devrvk/so-decompiler)
docker run --rm --platform linux/amd64 \
  -v "/Volumes/Seagate/PSVITA Develop/Advena-Vita/Advena-1.0.1/lib/armeabi:/input" \
  -v "/Volumes/Seagate/PSVITA Develop/Advena-Vita/decompiled/libgameDSO_armeabi/ghidra:/output" \
  devrvk/so-decompiler decompile /input/libgameDSO.so /output
```

Re-ejecutable en cualquier momento con `porting_tools/build/decompile_all.sh`.

---

## 4. Fases del Port (Checklist)

- [x] **Fase 0: Preparación del Entorno**
  - [x] Repositorio Git inicializado, `.gitignore` anti-DMCA.
  - [x] `porting_tools/` adaptado (`manage_vita.py`, `build_and_install.sh`, `deploy_and_launch_vita3k.sh`, `decompile_all.sh`, `run_tests.sh`).
  - [x] Submódulo / librerías `falso_jni`, `so_util`, `libc_bridge`, `fios`, `kubridge`, `sha1`.
  - [x] Decompilación completa: Java (`decompiled/apk_jadx/`) y Ghidra C (`decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c`).

- [ ] **Fase 1: Configuración del Loader y Mapeo de Símbolos (`source/dynlib.c`)**
  - [ ] Resolver todos los símbolos dinámicos (`libc`, `libm`, `pthreads`, `GLES1`, `io`, `sockets`).
  - [ ] Implementar hooks de redirección de I/O a `ux0:data/advena/assets/` y `ux0:data/advena/saves/`.
  - [ ] Cargar `libgameDSO.so` mediante `so_file_load` / `so_relocate` / `so_resolve`.

- [ ] **Fase 2: Implementación de la Tabla JNI (`source/java.c`)**
  - [ ] Implementar handlers de `isAssetExist`, `readAssete`, `readAssets`, `loadFileFromStorage`.
  - [ ] Implementar sistema de guardado seguro (`isFileExist`, `saveFile`, `loadFile`, `deleteFile`) en `ux0:data/advena/saves/`.
  - [ ] Handlers de información de dispositivo (`getPhoneModel`, `getDeviceID`, `getLocaleID`, etc.).
  - [ ] Handlers de timers y estado de UI (`OnAsyncTimerSet`, `OnUIStatusChange`).

- [ ] **Fase 3: Pipeline de Renderizado Gráfico (`source/main.c`)**
  - [ ] Inicializar ventana y contexto `vitaGL` en 960x544.
  - [ ] Configuración del ciclo nativo: `InitializeJNIGlobalRef` -> `NativeInitWithBufferSize(400, 240)` -> `NativeInitDeviceInfo(400, 240)` -> `NativeResize(960, 544)`.
  - [ ] Bucle principal de renderizado invocando `NativeRender()` y `gl_swap()`.

- [ ] **Fase 4: Input Táctil y Controles Físicos**
  - [ ] Soporte para pantalla táctil frontal mapeada proporcionalmente a 400x240 (`handleCletEvent` con eventos 23=Down, 25=Move, 24=Up).
  - [ ] Mapeo de botones físicos de la PS Vita:
    - D-Pad / Stick Analógico Izquierdo -> Pad direccional virtual.
    - Botón Cruz (X) -> Ataque principal / Confirmar.
    - Botón Círculo (O) / Cuadrado (□) / Triángulo (Δ) -> Habilidades especiales / Items rápidos.
    - Botones L / R -> Cambio de personajes / Shortcuts.
    - Start -> Menú de pausa / Sistema.
    - Select -> Estado / Inventario.

- [ ] **Fase 5: Sistema de Audio (`source/audio.c`)**
  - [ ] Mezclador de audio nativo para PS Vita (`SceAudioOut`).
  - [ ] Reproductor de BGM en hilo secundario (soporte para archivos Ogg Vorbis vía `Tremor` / `libvorbisidec`).
  - [ ] Canales de efectos de sonido SFX (`s000.ogg` .. `s076.ogg`).
  - [ ] Enlace con callbacks JNI `OnSoundPlay(sndID, vol, isLoop)` y `OnStopSound()`.

- [ ] **Fase 6: LiveArea, Empaquetado VPK y Pruebas en Hardware Real**
  - [ ] Configurar assets de LiveArea (`icon0.png`, `bg0.png`, `pic0.png`, `startup.png`, `template.xml`).
  - [ ] Script para empaquetar y transferir assets (`ux0:data/advena/`).
  - [ ] Validación en emulador Vita3K y en consola PS Vita física vía `manage_vita.py`.
