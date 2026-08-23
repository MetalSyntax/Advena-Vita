#!/bin/bash
# ==============================================================================
# deploy_and_launch_vita3k.sh — Compila y despliega en Vita3K (macOS)
# ==============================================================================
# Compila con la configuración seleccionada, despliega el eboot.bin + fuente
# directamente en el directorio de la app instalada en Vita3K (sin reinstalar VPK),
# relanza el emulador y activa la ventana.
#
# Uso:
#   ./deploy_and_launch_vita3k.sh [preset: debug|release|verbose|relwithdebinfo|cg]
# ==============================================================================
set -e

# Si se pasa un argumento por línea de comandos, usarlo directamente; si no, preguntar interactivamente
if [ -n "$1" ]; then
    BUILD_PRESET="$1"
else
    echo "================================================================"
    echo "  🎮 Despliegue y Ejecución en Vita3K (macOS)"
    echo "================================================================"
    echo ""
    echo "Selecciona la configuración de compilación:"
    echo "  1) Debug             (Logs de soloader activados, -O2 -g — Recomendado)"
    echo "  2) Release           (Optimizado -O3, producción)"
    echo "  3) Debug Verbose     (FalsoJNI ALL + OpenGL + Soloader logs)"
    echo "  4) RelWithDebInfo    (Release optimizado + símbolos para core dumps)"
    echo "  5) Shaders CG        (Compilación usando motor de shaders CG)"
    echo "  6) Shaders GLSL+Dump (GLSL con volcado de shaders a disco)"
    echo "  7) MinSizeRel        (Optimizado para tamaño mínimo)"
    echo "  q) [ Cancelar ]"
    echo ""
    read -p "Opción [1]: " PRESET_CHOICE
    PRESET_CHOICE="${PRESET_CHOICE:-1}"

    if [ "$PRESET_CHOICE" = "q" ] || [ "$PRESET_CHOICE" = "Q" ]; then
        echo "[*] Operación cancelada."
        exit 0
    fi

    case "$PRESET_CHOICE" in
        1) BUILD_PRESET="debug" ;;
        2) BUILD_PRESET="release" ;;
        3) BUILD_PRESET="verbose" ;;
        4) BUILD_PRESET="relwithdebinfo" ;;
        5) BUILD_PRESET="cg" ;;
        6) BUILD_PRESET="glsl_dump" ;;
        7) BUILD_PRESET="minsizerel" ;;
        *) BUILD_PRESET="debug" ;;
    esac
fi

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_SH="$PROJECT_ROOT/build.sh"
BUILD_DIR="/tmp/advena-build"
TITLE_ID="ADVENA001"
APP_FS="$HOME/Library/Application Support/Vita3K/Vita3K/fs/ux0/app/$TITLE_ID"
FONT_SRC="$PROJECT_ROOT/extras/fonts/DejaVuSans.ttf"
SCRIPT_DIR="$(cd "$(dirname "$0")/../automation" && pwd)"

echo "================================================================"
echo "  🎮 Despliegue y Ejecución en Vita3K (Preset: $BUILD_PRESET)"
echo "================================================================"

echo "[1/4] Compilando con build.sh ($BUILD_PRESET)..."
if [ -x "$BUILD_SH" ]; then
    "$BUILD_SH" "$BUILD_PRESET"
else
    echo "[-] Error: No se encontró $BUILD_SH ejecutable."
    exit 1
fi

echo "[2/4] Desplegando eboot.bin + fuente en $APP_FS..."
mkdir -p "$APP_FS"
if [ -f "$BUILD_DIR/eboot.bin" ]; then
    cp "$BUILD_DIR/eboot.bin" "$APP_FS/eboot.bin"
elif [ -f "$PROJECT_ROOT/build/eboot.bin" ]; then
    cp "$PROJECT_ROOT/build/eboot.bin" "$APP_FS/eboot.bin"
else
    echo "[-] Error: No se encontró eboot.bin para desplegar."
    exit 1
fi

if [ -f "$FONT_SRC" ]; then
    cp "$FONT_SRC" "$APP_FS/DejaVuSans.ttf"
fi
rm -f "$HOME/Library/Application Support/Vita3K/Vita3K/logs/$TITLE_ID - [Advena].log"

echo "[3/4] Relanzando Vita3K limpio..."
pkill -9 -x Vita3K 2>/dev/null || true
sleep 1
open -a Vita3K
sleep 3
osascript -e 'tell application "System Events" to tell process "Vita3K" to set frontmost to true' 2>/dev/null || true
sleep 1

echo "[4/4] Doble clic en el ícono del juego..."
ROW_X=$(osascript -e '
tell application "System Events"
    try
        tell process "Vita3K"
            set p to position of row 1 of table 1 of window 1
            return item 1 of p
        end tell
    end try
end tell' 2>/dev/null || echo "")
ROW_Y=$(osascript -e '
tell application "System Events"
    try
        tell process "Vita3K"
            set p to position of row 1 of table 1 of window 1
            return item 2 of p
        end tell
    end try
end tell' 2>/dev/null || echo "")

if [ -n "$ROW_X" ] && [ -n "$ROW_Y" ]; then
    CLICK_X=$((ROW_X + 60))
    CLICK_Y=$((ROW_Y + 38))
    if [ -f "$SCRIPT_DIR/click_helper.py" ]; then
        python3 "$SCRIPT_DIR/click_helper.py" "$CLICK_X" "$CLICK_Y" 2 2>/dev/null || true
    fi
fi

echo ""
echo "✅ Listo. Log en vivo:"
echo "  tail -f \"$HOME/Library/Application Support/Vita3K/Vita3K/logs/$TITLE_ID - [Advena].log\""
