#!/bin/bash
set -e

# ==============================================================================
# build.sh — Script de compilación multi-configuración para Advena (PS Vita)
# ==============================================================================
# Uso:
#   ./build.sh [preset_o_tipo] [argumentos_cmake_adicionales...]
#
# Presets disponibles:
#   release           -> Build Release optimizado (-O3, producción)
#   debug             -> Build Debug con logs de soloader (-O2 -g)
#   verbose | debug_verbose -> Build Debug detallado (FalsoJNI ALL + OpenGL + Soloader)
#   relwithdebinfo    -> Build Release optimizado con símbolos de debug (-O3 -g)
#   cg | release_cg   -> Build con Shaders Cg runtime (-DSHADER_FORMAT=CG)
#   glsl_dump         -> Build con volcado activo de shaders GLSL (-DDUMP_COMPILED_SHADERS=ON)
#   minsizerel        -> Build optimizado para tamaño mínimo (-Os)
#
# Ejemplos:
#   ./build.sh release
#   ./build.sh debug
#   ./build.sh verbose
#   ./build.sh cg
#   ./build.sh Debug -DFALSOJNI_DEBUGLEVEL=0
# ==============================================================================

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Uso: ./build.sh [preset] [flags_cmake_extra...]"
    echo ""
    echo "Presets disponibles:"
    echo "  release           Build Release optimizado (-O3, producción)"
    echo "  debug             Build Debug estándar con logs activados (-O2 -g)"
    echo "  verbose           Build Debug exhaustivo (FalsoJNI ALL + OpenGL + Soloader logs)"
    echo "  relwithdebinfo    Build optimizado con símbolos de depuración (-O3 -g)"
    echo "  cg                Build Release usando Shaders CG runtime"
    echo "  glsl_dump         Build Debug con volcado automático de shaders GLSL a disco"
    echo "  minsizerel        Build optimizado para tamaño mínimo (-Os)"
    echo ""
    echo "Cualquier argumento adicional se pasará directamente a CMake."
    exit 0
fi

TARGET_PRESET="${1:-debug}"
shift || true
EXTRA_CMAKE_ARGS=("$@")

CMAKE_BUILD_TYPE="Debug"
CMAKE_CUSTOM_DEFS=()
VARIANT_SUFFIX="debug"

case "$(echo "$TARGET_PRESET" | tr '[:upper:]' '[:lower:]')" in
    release)
        CMAKE_BUILD_TYPE="Release"
        VARIANT_SUFFIX="release"
        ;;
    debug)
        CMAKE_BUILD_TYPE="Debug"
        VARIANT_SUFFIX="debug"
        ;;
    verbose|debug_verbose|debugverbose)
        CMAKE_BUILD_TYPE="Debug"
        CMAKE_CUSTOM_DEFS+=("-DDEBUG_SOLOADER=ON" "-DFALSOJNI_DEBUGLEVEL=ALL" "-DDEBUG_OPENGL=ON")
        VARIANT_SUFFIX="debug_verbose"
        ;;
    relwithdebinfo|relwithdebug|profiling)
        CMAKE_BUILD_TYPE="RelWithDebInfo"
        VARIANT_SUFFIX="relwithdebinfo"
        ;;
    cg|release_cg|cg_shaders)
        CMAKE_BUILD_TYPE="Release"
        CMAKE_CUSTOM_DEFS+=("-DSHADER_FORMAT=CG")
        VARIANT_SUFFIX="cg"
        ;;
    glsl_dump|glsldump|debug_glsl_dump)
        CMAKE_BUILD_TYPE="Debug"
        CMAKE_CUSTOM_DEFS+=("-DSHADER_FORMAT=GLSL" "-DDUMP_COMPILED_SHADERS=ON")
        VARIANT_SUFFIX="glsl_dump"
        ;;
    minsizerel|minsize)
        CMAKE_BUILD_TYPE="MinSizeRel"
        VARIANT_SUFFIX="minsizerel"
        ;;
    *)
        # Si se pasa un tipo CMake directo (ej: Debug, Release, RelWithDebInfo)
        CMAKE_BUILD_TYPE="$TARGET_PRESET"
        VARIANT_SUFFIX="$(echo "$TARGET_PRESET" | tr '[:upper:]' '[:lower:]')"
        ;;
esac

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="/tmp/advena-src"
BUILD_DIR="/tmp/advena-build"
VPK_NAME="advena.vpk"
VARIANT_VPK_NAME="advena_${VARIANT_SUFFIX}.vpk"
VARIANT_ELF_NAME="advena_${VARIANT_SUFFIX}.elf"

echo "================================================================"
echo "  🚀 Script de Build Automático para Advena (PS Vita)"
echo "  Configuración : $CMAKE_BUILD_TYPE"
echo "  Variante      : $VARIANT_SUFFIX ($VARIANT_VPK_NAME)"
if [ ${#CMAKE_CUSTOM_DEFS[@]} -gt 0 ]; then
    echo "  Definiciones  : ${CMAKE_CUSTOM_DEFS[*]}"
fi
if [ ${#EXTRA_CMAKE_ARGS[@]} -gt 0 ]; then
    echo "  Extra CMake   : ${EXTRA_CMAKE_ARGS[*]}"
fi
echo "================================================================"

echo "[1/4] Preparando entorno de compilación..."
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

# Limpieza ligera de cache de CMake para aplicar correctamente cambios de tipo de build
rm -f CMakeCache.txt

cmake "$SRC_DIR" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    "${CMAKE_CUSTOM_DEFS[@]}" \
    "${EXTRA_CMAKE_ARGS[@]}"

make -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

VPK_SOURCE="$BUILD_DIR/$VPK_NAME"
if [ ! -f "$VPK_SOURCE" ]; then
    echo "[-] Error: No se generó el archivo $VPK_SOURCE"
    exit 1
fi

echo "[4/4] Copiando artefactos a $PROJECT_DIR/build/ ..."
mkdir -p "$PROJECT_DIR/build"

# Guardar tanto la versión específica de variante como advena.vpk para compatibilidad
cp "$VPK_SOURCE" "$PROJECT_DIR/build/$VARIANT_VPK_NAME"
cp "$VPK_SOURCE" "$PROJECT_DIR/build/$VPK_NAME"

if [ -f "$BUILD_DIR/eboot.bin" ]; then
    cp "$BUILD_DIR/eboot.bin" "$PROJECT_DIR/build/"
fi

if [ -f "$BUILD_DIR/advena" ]; then
    cp "$BUILD_DIR/advena" "$PROJECT_DIR/build/$VARIANT_ELF_NAME"
    cp "$BUILD_DIR/advena" "$PROJECT_DIR/build/advena.elf"
fi

echo "================================================================"
echo "  ✅ Build exitoso!"
echo "  📦 VPK Variante: $PROJECT_DIR/build/$VARIANT_VPK_NAME"
echo "  📦 VPK Estándar: $PROJECT_DIR/build/$VPK_NAME"
echo "  🔍 ELF Símbolos: $PROJECT_DIR/build/$VARIANT_ELF_NAME"
echo "================================================================"
