#!/bin/bash
set -e

# ==============================================================================
# build.sh — Build script para Advena (PS Vita)
# ==============================================================================
# Uso: ./build.sh [Release|Debug]
BUILD_TYPE="${1:-Debug}"

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="/tmp/advena-src"
BUILD_DIR="/tmp/advena-build"
VPK_NAME="advena.vpk"

echo "================================================================"
echo "  🚀 Script de Build Automático para Advena (PS Vita) [$BUILD_TYPE]"
echo "================================================================"

echo "[1/4] Preparando entorno de compilación..."
# Evitamos problemas de rutas con espacios en vita-pack-vpk usando staging en /tmp
mkdir -p "$BUILD_DIR"
mkdir -p "$SRC_DIR"

if [ -z "$VITASDK" ]; then
    if [ -d "/usr/local/vitasdk" ]; then
        export VITASDK="/usr/local/vitasdk"
    elif [ -d "$HOME/vitasdk" ]; then
        export VITASDK="$HOME/vitasdk"
    else
        echo "[-] Error: VITASDK no definida y no se encontró en rutas por defecto."
        exit 1
    fi
fi
export PATH="$VITASDK/bin:$PATH"

echo "[2/4] Sincronizando código fuente a $SRC_DIR..."
rsync -a \
    --delete \
    --exclude '.git' \
    --exclude '.claude' \
    --exclude 'build' \
    --exclude 'decompiled' \
    --exclude 'Advena-1.0.1' \
    --exclude '*.apk' \
    --exclude '*.zip' \
    --exclude '.*' \
    "$PROJECT_DIR/" "$SRC_DIR/"

echo "[3/4] Ejecutando CMake y Make..."
cd "$BUILD_DIR"
cmake "$SRC_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
make -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

VPK_SOURCE="$BUILD_DIR/$VPK_NAME"
if [ ! -f "$VPK_SOURCE" ]; then
    echo "[-] Error: No se generó el archivo $VPK_SOURCE"
    exit 1
fi

echo "[4/4] Copiando artefactos a $PROJECT_DIR/build/ ..."
mkdir -p "$PROJECT_DIR/build"
cp "$VPK_SOURCE" "$PROJECT_DIR/build/"
if [ -f "$BUILD_DIR/eboot.bin" ]; then
    cp "$BUILD_DIR/eboot.bin" "$PROJECT_DIR/build/"
fi
if [ -f "$BUILD_DIR/advena" ]; then
    cp "$BUILD_DIR/advena" "$PROJECT_DIR/build/advena.elf"
fi

echo "================================================================"
echo "  ✅ Build exitoso: $PROJECT_DIR/build/$VPK_NAME"
echo "================================================================"
