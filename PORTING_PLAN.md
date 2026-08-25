# Plan de Port — ADVENA: Legend of Emeris (PS Vita)

> Documento de Arquitectura, Análisis Estático Verificado de `libgameDSO.so`, `decompiled/apk_jadx/`, Mapeo de Símbolos JNI, Registro de Bugs Resueltos y Hoja de Ruta para el port de **ADVENA** (Android) a **PS Vita**.

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
* **Ruta de Assets:** `ux0:data/advena/assets/` y `ux0:data/advena/res/`
* **Ruta de Sonido / Audio:** `ux0:data/advena/sound/` (archivos `s000.ogg` .. `s076.ogg`)
* **Ruta de Guardado:** `ux0:data/advena/saves/`

### Arquitectura del Motor (Gamevil Nexus2 / GxPZx Engine)
1. **Motor C++ de Action RPG (GxPZx):**
   * Advena utiliza el motor C++ de alto rendimiento de Gamevil (familias de clases `CGxPZxMgr`, `CGxPZxFrame`, `CCharObject`, `CBattleUI`, `CGameET`, `CB15InputKey`).
   * El pipeline gráfico se apoya en **OpenGL ES 1.1 Fixed-Function** (`glClearColorx`, `glTexParameterx`, `glTexCoordPointer`, `glNormalPointer`, `glDrawArra2. **Resolución de Renderizado:**
   * La resolución lógica base del juego es **480 x 320** (`GAME_W = 480`, `GAME_H = 320`, definido explícitamente en `AdvenaLauncher.java: gameScreenWidth = 480, gameScreenHeight = 320`).
   * Se inicializa mediante `NativeInitWithBufferSize(480, 320)` y `NativeInitDeviceInfo(480, 320)`, configurando el viewport a 960 x 544 en PS Vita con `vitaGL`.
3. **Ciclo de Vida JNI:**
   * `Java_com_gamevil_nexus2_Natives_InitializeJNIGlobalRef`: Guarda la referencia de `JavaVM*` y referencias JNI globales del motor.
   * `Java_com_gamevil_nexus2_Natives_NativeInitWithBufferSize(480, 320)`: Inicializa el gestor de memoria interno del motor (`Gcx_MM_Init` / `startClet`). Debe llamarse **antes** de `NativeInitDeviceInfo`.
   * `Java_com_gamevil_nexus2_Natives_NativeInitDeviceInfo(480, 320)`: Configura las dimensiones del dispositivo y aloca los buffers de dibujo.
   * `Java_com_gamevil_nexus2_Natives_NativeResize(960, 544)`: Configura la matriz ortográfica y el viewport en la pantalla de la consola.
   * `Java_com_gamevil_nexus2_Natives_handleCletEvent(event, p1, p2, pointerId)`: Inyecta eventos de entrada (táctil y teclas de control).
   * `Java_com_gamevil_nexus2_Natives_NativeRender()`: Ejecuta 1 tick del bucle de juego y renderiza el frame actual.
   * `Java_com_gamevil_nexus2_Natives_NativePauseClet()`, `NativeResumeClet()`, `NativeDestroyClet()`.
4. **Carga de Assets y Guardado:**
   * Los assets binarios (`.pzx`, `.zt1`, `data/`, `font/`, `mapdata/`, `pzx/`, `script/`, `res/`) se cargan mediante callbacks JNI `readAssete(String)` / `isAssetExist(String)` y llamadas directas de I/O libc (`fopen`, `stat`, `access`).
   * Las partidas guardadas se gestionan mediante callbacks JNI `saveFile(String, byte[])`, `loadFile(String)`, `isFileExist(String)` y `deleteFile(String)` (redirigidos a `ux0:data/advena/saves/`).

---

## 1. Detección de Arquitectura y Gráficos

* **Arquitectura Binaria:** ARMv5TE / ARMv6 (`armeabi`), código ARM/Thumb en modo `softfp`. Totalmente compatible de forma nativa con el CPU Cortex-A9 de la PS Vita.
* **Gráficos:** OpenGL ES 1.1 Fixed-Function soportado directamente mediante `vitaGL` (`USE_SCELIBC_IO`, modo GLSL o traducción de primitivas).
* **Resolución PS Vita:** 960 x 544 (escalado desde el canvas nativo de 480x320 con viewport `glViewport(0, 0, 960, 544)`).glViewport(0, 0, 960, 544)`).

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
| `isAssetExist` | `(Ljava/lang/String;)I` | Consulta existencia y tamaño del asset en `ux0:data/advena/assets/<name>` y `res/` |
| `readAssete` / `readAssets` | `(Ljava/lang/String;)[B` | Lee y retorna array de bytes del asset con soporte Dalvik header |
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
| `getAbsolueFilePath` | `()Ljava/lang/String;` | Retorna ruta base de guardado `ux0:data/advena/saves` |
| `getVPoint` | `(I)V` | Notificación de balance de Puntos V |

---

## 3. Registro Histórico de Bugs Diagnosticados y Resueltos

### Bug 1: Persistencia de partidas guardadas en `ux0:data/advena/saves/`
- **Problema:** El motor construía rutas prefijadas con `ux0:data/advena/` (`s0.dat`..`s2.dat`, `g.dat`, `g_an_g.dat`) y `resolve_path_soloader()` las pasaba sin redirección, dejándolas en la raíz de datos en lugar de la subcarpeta `saves/`.
- **Solución (`source/reimpl/io.c`):** Se implementó una verificación de basenames (`is_save_basename`) que redirige explícitamente los archivos de progreso a `ux0:data/advena/saves/` creando el directorio automáticamente.

### Bug 2: Data Abort al cargar partida (`CSaveMgr::LoadPlayData` -> `CGsEncryptFile::ReadPtr`)
- **Problema:** Al entrar al menú de carga de partida, si el slot estaba vacío/truncado (`GsFSFileSize == 0`), `LoadBegin` abortaba sin alocar el buffer de desencriptación (`this->buffer == NULL`). `CSaveMgr::LoadPlayData` ignoraba el retorno e invocaba inmediatamente `ReadPtr()` $\rightarrow$ `memcpy()` con puntero fuente `NULL` causando un Data Abort.
- **Solución (`source/patch.c`):** Se hookeó `CGsEncryptFile::ReadPtr` (offset `0x74714 + 1`). Si `this->buffer == NULL`, se rellena el destino con ceros y se avanza el cursor de lectura en vez de desreferenciar el buffer nulo.

### Bug 3: Stack Overflow / Data Abort recursivo en `InitialPlayerPadSet_patched`
- **Problema:** `so_patch()` se estaba llamando dos veces durante el arranque (en `init.c` y en `main.c`). La segunda llamada provocó que `hook_thumb()` leyera el trampolín `ldr pc, [pc]` como las "instrucciones originales". Al ejecutarse `InitialPlayerPadSet_patched()`, el intento de restaurar y llamar a la función original ejecutaba el propio trampolín, creando una recursión infinita que desbordaba el stack.
- **Solución:**
  1. Se eliminó la llamada duplicada a `so_patch()` en `source/main.c`.
  2. Se agregó un guard estático de idempotencia en `so_patch()` (`source/patch.c`) para evitar cualquier doble aplicación futura.

### Bug 4: Posición y visibilidad del botón Teamstrike [T]
- **Problema:** `GVUIPlayerController::InitialPlayerPadSet` (offset `0x896cc`) posicionaba el botón Teamstrike usando offsets calculados para pantallas pequeñas. Además, `ShowBtn` ocultaba botones por defecto.
- **Solución (`source/patch.c`):**
  1. Se parcheó `ShowBtn` (offsets `0x88f82` y `0x88fba`) para forzar máscara `0x1F` y anular `Hide()`.
  2. Se hookeó `InitialPlayerPadSet` para reposicionar el objeto Teamstrike (`+0x1b0`) usando coordenadas relativas al canvas lógico del juego.

### Bug 5: Desaparición de sprites (ej. botón [X] en menús/opciones)
- **Problema:** Un parche erróneo en `DrawOP_ENLARGE_Compress_16_Ex` (offset `0x136076`) sobreescribía la bifurcación `cmp r3, #0; beq 1360e6` del rasterizador de sprites comprimidos de 16-bit, destruyendo el dibujado de elementos de interfaz.
- **Solución (`source/patch.c`):** Se eliminó el parche destructivo, restaurando la descompresión y dibujado original de sprites.

### Bug 6: Advertencia JNI y cuelgues por `getVPoint` no registrado
- **Problema:** El log arrojaba `GetStaticMethodID(env, ..., "getVPoint", "(I)V"): not found` al entrar en menús.
- **Solución (`source/java.c`):** Se añadió `getVPoint` a `nameToMethodId` y `methodsVoid`.

### Bug 7: Desfase del input táctil y calibración
- **Problema:** En versiones tempranas, las coordenadas táctiles estaban descalibradas respecto a las cajas de colisión y botones del juego.
- **Solución (`source/main.c`):** Se adaptaron las fórmulas de mapeo de pantalla táctil y se calibraron los hotspots virtuales basados en el espacio de coordenadas nativo del motor.

### Bug 8: Rotación y limpieza de logs
- **Problema:** Se creaba un archivo redundante `advena_latest.log` que competía con la rotación secuencial.
- **Solución (`source/utils/logger.c`):** Se eliminó el archivo residual y se mantuvo la rotación limpia `advena_001.log` .. `advena_NNN.log`.

### Bug 9: Recorte de menús inferiores, pérdida del botón [X] y retratos de UI por resolución incorrecta (400x240 vs 480x320)
- **Problema:** Se había asumido una resolución lógica de 400x240 basándose en otros juegos de Gamevil/Nexus2. Al renderizar en 240px de alto, todos los menús y diálogos (diseñados para 320px de alto en Android, como el menú de opciones con el slider de velocidad y el botón [X] inferior) perdían los 80px inferiores, cortando elementos críticos de la interfaz.
- **Solución:**
  1. Se inspeccionó el APK decompilado (`AdvenaLauncher.java:159-160`), confirmando que la resolución de diseño oficial de Advena es `gameScreenWidth = 480` y `gameScreenHeight = 320`.
  2. Se reconfiguraron `GAME_W = 480` y `GAME_H = 320` (`ENGINE_LOGICAL_W/H = 480/320`) en `source/main.c`.
  3. El rasterizador por software ahora renderiza el lienzo completo a 480x320 y `glResize(960, 544)` lo proyecta estirado a pantalla completa en la PS Vita sin perder ningún botón o información visual.
  4. Se recalibraron los hotspots táctiles virtuales para el espacio de 480x320.

### Bug 10: Comportamiento errático del D-Pad físico (movimiento hacia abajo al presionar izquierda) y colisión de eventos táctiles sintetizados
- **Problema:** El bucle de entrada en `main.c` enviaba tanto el KeyCode físico (`KEY_WALK_LEFT`) como un evento táctil sintetizado (`update_virtual_dpad`). El D-Pad virtual interno del motor (`GVUIDirectionPad::checkHitRegion`) interpretaba la posición angular del toque sintético como `KEY_WALK_DOWN` (0x38), enviando eventos conflictivos que anulaban la dirección deseada.
- **Solución (`source/main.c`):** Se eliminó la sintetización de toques virtuales para botones físicos. Los botones físicos y sticks ahora inyectan exclusivamente sus KeyCodes nativos limpios (`50`, `56`, `52`, `54`, etc.) al motor sin interferir con la pantalla táctil real.

### Bug 11: Alineación de estructura `stat64_bionic` para tamaño de archivos de guardado
- **Problema:** En `source/reimpl/io.h`, la estructura `stat64_bionic` tenía `__attribute__((__packed__))` pero carecía del padding de 4 bytes antes de `st_size`. Esto causaba que `st_size` se ubicara en el offset `0x2C` en lugar de `0x30` (esperado por la ABI de 32 bits de Android Bionic en `MC_fsFileAttribute`).
- **Solución (`source/reimpl/io.h`):** Se insertó el campo `unsigned char __pad4[4];` alineando exactamente `st_size` al offset `0x30` (48 bytes), permitiendo que `GsFSFileSize` lea el tamaño real del archivo en el sistema de archivos de la Vita.
- **Estado Actual:** La lectura de metadatos de archivos de guardado está corregida, pero el flujo completo de carga de partidas guardadas in-game permanece pendiente de depuración detallada para una sesión posterior.

### Bug 13: Data Abort al liberar el toque en los menús de Estado/Equipo tras cargar partida (`CUIMenuStatus::PointerRelease` / `CUISubMenuReinForced::PointerRelease`)
- **Problema:** Dos capturas `.psp2dmp` idénticas (`logs/Advena-psp2core-1787009965-...` y `-1787013842-...`), reproducidas al soltar el dedo sobre los menús de Estado/Equipo poco después de que la partida guardada terminara de cargar, muestran el mismo Data Abort. El pseudo-C de Ghidra (`out_ghidra.c:173961-173970` y `:190423-190470`) confirma que ambos métodos desreferencian sin chequeo varios sub-objetos de la UI (`this+0xe0..0xf0` en `CUIMenuStatus::PointerRelease`; `this+0xd8` o `this+0x44` en `CUISubMenuReinForced::PointerRelease`, según el flag de `this+0xec`) que aún no están alocados mientras el menú se reconstruye desde el estado de la partida cargada.
- **Solución (`source/patch.c`):** Se hookearon ambos métodos (offsets `0xe8608` y `0xfcb78`). Si algún sub-objeto requerido es `NULL`, se omite la llamada al original en vez de crashear.
- **Corrección adicional durante la verificación:** el guard de `CUISubMenuReinForced::PointerRelease` leía el flag de `this+0xec` como `int` (4 bytes), pero el desensamblado real (`ldrb r3, [r0, r3]`) confirma que es un solo byte -- con basura en `this+0xed..0xef` el guard podía tomar la rama equivocada y dejar pasar el crash real. Se corrigió a una lectura de 1 byte (`uint8_t`).
- **Estado Actual:** Corregido y compilado (`build.sh Release`); pendiente de verificar en consola física que el flujo completo de carga de partida ya no aborta al interactuar con esos menús.

### Bug 14: Data Abort real al cargar partida (`CMapMgr::InitHero` -> `CCharObject::ClearMsgState`)
- **Problema:** El Bug 13 (arriba) se diagnosticó cruzando el reporte automático de `parse_dump.py`, cuya auto-detección de base de memoria del `.so` (voto por coincidencia de símbolos contra el contenido de la pila) resultó **incorrecta** (`0x980aa000` en vez de la real `0x98000000` = `LOAD_ADDRESS` de `source/utils/init.c`) para el registro `PC`/`LR` de este dump concreto -- llevando a atribuir el crash a `CUIMenuStatus`/`CUISubMenuReinForced::PointerRelease`, que son bugs reales pero no la causa de este `.psp2dmp`.
- **Diagnóstico correcto:** Se extrajeron directamente del `.psp2dmp` las páginas de memoria reales que contienen las direcciones `PC` y `LR` (`CoreParser.read_vaddr` sobre los segmentos que el propio dump captura) y se desensamblaron con la base real. La instrucción que crashea es `strb r2,[r3,#0x12]` en `.so+0x97474`, dentro de `CCharObject::ClearMsgState()` (`.so+0x9746c`), llamada por `bl` desde `CMapMgr::InitHero()` (`.so+0xca0a4`, decompilado en `out_ghidra.c:135905-135965`) -- la rutina que reposiciona al héroe justo después de cargar el mapa/partida. `ClearMsgState()` (decompilado en `out_ghidra.c:47947-47965`) desreferencia sin chequeo tres sub-objetos de `this` (`+0x5c`, `+0x64`, `+0x74`); en este crash `this+0x64` (icono de mensaje del héroe) es `NULL` porque esos sub-objetos de UI todavía no se reconstruyeron en ese instante del `Load Game`.
- **Solución (`source/patch.c`):** Se hookeó `CCharObject::ClearMsgState` (offset `0x9746c`). Si alguno de los 3 sub-objetos requeridos es `NULL`, se omite la llamada al original en vez de crashear.
- **Lección para el diagnóstico:** no confiar en la base auto-detectada de `parse_dump.py` cuando el desensamblado en PC/LR sale vacío o el offset resulta negativo -- señal de que la base es incorrecta para ese registro. Verificar cruzando con `CoreParser.read_vaddr` sobre las páginas reales que el propio dump capturó (siempre incluye las páginas de PC y LR) en vez de la heurística de votación por símbolos de la pila.
- **Estado Actual:** Corregido y compilado (`build.sh Debug`); pendiente de verificar en consola física.

### Bug 15: Corrupción en carga de partidas y pérdida aparente de progreso por desalineación de `stat64_bionic`
- **Problema:** Tras guardar partida, al intentar cargarla desde el menú el juego crasheaba (Data Abort), y al reiniciar el juego era como si nunca se hubiera guardado nada (los slots aparecían vacíos).
- **Causa Raíz:** En `source/reimpl/io.h`, la definición de `stat64_bionic` usaba los tipos `nlink_t`, `uid_t` y `gid_t` del newlib de VitaSDK con `__attribute__((__packed__))`. En VitaSDK estos tipos son de 16 bits (`uint16_t`, 2 bytes) en lugar de 32 bits (`uint32_t`, 4 bytes) como en el Bionic de Android 32-bit (ARM). Esto causaba que `st_size` quedara ubicado en el offset `0x2A` (42 bytes) en lugar del offset `0x30` (48 bytes) que la función nativa del `.so` `MC_fsFileAttribute` lee (`ldr r3, [sp, #48]`).
  - Como consecuencia, `GsFSFileSize()` siempre leía `0` bytes para cualquier archivo existente (`s0.dat`, `g.dat`).
  - Al cargar partida, `CGsEncryptFile::LoadBegin()` creía que el archivo estaba vacío (`fileSize == 0`), no alocaba su buffer (`buffer == NULL`) y `LoadPlayData()` leía ceros en toda la memoria del juego, dejando al jugador en `NULL` y crasheando en el primer autosave / cambio de mapa.
  - Al reiniciar el juego, `tagGameData::Load()` leía `GsFSFileSize("g.dat") == 0` y re-inicializaba todos los slots como si no hubiera partidas guardadas.
- **Solución:**
  1. En `source/reimpl/io.h`, se redefinió `stat64_bionic` con tipos explícitos de ancho fijo (`uint32_t` para `st_nlink`, `st_uid`, `st_gid`, y paddings explícitos `__pad0`..`__pad5`), asegurando que `st_mode` quede en offset `0x10` (16) y `st_size` en offset `0x30` (48), idéntico al ABI de Android Bionic 32-bit.
  2. En `source/reimpl/bits/_struct_converters.c`, se actualizó `stat_newlib_to_bionic()` para hacer `memset(0)` y asignar los timestamps correctamente.
  3. En `source/reimpl/io.c`, se corrigió la llamada a `ftell()` en `fclose_soloader()` usando `sceLibcBridge_ftell()` cuando `USE_SCELIBC_IO` está activo.
- **Estado:** Corregido y compilado en Release y Debug.

### Bug 12: Pipeline de visualización y adaptación de resolución completa (480x320 -> 960x544)
- **Problema:** Al intentar adaptar la pantalla a la PS Vita, se produjeron desalineaciones y cortes de bordes (pérdida de la barra de retratos de héroes superior en combate y del botón de cierre [X] en menús/opciones) si la altura interna se reducía a 240px o si se usaban offsets arbitrarios.
- **Solución:**
  1. Se verificó que el canvas lógico nativo de Advena en Android es de **480x320** (`AdvenaLauncher.java`).
  2. Se configuraron `NativeInitWithBufferSize(480, 320)` y `NativeInitDeviceInfo(480, 320)` para que el motor renderice el 100% de los elementos y capas de UI en el buffer interno.
  3. Se configuró `NativeResize(960, 544)` (`glResize`) para mapear el viewport y matriz ortográfica a la resolución nativa completa de la PS Vita (960x544), logrando que el juego ocupe toda la pantalla de extremo a extremo sin pérdida de información visual ni cortes de interfaz.

---

## 4. Fases del Port (Checklist Actualizado)

- [x] **Fase 0: Preparación del Entorno**
  - [x] Repositorio Git inicializado, `.gitignore` anti-DMCA.
  - [x] `porting_tools/` adaptado (`manage_vita.py`, scripts de análisis y build).
  - [x] Submódulo / librerías `falso_jni`, `so_util`, `libc_bridge`, `fios`, `kubridge`, `sha1`.
  - [x] Decompilación completa: Java (`decompiled/apk_jadx/`) y Ghidra C (`decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c`).

- [x] **Fase 1: Configuración del Loader y Mapeo de Símbolos (`source/dynlib.c`)**
  - [x] Resolver todos los símbolos dinámicos (`libc`, `libm`, `pthreads`, `GLES1`, `io`, `sockets`).
  - [x] Implementar hooks de redirección de I/O a `ux0:data/advena/assets/`, `res/` y `ux0:data/advena/saves/`.
  - [x] Cargar `libgameDSO.so` mediante `so_file_load` / `so_relocate` / `so_resolve`.

- [x] **Fase 2: Implementación de la Tabla JNI (`source/java.c`)**
  - [x] Implementar handlers de `isAssetExist`, `readAssete`, `readAssets`, `loadFileFromStorage`.
  - [x] Implementar sistema de guardado seguro (`isFileExist`, `saveFile`, `loadFile`, `deleteFile`) en `ux0:data/advena/saves/`.
  - [x] Handlers de información de dispositivo (`getPhoneModel`, `getDeviceID`, `getLocaleID`, etc.).
  - [x] Handlers de timers y estado de UI (`OnAsyncTimerSet`, `OnUIStatusChange`, `getVPoint`).

- [x] **Fase 3: Pipeline de Renderizado Gráfico (`source/main.c`)**
  - [x] Inicializar ventana y contexto `vitaGL` en 960x544.
  - [x] Configuración del ciclo nativo: `InitializeJNIGlobalRef` -> `NativeInitWithBufferSize(480, 320)` -> `NativeInitDeviceInfo(480, 320)` -> `NativeResize(960, 544)`.
  - [x] Bucle principal de renderizado invocando `NativeRender()` y `gl_swap()`.

- [x] **Fase 4: Input Táctil y Controles Físicos**
  - [x] Soporte para pantalla táctil frontal mapeada a canvas 480x320 (`handleCletEvent` con eventos 23=Down, 25=Move, 24=Up).
  - [x] Mapeo completo de botones físicos de la PS Vita:
    - D-Pad / Stick Analógico Izquierdo -> Movimiento (KeyCodes 50, 56, 52, 54).
    - Botón Cruz (✕) -> Ataque / Confirmar / Hablar con NPC (Key 53).
    - Botón Círculo (◯) -> Salto adelante (Key -4).
    - Botón Triángulo (△) -> Salto atrás (Key -3).
    - Botón Cuadrado (□) / Stick Derecho -> Skills 1..4 (Keys -13, 42, -14, 35).
    - Gatillos L1 / R1 -> Teamstrike [T] (Key 48) y Tag Switch (Key -12).
    - Start -> Menú de Pausa (Key -8).
    - Select -> Mochila / Inventario / Cancelar (Key -16).

- [x] **Fase 5: Sistema de Audio (`source/audio.c`)**
  - [x] Mezclador de audio nativo para PS Vita (`SceAudioOut`).
  - [x] Reproductor de BGM en hilo secundario (soporte para archivos Ogg Vorbis vía `Tremor` / `libvorbisidec`).
  - [x] Canales de efectos de sonido SFX (`s000.ogg` .. `s076.ogg`).
  - [x] Enlace con callbacks JNI `OnSoundPlay(sndID, vol, isLoop)` y `OnStopSound()`.

- [x] **Fase 6: LiveArea, Empaquetado VPK y Build System**
  - [x] Configurar assets de LiveArea (`icon0.png`, `bg0.png`, `pic0.png`, `startup.png`, `template.xml`).
  - [x] Custom commands en `CMakeLists.txt` compatibles con rutas con espacios para generar `param.sfo` y `advena.vpk`.
  - [x] Validación y compilación limpia con VitaSDK (`advena.elf`, `eboot.bin`, `advena.vpk`).

- [x] **Fase 7: Guardado de Partida — Confirmación en Consola Física**
  - [x] Verificado en consola real: el flujo completo de Guardar -> Salir -> Cargar partida ya no crashea y conserva el progreso. Cierra el ciclo abierto por los Bugs 1, 2, 11, 13, 14 y 15 (redirección de rutas, alineación de `stat64_bionic`, y los guards de `NULL` en `CUIMenuStatus`/`CUISubMenuReinForced::PointerRelease` y `CCharObject::ClearMsgState`).

### Bug 16: Rendimiento — caída a ~15 FPS o menos en combate/mapas cargados

- **Síntoma:** El framerate cae a 15 FPS o menos en ciertas escenas, reportado por el usuario tras confirmar que el guardado ya funciona.
- **Diagnóstico:** A diferencia de Zenonia 4 (mismo motor Gamevil Nexus2/GxPZx, pero renderizado 100% por software con `PutCompressImg`/`CGxPZxZero::Blt` subiendo un framebuffer RGB565 completo), Advena usa el pipeline **OpenGL ES 1.1 de función fija nativo del `.so`** (`glDrawArrays`, `glDrawElements`, `glTexImage2D` resueltos directamente en `source/dynlib.c`), traducido por `vitaGL`. Esto descarta de raíz varias de las técnicas de `optimization_ideas.md` de Zenonia 4 que son específicas de su renderizador por software (parche de `PutCompressImg`, GPU Quads/decodificación dual, dirty rectangles, downsampling entrelazado) — no aplican aquí porque no existe ese framebuffer de software que interceptar.
- **Comparación de las técnicas genéricas de esa lista (no ligadas al software renderer) contra el estado de Advena:**
  - Overclock de CPU/GPU/Bus (`scePowerSetArmClockFrequency(444)` + Bus/Gpu/GpuXbar) — **ya presente** en `source/utils/init.c`, idéntico a Zenonia 4. Sin cambios.
  - Pool de memoria de `vitaGL` y ausencia de MSAA/triple buffer (`vglInitExtended(0, 960, 544, 6*1024*1024, SCE_GXM_MULTISAMPLE_NONE)`) — **ya presente**, valor idéntico al usado en Zenonia 2/3 y Prince of Persia (mismo motor), documentado en `source/utils/glutil.c`. Sin cambios.
  - Audio en hilo dedicado sin bloquear el hilo principal (`audio_thread` en `source/audio.c`) — **ya presente**. Sin cambios.
  - Intercepción NEON de `memcpy`/`memset` (idea #4) — **descartada a propósito**: medida en consola real en Zenonia 4 (Fase 35) sin mejora de FPS detectable (9.54 vs 9.6 FPS, dentro del ruido). No se replica aquí.
  - Desactivar VSync (idea #9) — **descartada a propósito**: en Zenonia 4 se confirmó en consola que no era el techo real de FPS (el render por software ya excedía el frame budget). El pacing actual de Advena (`sceDisplayWaitVblankStartMulti(2)`, ~30 FPS máx.) tampoco es sospechoso de ser el cuello de botella cuando el juego ya cae a 15 FPS o menos (muy por debajo del cap).
  - **Afinidad de núcleos de CPU (idea #6)** — **NO estaba presente en Advena y sí se aplicó ahora**: se fijó el hilo principal (render + lógica, `NativeRender()`) al núcleo `SCE_KERNEL_CPU_MASK_USER_0` en `source/main.c` (justo después de `gl_init`) y el hilo mezclador de audio al núcleo `SCE_KERNEL_CPU_MASK_USER_1` en `source/audio.c` (justo antes de `sceKernelStartThread`), replicando exactamente la implementación de Zenonia 4 (`loader/main.c`/`loader/audio.c`, Fase 36 de ese port). Es una optimización estándar sin flag de build, sin riesgo de regresión funcional — evita que el scheduler migre el hilo de render entre núcleos mientras compite con el mezclador de audio.
- **Pendiente (requiere verificación en consola física, no se puede completar sin hardware ni telemetría real):**
  1. Medir el FPS real antes/después del fix de afinidad con `perf-telemetry` de `psvita-toolkit` (o un contador simple en el loop de `main.c`) para cuantificar el impacto real, igual que se hizo en cada paso de `optimization_ideas.md` de Zenonia 4 — no asumir mejora sin medir.
  2. Si la caída a 15 FPS persiste, el candidato de mayor impacto (según la experiencia de Zenonia 4, idea #1 confirmada con datos reales: 445–547 llamadas/frame a `PutCompressImg` en combate) sería identificar el hot path equivalente en Advena vía el mismo método de instrumentación (contadores tras un flag `INSTRUMENT_*_CALLS`, build de prueba dedicado) antes de tocar ningún parche — Advena comparte las clases `CGxPZxMgr`/`CGxPZxFrame` del mismo motor, pero al usar GL real en vez de blit por software, el cuello de botella más probable aquí es **conteo de draw calls / cambios de estado GL por frame** (miles de `glDrawArrays` con un solo quad cada uno, típico de un port directo de un motor de función fija), no el software rasterizer. Instrumentar `glDrawArrays`/`glDrawElements`/`glBindTexture` en `source/dynlib.c` para contar llamadas y cambios de textura por frame sería el equivalente directo al probe de `PutCompressImg` de Zenonia 4.

- **Segunda pasada de perf (2026-08-23) — cambios aplicados tras revisar una lista externa de mejoras contra el código y este mismo diagnóstico:**
  - **Aplicado:** Eliminado el `sceDisplayWaitVblankStartMulti(2)` explícito tras `gl_swap()` en `source/main.c`. `vglSwapBuffers()` ya hace su propio flip/sync de vblank internamente; el wait adicional forzaba un piso de ~33.3ms por frame SIEMPRE, incluso en escenas donde el render ya terminaba en <17ms, capando innecesariamente a 30 FPS. No es un fix de este Bug 16 (las escenas de combate ya caen muy por debajo del cap, como se documentó arriba) pero sí mejora el techo de FPS en menús/mapas livianos.
  - **Aplicado:** Pool inmediato de `vglInitExtended` subido de 6MB a 8MB en `source/utils/glutil.c` (headroom para los `glDrawArrays`/`glDrawElements` de arrays de vértices client-side que emite el `.so`). MSAA_NONE y `vglUseTripleBuffering(GL_FALSE)` se dejaron intactos a propósito — siguen siendo la config validada en Zenonia 2/3, Prince of Persia y esta misma investigación.
  - **Aplicado:** Instrumentación de Bug 16 pendiente (punto 2 arriba) implementada: flag de CMake `INSTRUMENT_GL_CALLS` (OFF por defecto) que resuelve `glDrawArrays`/`glDrawElements`/`glBindTexture` a wrappers contadores en `source/dynlib.c`/`source/utils/glutil.c`, reportando draws/binds/cambios de textura por frame vía `game_log()`. Falta: compilar con `-DINSTRUMENT_GL_CALLS=ON`, correr en consola real durante combate, y leer el log — ese dato es el que decide el próximo parche (no se puede completar sin hardware).
  - **Aplicado:** `-mcpu=cortex-a9 -mfpu=neon` añadido a los flags de compilación en `CMakeLists.txt` (el toolchain de Vita no los fija por sí solo). Verificado con una build de Release real.
  - **Descartado — `-flto`:** rompe el link (`lto1: fatal error` al fusionar traducciones) por el patrón de `dynlib.c` de declarar símbolos de runtime de C++ (`operator new`, `__cxa_throw_bad_alloc`, etc.) como `extern void *` para la tabla de resolución de imports; bajo LTO, GCC los cruza contra los prototipos reales de libstdc++ y falla. Confirmado con una build real, no es teórico.
  - **Descartado — Triple buffering / `vglUseDirectBuffer`:** activar triple buffering se descartó por ser justamente la config ya validada como "sin cambios" arriba; `vglUseDirectBuffer` ni siquiera existe en el `vitaGL.h` de este VITASDK (no aplica).
  - **Descartado — Hook NEON de `DrawOP_ENLARGE_Compress_16_Ex`:** esa función SÍ existe en el `.so` (decompilada en `out_ghidra.c`), pero es del software rasterizer del motor compartido (misma familia que `PutCompressImg` de Zenonia 4) — no el pipeline GL real que usa Advena. Reescribirla a ciegas sin poder verificar corrección en consola (matemática de blend RGB565 empaquetada bit a bit) es alto riesgo para un hot path no confirmado; queda descartada hasta tener datos del punto de instrumentación de arriba.
  - **Aplicado (menor):** `AUDIO_GRAIN` subido de 512 a 1024 en `source/audio.c` (menos wakeups del hilo de audio, ~11.6ms más de latencia, imperceptible para BGM/SFX). De los 70 `.ogg` en `ux0_data/advena/sound/`, solo `s048.ogg` no estaba a 44100Hz (estaba a 22050Hz); resampleado a 44100Hz/estéreo con ffmpeg (backup en `s048.ogg.orig`). Los otros 69 ya estaban correctos — no hacía falta un resampleo masivo.
  - **Descartado — logging en hot path de input:** ya es un no-op en builds Release (`l_info`/`l_warn`/etc. se compilan a nada sin `DEBUG_SOLOADER` en `source/utils/logger.h`); no había nada que arreglar.

- **Tercera pasada de perf (2026-08-25) — el usuario confirma mejora leve tras la segunda pasada, pero sigue habiendo caídas de FPS con 10+ enemigos atacando simultáneamente:**
  - **Aplicado:** `vglUseCachedMem(GL_TRUE)` antes de `vglInitExtended` en `source/utils/glutil.c` (`gl_init`). El `.so` sube arrays de vértices client-side reales (`glVertexPointer`/`glTexCoordPointer` + `glDrawArrays`/`glDrawElements` reales, no VBOs) en cada frame; por defecto vitaGL usa memoria no cacheada para sus pools internos (escritura de CPU lenta, lectura de GPU rápida). Con 10+ enemigos atacando a la vez eso es mucho volumen de escritura de CPU por frame hacia esos pools -- el escenario textbook para el que existe esta opción. vitaGL maneja el flush de caché hacia la GPU internamente. Cambio aditivo, reversible con un solo flag, sin tocar el pipeline GL en sí. Verificado con una build Release real (compila y linkea sin cambios).
  - **Revisado y descartado por ahora — `vglUseVramForUSSE(GL_TRUE)`:** existe en este `vitaGL.h` y por defecto está en `GL_FALSE`. Se descartó activarlo en esta pasada: consumiría VRAM adicional (ya usada primero por defecto para texturas vía `vglUseVram`, que ya está en `GL_TRUE` por defecto en esta vitaGL -- sin cambios ahí) justo en el escenario de más presión de VRAM (muchos enemigos con texturas de animación cargadas a la vez); sin poder medir en consola el balance real de VRAM libre durante combate, es un cambio de riesgo no justificado todavía.
  - **Sigue pendiente (requiere consola física, no se puede completar sin hardware):** el punto 2 de la segunda pasada -- compilar con `-DINSTRUMENT_GL_CALLS=ON`, reproducir una pelea con 10+ enemigos atacando a la vez, y leer `[PERF] GL/frame: draws=... binds=... tex_switches=...` del log. Ese dato sigue siendo el que decide si el cuello de botella real es conteo de draw calls (querría explorar agrupar/reducir llamadas, mucho más invasivo) vs. bandwidth de subida de vértices (lo que este cambio ya ataca) vs. otra cosa. Sin esa medición, cualquier parche adicional sobre el pipeline GL sería a ciegas.

- **Cuarta pasada de perf (2026-08-25, mismo día) — el usuario reporta que el juego sigue bajando hasta 7 FPS con 10+ enemigos atacando; consola física disponible y accesible por FTP durante esta sesión, se completó la medición pendiente de la 3ª pasada Y se corrigió el diagnóstico original de este Bug 16:**
  - **CORRECCIÓN IMPORTANTE al diagnóstico original de este Bug 16 (arriba, sección "Diagnóstico"):** se asumió que Advena "usa el pipeline OpenGL ES 1.1 de función fija nativo del `.so`" (glDrawArrays/glDrawElements/glTexImage2D resueltos directamente) y que por eso "no existe ese framebuffer de software que interceptar" a diferencia de Zenonia 4. **Esa asunción era incorrecta** -- se basaba solo en que `source/dynlib.c` resuelve esos símbolos GL hacia vitaGL, sin medir nunca cuántas veces se llaman de verdad. Compilando con `-DINSTRUMENT_GL_CALLS=ON`, desplegando por FTP (`psvita-toolkit deploy --eboot`) y jugando en consola real (logs `advena_036.log`/`advena_037.log`, bajados por FTP con `curl ftp://192.168.3.15:1337/...`), se midió **`draws=1, binds=1` por frame, constante durante TODA la sesión de juego incluyendo combate**, con esa única llamada subiendo un buffer completo de **480x320 = 153600 píxeles vía `glTexSubImage2D` en cada frame** (agregada la instrumentación de `glTexImage2D`/`glTexSubImage2D` en esta misma pasada). Esto prueba que Advena, igual que Zenonia 4, compone la escena por software en un framebuffer interno de 480x320 y solo toca GL para subir/presentar ESE único buffer como un quad de pantalla completa -- no hay draw calls por sprite. La memoria de sesión (`advena_vita_rendering_vs_zenonia4.md`) que decía "diferente pipeline de render, no aplican las técnicas de Zenonia4" quedó corregida.
  - **Confirmación del hot path real -- `PutCompressImg` (`.so+0x140050`):** `nm -D` sobre el `.so` real muestra que Advena exporta la misma función `PutCompressImg(int,int,int,int,unsigned char*,unsigned short*,enumDrawOP,int,long)` que Zenonia 4 usó para su propio probe de hot-path ya validado (mismo motor GxPZx). Es el dispatcher común que reparte cada blit de sprite a la variante `DrawOP_<BLEND>_<Clipping>Compress_16_<Formato>` correcta (~20 variantes exportadas: COPY/ADD/BLEND16/DARKEN/ENLARGE/FX × Compress_16_16/_Ex/_Alpha/_Auto × con/sin Clipping). Se agregó un hook de solo-conteo (no destructivo, siempre ejecuta el original sin modificarlo -- infraestructura nueva en `source/patch.c`/`source/utils/init.h`, gateada por el nuevo flag de CMake `INSTRUMENT_BLIT_CALLS`) sobre `PutCompressImg`. Primer intento equivocado: se hookeó `DrawOP_ENLARGE_Compress_16_Ex` (`.so+0x136034`, identificada en el Bug 5 de este documento) asumiendo que era el blit genérico de sprites -- midió **0 llamadas** durante toda una sesión con combate real, confirmando que esa variante específica (ENLARGE) no es la que usan los sprites de gameplay; se reemplazó por el hook sobre `PutCompressImg` (el dispatcher común, no una variante puntual).
  - **Datos medidos en consola real (log `advena_039.log`, sesión con zona tranquila + combate con 10+ enemigos, mismo playthrough):** `PutCompressImg` pasa de **~0 llamadas/frame en menús** a **~577-650 llamadas/frame caminando** (zona tranquila, ya no es gratis: tiles + fondo + UI) a **~750-800 llamadas/frame en combate normal**, con **picos de hasta 1975 llamadas/frame** cuando hay muchos enemigos atacando a la vez (`n=2394` frames muestreados, `avg=622.8`, `max=1975`). Eso es un multiplicador de ~3.2x sobre el baseline caminando en el peor pico -- consistente con la caída reportada de framerate normal a 7 FPS (si el costo de CPU escala aprox. linealmente con la cantidad de blits, un frame que costaba X ms pasa a costar ~3.2X ms en el pico). Mismo patrón que el hot-path de Zenonia 4 (`PutCompressImg`, 445-547 llamadas/frame en combate), solo que con números base más altos.
  - **Conclusión:** el cuello de botella real de Bug 16 es el compositor de sprites por software del `.so` (`PutCompressImg` + familia `DrawOP_*`), NO el pipeline GL (que ya está optimizado: 1 solo draw call, 1 sola subida de textura por frame). Las técnicas de `optimization_ideas.md` de Zenonia 4 dirigidas a ESE renderizador (parcheo NEON del blit, dirty rectangles, hot-path call counting ya replicado aquí) sí son aplicables a Advena después de todo -- la descarte previa de esas técnicas en este documento (secciones anteriores de este mismo Bug 16) se basaba en el diagnóstico incorrecto de arriba y debe reconsiderarse.
  - **Pendiente para la próxima pasada (requiere más trabajo, no trivial):** todavía no se sabe CUÁL(es) de las ~20 variantes `DrawOP_*` es la que realmente ejecuta `PutCompressImg` durante combate (el parámetro `enumDrawOP` que recibe determina la variante interna) -- sería el siguiente paso de instrumentación (bucketear conteos por valor de `enumDrawOP` en el mismo hook) antes de intentar cualquier reescritura NEON, dado que cada variante tiene su propia matemática de blend RGB565 empaquetada bit a bit y un patch a ciegas sobre la variante equivocada no tocaría el hot path real. Evaluar también si hay una optimización más simple y de menor riesgo disponible antes de tocar el blend math (p. ej. culling de sprites fuera de pantalla, si el `.so` ya lo hace o no).

- **Quinta pasada de perf (2026-08-25, misma sesión) -- identificación exacta de la variante `DrawOP_*` dominante:**
  - **Cómo se resolvió el pendiente de arriba:** se encontró `CMvGraphics::InitialBlend()` (`out_ghidra.c:214383-214425`), la rutina de init que registra cada valor de `enumDrawOP` contra su par concreto de funciones vía `SetZeroBlendFunc(clave, DrawOP_<NOMBRE>_Compress_16_Auto+1, DrawOP_<NOMBRE>_ClippingCompress_16_Auto+1)`. Esto da el mapeo completo y exacto `enumDrawOP -> nombre` sin necesidad de adivinar: `0x0=COPY, 0x1=BLEND16, 0x2=ADD, 0x4=VOID, 0x5=SHADOW, 0x6=LIGHTEN, 0x7=DARKEN, 0x9=NEGATIVE, 0xa=GRAY, 0xb=RGB, 0xc=RGBHALF, 0xd=RGBADD, 0xe=RGBMULTI, 0xf=OUTLINE, 0x10=ENLARGE, 0x13=FX`. De paso esto explica por qué el primer intento de esta pasada (hookear `DrawOP_ENLARGE_Compress_16_Ex`, con sufijo "_Ex") midió 0 llamadas: la tabla de despacho de `PutCompressImg` SOLO conecta las variantes con sufijo "_Auto" -- "_Ex" no está cableada desde este call path (puede seguir siendo alcanzable desde otro lado, ver Bug 5, pero no desde el compositor de sprites).
  - **Implementado:** se extendió el hook de `PutCompressImg` (`source/patch.c`) con un histograma por `enumDrawOP` (`instr_blit_by_op[32]`, reseteado cada frame) y una función que lo formatea como texto compacto (`patch_format_and_reset_blit_histogram`), agregado al log `[PERF]` de `gl_instrument_frame_end()` (`source/utils/glutil.c`) como campo `ops=NOMBRE:cuenta,...`. Sigue siendo puramente diagnóstico (no cambia comportamiento).
  - **Datos medidos en consola real (log `advena_040.log`, zona tranquila + combate, ~547 frames muestreados):** sumando todos los frames, la distribución de operaciones es **`COPY=180371 (79.3%), DARKEN=14031 (6.2%), BLEND16=12935 (5.7%), ADD=10805 (4.8%), FX=4307 (1.9%), RGBHALF=1969, LIGHTEN=1687, RGB=795, RGBMULTI=506, RGBADD=28`** (total 227434 blits). `COPY` domina de forma aplastante -- casi 4 de cada 5 blits de sprite en esta sesión son `DrawOP_COPY_Compress_16_Auto`/`DrawOP_COPY_ClippingCompress_16_Auto` (probablemente tiles de fondo, sprites opacos de personajes/enemigos y elementos de UI que no necesitan blending), muy por delante de cualquier variante con blending real.
  - **Conclusión / objetivo concreto para la próxima optimización real:** `DrawOP_COPY_Compress_16_Auto` + `DrawOP_COPY_ClippingCompress_16_Auto` son el objetivo de mayor ROI para una reescritura NEON -- además de ser la variante más llamada por lejos, `COPY` es matemáticamente la más simple de las ~20 variantes (descompresión + copia directa de píxeles, sin aritmética de blend/alpha como sí requieren `BLEND16`/`ADD`/`RGB`/etc.), lo que la hace también la de MENOR riesgo para optimizar a mano sin corromper el render.
  - **Pendiente (trabajo real de ingeniería, no instrumentación -- recomendado para una sesión dedicada aparte):** leer el pseudo-C completo de `DrawOP_COPY_Compress_16_Auto` en `out_ghidra.c`, entender el formato exacto de descompresión RGB565 que usa, y escribir/validar en consola física una versión NEON equivalente byte-exacto antes de reemplazarla. No intentar esto a ciegas ni sin poder verificar visualmente en hardware real que no corrompe sprites.

- **Sexta pasada de perf (2026-08-25, misma sesión) -- implementación real: reemplazo de `DrawOP_COPY_Compress_16`, verificado parcialmente en consola:**
  - **Formato descomprimido, entendido de `out_ghidra.c:261348-261434` (`DrawOP_COPY_Compress_16`, la función a la que realmente cae `DrawOP_COPY_Compress_16_Auto` en el caso común):** stream de tokens `uint16` little-endian por scanline: header extendido opcional de 10 bytes si el primer `int16` del stream es `-5` (`0xFFFB`); luego, por token: `0xFFFF` = fin de sprite; `0xFFFE` = fin de fila (`dst += stride`, no consume bytes de datos); bit alto seteado (`token & 0x8000`) = "run opaco" de `token & 0x7FFF` píxeles, cada uno leyendo 1 byte índice del stream y escribiendo `paleta[índice]` (RGB565) en destino; si no, "run transparente" de `token` píxeles (`dst += token`, no lee ni escribe nada -- son los píxeles ya transparentes del sprite).
  - **Por qué NO se usó NEON literal para el loop caliente (contra lo que pedía el nombre original de esta tarea):** el trabajo por-píxel del run opaco es un lookup índice-de-8-bits -> paleta-de-256-entradas-de-16-bits -- un patrón de *gather*. ARMv7 NEON no tiene instrucción de gather; `VTBL` solo cubre tablas de 32 bytes/32 entradas, 8x más chico que esta paleta de 256 entradas. Encadenar 8 pasadas de `VTBL` con selección condicional para cubrir las 256 entradas es real trabajo de ingeniería con riesgo de corrección real, sin garantía de ganar contra la alternativa de abajo. Se decidió no intentarlo sin datos que lo justifiquen.
  - **Qué se hizo en cambio (la ganancia real, de menor riesgo):** se comparó el pseudo-C contra el disassembly REAL del `.so` (`arm-vita-eabi-objdump -Mforce-thumb` en `.so+0x142588`) y se confirmó que el compilador original (target armeabi/ARMv5TE, Thumb-1 puro) **derrama el contador del run opaco a la pila** y lo recarga con `ldrh`/`strh` en casi cada iteración del loop, en vez de mantenerlo en un registro -- ~9-10 instrucciones Thumb-1 por píxel para lo que debería ser 3-4. Se escribió una reimplementación en C, byte-exacta contra el pseudo-C de arriba (`DrawOP_COPY_Compress_16_rewrite`, `source/patch.c`), que se compila con los flags YA existentes de este proyecto (`-O3 -mcpu=cortex-a9 -mfpu=neon`, `CMakeLists.txt`) -- mismo algoritmo, compilador moderno con mejor asignación de registros, sin cambios de comportamiento.
  - **Implementación (`source/patch.c`, `source/dynlib.c` no se tocó):** hook de **reemplazo total** (a diferencia de todos los demás hooks de este archivo, que restauran-llaman-reparchean para ejecutar el original) sobre `DrawOP_COPY_Compress_16` (`.so+0x142588`) -- el código máquina original NO se vuelve a ejecutar nunca más tras instalar el hook. Gateado detrás de un flag de CMake nuevo y explícitamente apagado por defecto: `OPT_COPY_BLIT_REWRITE` (`OFF` por defecto -- recién se activa a mano hasta tener confianza suficiente). `DrawOP_COPY_Compress_16_Auto` (el dispatcher, offset 0x131544) NO se tocó -- sigue despachando al 5-entry function-pointer table para headers especiales exactamente igual que antes, y cae a la versión reescrita solo en el caso común (la inmensa mayoría de las llamadas).
  - **Verificación en consola real (build `-DOPT_COPY_BLIT_REWRITE=ON`, desplegada por FTP):** el usuario reportó **"mejoró la velocidad fuera de combate"** y **"no noté errores gráficos de momento"** tras jugar caminando, en menús y en combate -- primera señal positiva real, aunque no es una verificación exhaustiva de todo el contenido del juego (mapas/jefes/cinemáticas no cubiertos todavía). El bajón de FPS específicamente **al golpear/colisionar sigue existiendo** pese al reemplazo.
  - **Investigación del bajón al golpear/colisionar (inconclusa):** se repitió la instrumentación de la 5ª pasada (`INSTRUMENT_GL_CALLS` + `INSTRUMENT_BLIT_CALLS`) combinada con `OPT_COPY_BLIT_REWRITE=ON`, pidiendo al usuario reproducir varios golpes/colisiones seguidos. El log resultante (`advena_041.log`) muestra la MISMA distribución de operaciones que antes (`COPY` ~79%, sin pico distinto en el momento del golpe) y un pico máximo de 660 blits/frame -- muy por debajo del pico de 1975 medido en la sesión original con 10+ enemigos. **No se puede concluir todavía** si el bajón al golpear es (a) el mismo cuello de botella de volumen de blits en un pico más fuerte que esta repro puntual no llegó a alcanzar, o (b) un cuello de botella DISTINTO fuera del pipeline de render (lógica de colisión/IA, disparo de sonidos, spawn de números de daño, etc.) que esta instrumentación (que solo mide `PutCompressImg`/GL) no puede ver. **Pendiente para una sesión futura:** reproducir golpes/colisiones CON 10+ enemigos simultáneos (la densidad de la repro original) mientras está activa esta misma instrumentación, para saber si el pico de blits en ese instante puntual iguala o supera el ~1975 ya visto (confirmaría que sigue siendo el mismo cuello de botella de render) o se mantiene bajo (apuntaría a lógica de juego, no render, como causa del bajón puntual de golpe/colisión -- requeriría un método de instrumentación distinto, p. ej. contar/cronometrar las rutinas de colisión/daño en vez de las de blit).
  - **Estado actual:** `OPT_COPY_BLIT_REWRITE` queda disponible pero apagado por defecto en `build.sh release` sin flags -- no se lo considera listo para ser el comportamiento por defecto hasta tener una verificación visual más exhaustiva (distintos mapas, jefes, cinemáticas) confirmada por el usuario.
