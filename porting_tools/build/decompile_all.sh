#!/bin/bash
# ==============================================================================
# decompile_all.sh — Decompilación de APK y .so para Advena (PS Vita)
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

APK_FILE="${PROJECT_ROOT}/Advena-1.0.1.apk"
LIB_DIR="${PROJECT_ROOT}/Advena-1.0.1/lib"
DECOMPILED_DIR="${PROJECT_ROOT}/decompiled"
APK_OUT_DIR="${DECOMPILED_DIR}/apk_jadx"

mkdir -p "$APK_OUT_DIR"

echo "=============================================================================="
echo "  [1/2] Iniciando decompilación con JADX..."
echo "=============================================================================="
if command -v jadx >/dev/null 2>&1; then
  jadx -d "$APK_OUT_DIR" "$APK_FILE" || true
else
  docker run --rm -v "${PROJECT_ROOT}:/app" ubuntu:latest bash -c "
    cd /tmp
    apt-get update && apt-get install -y wget unzip default-jre
    wget -qO- https://github.com/skylot/jadx/releases/download/v1.4.7/jadx-1.4.7.zip > jadx.zip
    unzip -q jadx.zip -d jadx
    ./jadx/bin/jadx -d /app/decompiled/apk_jadx /app/$(basename "$APK_FILE")
  "
fi
echo "[+] JADX Finalizado. Resultados en ${APK_OUT_DIR}"

echo ""
echo "=============================================================================="
echo "  [2/2] Iniciando decompilación de .so con Ghidra (so-decompiler)..."
echo "=============================================================================="
if [ ! -d "$LIB_DIR" ]; then
    echo "[-] Directorio de librerías no encontrado en: $LIB_DIR"
else
    find "$LIB_DIR" -type f -name "*.so" | while read -r so_file; do
        so_name=$(basename "$so_file")
        abi=$(basename "$(dirname "$so_file")")
        so_out="${DECOMPILED_DIR}/${so_name%.so}_${abi}/ghidra"
        mkdir -p "$so_out"

        echo "[*] Decompilando $so_name ($abi) -> $so_out ..."
        docker run --rm --platform linux/amd64 \
          -v "$(dirname "$so_file"):/input" \
          -v "$so_out:/output" \
          devrvk/so-decompiler decompile "/input/$so_name" /output
    done
fi

echo "[+] Decompilación de archivos .so finalizada."
echo "    Resultados en: ${DECOMPILED_DIR}/"
