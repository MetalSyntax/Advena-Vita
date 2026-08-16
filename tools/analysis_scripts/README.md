# Herramientas Python de Análisis y Diagnóstico (Advena PS Vita)

Este directorio contiene las 117 herramientas de introspección, desensamblado, parcheo de opcodes y análisis estático utilizadas para el port de **Advena** a PlayStation Vita.

---

## 📂 Categorías de Herramientas

### 1. Triage y Análisis de Crashes (Ghidra & Desensamblado ARM Thumb-2)
- **`disasm_drawop.py`**: Desensambla la función `DrawOP_ENLARGE_Compress_16_Ex` (offset `0x136050`) para ubicar accesos a memoria y fallos de punteros nulos.
- **`disasm_entry.py`**: Inspecciona el prólogo de `DrawOP_ENLARGE_Compress_16_Ex` (`0x136034`).
- **`disasm_auto.py` / `disasm_fx_body.py` / `disasm_fx_exit.py`**: Desensambla funciones de renderizado de efectos `DrawOP_FX_Compress_16_Ex` (`0x13646c`).
- **`find_drawop.py` / `find_drawop_callers.py` / `print_drawop.py`**: Rastreo y análisis de flujo de llamadas a las funciones de compresión gráfica.
- **`check_native_init.py` / `print_native_init.py` / `print_buffer_size.py`**: Análisis de la secuencia de inicialización `NativeInitWithBufferSize`, `NativeInitDeviceInfo` y `NativeResize`.

### 2. Diagnóstico de Partidas Guardadas Corruptas (CSaveMgr)
- **`find_savemgr.py`**: Lista todos los métodos de la clase `CSaveMgr` en Ghidra.
- **`check_save_sym.py` / `check_menu_sym.py`**: Busca símbolos de guardado en la tabla dinámica del `.so`.
- **`disasm_legal.py`**: Desensambla `CSaveMgr::IsLeaglSaveData(int)` (offset `0xAB424`).
- **`disasm_continue.py`**: Desensambla `CUISubMenuSaveSlot::SelectContinueMenu` (`0x000FF5AD`).
- **`print_save_valid.py`**: Inspecciona la verificación de checksum XOR del save.
- **`find_legal_callers.py` / `print_192355.py`**: Localiza las llamadas a `CreateDontContinuePopup()`.

### 3. Controles, Input y Botones Virtuales (GVUI & Nexus2)
- **`find_battle_keys.py`**: Mapeo completo de teclas procesadas por el motor de combate (`53` = Ataque, `48` = Teamstrike, `-13` = Skill 1, `-3`/`-4` = Saltos).
- **`find_neoui.py` / `find_nexus_files.py`**: Búsqueda y análisis de las clases Java de eventos táctiles y teclado (`NeoUIControllerView`, `NexusGLRenderer`).
- **`find_initpad.py` / `print_init_pad.py` / `print_pad_btns.py`**: Análisis de las posiciones y hotspots táctiles de los 5 botones virtuales en `GVUIPlayerController`.
- **`find_showbtn_all.py` / `print_showbtn.py` / `print_showbtn_body.py`**: Inspección de la máscara de visibilidad de botones en `GVUIPlayerController::ShowBtn`.
- **`find_char_move.py` / `print_char_move.py` / `print_cplayer_move.py` / `print_fellow_move.py`**: Análisis de movimiento de personajes y compañeros.

### 4. Prueba y Ensamblado de Opcodes (Thumb-2 Patching)
- **`test_null_guard.py`**: Ensambla y verifica instrucciones `cmp r0, #0; it eq; bxeq lr;` para inyección de guardas contra punteros nulos.
- **`test_branch.py` / `test_branch2.py` / `test_fx_branch.py`**: Calcula opcodes de salto condicional `beq` a las rutinas de salida limpia de funciones.
- **`test_draw_patch.py`**: Genera y valida parches de memoria para `kuKernelCpuUnrestrictedMemcpy`.
- **`test_opcode.py`**: Verificación de opcodes de salto y retorno directo (`0x47702001` -> `movs r0, #1; bx lr`).

### 5. Extracción e Integración de Recursos
- **`copy_res.py`**: Script de filtrado y copia selectiva de recursos gráficos en inglés (`res/drawable`) hacia `ux0_data/advena/res/`.
- **`find_pad_assets.py` / `inspect_res.py` / `list_res.py`**: Inspección y catálogo de archivos `.pzx` de interfaz (`Keypad.pzx`, `battleui.pzx`).
