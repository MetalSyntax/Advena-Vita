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
