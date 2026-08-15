#!/bin/bash
# ==============================================================================
# run_tests.sh — Verificación básica de consistencia y assets para Advena
# ==============================================================================
set -e

TESTS_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$TESTS_DIR/../.." && pwd)"

echo "== [1/2] Verificando binario libgameDSO.so =="
SO_PATH="$PROJECT_DIR/Advena-1.0.1/lib/armeabi/libgameDSO.so"
if [ -f "$SO_PATH" ]; then
    SO_SIZE=$(du -h "$SO_PATH" | cut -f1)
    echo "[+] Encontrado $SO_PATH ($SO_SIZE)"
    
    JNI_COUNT=$(strings "$SO_PATH" | grep -c "Java_com_gamevil_" || true)
    echo "[+] Símbolos JNI Gamevil detectados: $JNI_COUNT"
else
    echo "[-] Advertencia: No se encontró $SO_PATH"
fi

echo "== [2/2] Verificando estructura de assets =="
ASSETS_DIR="$PROJECT_DIR/Advena-1.0.1/assets"
if [ -d "$ASSETS_DIR" ]; then
    FILE_COUNT=$(find "$ASSETS_DIR" -type f | wc -l | tr -d ' ')
    echo "[+] Assets originales encontrados: $FILE_COUNT archivos en $ASSETS_DIR"
else
    echo "[-] Advertencia: No se encontró la carpeta $ASSETS_DIR"
fi

echo "✅ Verificación de host completada"
