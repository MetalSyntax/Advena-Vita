# Herramientas de porting Android → PS Vita

Recopiladas a lo largo de dos ports anteriores -- Zenonia 4 (motor Gamevil Nexus2, mismo namespace
JNI `Java_com_gamevil_nexus2_Natives_*` que Advena, ver `PORTING_PLAN.md`) y Prince of Persia Classic
(motor Cocos2d-x, no relacionado) -- y adaptadas para este port de Advena. Todas asumen un proyecto
so-loader (vitasdk + FalsoJNI) similar a este; lo que siga referenciando el port anterior en vez de
Advena está marcado explícitamente en el propio script/comentario.

## Build y despliegue

- **`build/build_and_install.sh`** — build completo vía symlink sin espacios (workaround del bug de
  `vita-pack-vpk` con rutas que contienen espacios) + instalación. Asume `project(advena)` en
  `CMakeLists.txt` (VPK/ejecutable `advena`); si el proyecto agrega flags de build experimentales,
  agregar su descripción corta en `flag_desc()`.
- **`manage_vita.py`** — herramienta de despliegue y debug por FTP (VitaShell): opción 1 escanea
  `BUILD_DIR` (default `build/`) y deja elegir entre TODOS los `.vpk` encontrados (útil si el proyecto
  genera varias variantes de build, ver `build.sh`) en vez de subir un archivo fijo; también descarga el
  log más reciente de `ux0:data/advena/logs/`. `VITA_IP`, `VITA_TITLEID` (`ADVENA001`) y los paths
  `ux0:data/advena/...` ya están seteados al inicio del archivo -- revisar si `VITA_IP` sigue apuntando
  a la Vita correcta.
- **`build/deploy_and_launch_vita3k.sh`** — para pruebas en el emulador Vita3K (macOS): copia el
  `eboot.bin` recién compilado directo al directorio de la app instalada (sin reinstalar el `.vpk`
  completo) y relanza el emulador. Requiere `/tmp/advena-src` y `/tmp/advena-build` ya configurados
  (ver comentario de cabecera del script).
- **`misc/get_dump.sh`** — descarga el `.psp2dmp` (core dump) más reciente de la consola por FTP.
  Uso: `./get_dump.sh <IP-de-la-vita>`.

## Automatización de clics en Vita3K (macOS)

Vita3K usa una UI Qt que **no** responde a clics sintéticos de accesibilidad (`osascript`/AppleScript) —
hace falta inyectar eventos reales de mouse a nivel de sistema operativo vía Quartz.

- **`automation/click_helper.py`** — clic (simple o doble) en una coordenada de pantalla dada.
- **`automation/hold_click.py`** — mueve, presiona y mantiene el botón del mouse por N segundos.
- **`automation/mousedown_only.py`** / **`automation/mouseup_only.py`** — presionar/soltar por separado
  (para simular un drag entre dos invocaciones).
- **`automation/key_helper.py`** — presiona una tecla del teclado por su nombre (mapa de keycodes de macOS).

Requieren `pip install pyobjc` (para el módulo `Quartz`).

## Decompilación

- **`build/decompile_all.sh`** — corre Jadx (`Advena-1.0.1.apk` → Java) y devrvk/so-decompiler
  (`Advena-1.0.1/lib/*/*.so` → pseudo-C) vía Docker. Ya apunta a las rutas reales de este proyecto.
- **`ai_bash_commands.sh`** — plantilla de comandos de inspección (símbolos libpng/Decode vía
  `readelf`) sobre `libgameDSO.so`. Los offsets de disassembly puntual del port anterior (Zenonia 4) se
  quitaron a propósito -- son de OTRO binario y hay que re-derivarlos para Advena antes de usarlos.
- **`translate_shaders.py`** — punto de partida para limpiar el boilerplate GLES de un shader volcado
  (`glsl_dump/*.glsl` → `assets/cg/*.cg`), no una traducción GLSL→Cg completa: cada shader nuevo todavía
  necesita revisión/reescritura a mano (ver `PORTING_PLAN.md`).

## Testing y utilidades varias

- **`tests/run_tests.sh`** — **deshabilitado** para Advena por ahora: heredado de Prince of Persia
  Classic (motor Cocos2d-x, rutas de audio como strings literales en el `.so`), pero Advena usa un motor
  distinto (Gamevil Nexus2) donde esa convención de rutas no aplica -- ver el comentario de cabecera del
  script para qué hace falta confirmar antes de reactivarlo.
- **`build/clean_macos.sh`** — borra archivos basura `._*` que macOS genera en unidades no-HFS+ (USB/red),
  que pueden confundir herramientas de empaquetado si se cuelan en el build.
- **`misc/translate_docs.py`** — traduce archivos Markdown en lote con `deep_translator`
  (Google Translate). Requiere `pip install deep-translator`.
- **`parse_dump.py`** — wrapper de `vita-parse-core` para analizar un `.psp2dmp`/`psp2core-*`; ya
  reconoce `advena` como nombre del ejecutable principal en el reporte.
