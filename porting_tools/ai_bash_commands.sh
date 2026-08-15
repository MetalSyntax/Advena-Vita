#!/bin/bash
# ==============================================================================
# Script de inspección y desensamblado (Comandos Bash utilizados por IA / Debug)
# ==============================================================================

set -e

# Configuración de variables con valores por defecto
VITASDK_PATH="${VITASDK:-/Users/metalsyntax/vitasdk}"
export PATH="$VITASDK_PATH/bin:$PATH"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SO_PATH="${1:-$PROJECT_ROOT/lib/libgameDSO.so}"
PATCH_C_PATH="$PROJECT_ROOT/source/patch.c"
DYNLIB_C_PATH="$PROJECT_ROOT/source/dynlib.c"
MAIN_C_PATH="$PROJECT_ROOT/source/main.c"

echo "=============================================================================="
echo "          EJECUTANDO COMANDOS BASH DE ANÁLISIS Y DESENSAMBLADO                "
echo "=============================================================================="
echo " SO Target: $SO_PATH"
echo " VitaSDK:   $VITASDK_PATH"
echo "=============================================================================="

if [ ! -f "$SO_PATH" ]; then
    echo "[-] Error: No se encontró el archivo .so en: $SO_PATH"
    exit 1
fi

disasm_thumb() {
    local start=$1
    local stop=$2
    local label=$3
    echo ""
    echo "[*] Disassembly ($label): $start -> $stop"
    arm-vita-eabi-objdump -d -M force-thumb --start-address="$start" --stop-address="$stop" "$SO_PATH"
}

# NOTA: los offsets de disasm_thumb de este archivo eran los de un binario
# de un port anterior (mismo motor Gamevil Nexus2 -- Zenonia). Aunque
# libgameDSO.so de Advena comparte el mismo motor/namespace JNI
# (Java_com_gamevil_nexus2_Natives_*, ver PORTING_PLAN.md), es un binario
# DISTINTO (otro juego, otro tamaño de assets/código) -- esas direcciones
# NO son válidas acá. Localizar las funciones equivalentes en el
# libgameDSO.so real de Advena antes de reusar disasm_thumb con offsets fijos
# (los comandos de búsqueda por símbolo de abajo sí son directamente
# reusables, porque no dependen de una dirección concreta).

# 1. Búsqueda de símbolos 'Decode' (readelf --dyn-syms) -- punto de partida
#    para encontrar el equivalente de SetImage4GL/DecodePNG en este binario.
echo ""
echo "[*] Búsqueda de símbolos 'Decode' (readelf --dyn-syms):"
arm-vita-eabi-readelf -W --dyn-syms "$SO_PATH" | grep "Decode" || true

# 2. Búsqueda de símbolos libpng (png_error, png_malloc, png_create*, etc.)
echo ""
echo "[*] Símbolos libpng en $SO_PATH:"
arm-vita-eabi-readelf -W --dyn-syms "$SO_PATH" | grep -E "png_(error|malloc|create|read|warning|destroy|set_read_fn)" || true

# Una vez identificados los símbolos/offsets reales de Advena, agregar acá
# las llamadas a disasm_thumb "0x<start>" "0x<stop>" "<label>" equivalentes.

# 7. Verificación de parches e imports en el código fuente del loader
if [ -f "$PATCH_C_PATH" ]; then
    echo ""
    echo "[*] Ocurrencias de 'png_create_struct_2' en patch.c:"
    grep "png_create_struct_2" -B 2 -A 2 "$PATCH_C_PATH" || true
fi

if [ -f "$DYNLIB_C_PATH" ]; then
    echo ""
    echo "[*] Ocurrencias de 'malloc' en dynlib.c:"
    grep -i "malloc" -B 2 -A 2 "$DYNLIB_C_PATH" || true
fi

if [ -f "$MAIN_C_PATH" ]; then
    echo ""
    echo "[*] Configuración de sceLibcHeapSize en main.c:"
    grep "sceLibcHeapSize" "$MAIN_C_PATH" || true
fi

echo ""
echo "[+] Todos los comandos de inspección y desensamblado ejecutados exitosamente."
