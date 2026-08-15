#!/bin/bash
set -e

# ==============================================================================
# build_and_install.sh — Build y despliegue interactivo para Advena (PS Vita)
# ==============================================================================

PROJECT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_SH="$PROJECT_DIR/build.sh"
VPK_NAME="advena.vpk"
VPK_PATH="$PROJECT_DIR/build/$VPK_NAME"

echo "================================================================"
echo "  Script de Build Automático para Advena (PS Vita)"
echo "================================================================"

echo ""
echo "[1/3] Selecciona el tipo de build:"
echo "  1) Debug (con logs de depuración)"
echo "  2) Release (optimizado)"
echo "  q) [ Cancelar ]"
echo ""
read -p "Opción [1]: " BUILD_CHOICE
BUILD_CHOICE="${BUILD_CHOICE:-1}"

if [ "$BUILD_CHOICE" = "q" ] || [ "$BUILD_CHOICE" = "Q" ]; then
    echo "[*] Operación cancelada."
    exit 0
fi

if [ "$BUILD_CHOICE" = "2" ]; then
    BUILD_TYPE="Release"
else
    BUILD_TYPE="Debug"
fi

echo ""
echo "[2/3] Compilando Advena ($BUILD_TYPE)..."
if [ -x "$BUILD_SH" ]; then
    "$BUILD_SH" "$BUILD_TYPE"
else
    echo "[-] Error: No se encontró $BUILD_SH ejecutable."
    exit 1
fi

if [ ! -f "$VPK_PATH" ]; then
    echo "[-] Error: No se generó $VPK_PATH"
    exit 1
fi

echo ""
echo "[3/3] Instalación (opcional)"
VITA3K_APP="/Applications/Vita3K.app/Contents/MacOS/Vita3K"
if [ -x "$VITA3K_APP" ]; then
    read -p "¿Deseas instalar y ejecutar $VPK_NAME en Vita3K ahora? [s/N] " INSTALL_VITA3K
    if [[ "$INSTALL_VITA3K" =~ ^[sS]$ ]]; then
        echo "Instalando VPK y lanzando Vita3K..."
        "$VITA3K_APP" -B OpenGL "$VPK_PATH" > /dev/null 2>&1 &
        echo "[+] Listo."
    else
        echo "[*] Omitiendo instalación automática en Vita3K."
    fi
else
    echo "[*] Vita3K no encontrado en /Applications/Vita3K.app."
fi
echo "Para instalar en una consola PS Vita real, usa manage_vita.py o 'make send'."
