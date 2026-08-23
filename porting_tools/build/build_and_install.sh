#!/bin/bash
set -e

# ==============================================================================
# build_and_install.sh — Asistente de compilación y despliegue para Advena (PS Vita)
# ==============================================================================
# Pregunta de forma guiada:
#   [1] Destino: Vita3K (Emulador), PS Vita Física (FTP), o Solo Compilar
#   [2] Tipo de Build: Debug, Release, Verbose, RelWithDebInfo, CG, etc.
#   [3] Método de despliegue según el destino seleccionado
# ==============================================================================

# Colores ANSI
C_RESET="\033[0m"
C_BOLD="\033[1m"
C_CYAN="\033[96m"
C_GREEN="\033[92m"
C_YELLOW="\033[93m"
C_RED="\033[91m"
C_DIM="\033[90m"

PROJECT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_SH="$PROJECT_DIR/build.sh"
MANAGE_VITA_PY="$PROJECT_DIR/porting_tools/manage_vita.py"
TITLE_ID="ADVENA001"

echo -e "${C_CYAN}================================================================${C_RESET}"
echo -e "${C_GREEN}${C_BOLD}  🚀 Asistente de Build y Despliegue — Advena (PS Vita)${C_RESET}"
echo -e "${C_CYAN}================================================================${C_RESET}"

# Si se pasa preset como argumento de CLI
CLI_PRESET="$1"
CLI_TARGET="$2"

# ------------------------------------------------------------------------------
# [PASO 1] Seleccionar Destino (Target)
# ------------------------------------------------------------------------------
if [ -n "$CLI_TARGET" ]; then
    TARGET_CHOICE="$CLI_TARGET"
else
    echo ""
    echo -e "${C_BOLD}[1/3] ¿Cuál es el destino de ejecución?${C_RESET}"
    echo -e "  ${C_GREEN}1)${C_RESET} ${C_BOLD}Vita3K${C_RESET}             (Emulador en macOS — iteración rápida)"
    echo -e "  ${C_GREEN}2)${C_RESET} ${C_BOLD}PS Vita Física${C_RESET}     (Consola real vía conexión FTP)"
    echo -e "  ${C_GREEN}3)${C_RESET} ${C_BOLD}Solo Compilar${C_RESET}      (Generar binarios en /build sin desplegar)"
    echo -e "  ${C_RED}q)${C_RESET} [ Cancelar ]"
    echo ""
    read -p "Destino [1]: " TARGET_CHOICE
    TARGET_CHOICE="${TARGET_CHOICE:-1}"

    if [ "$TARGET_CHOICE" = "q" ] || [ "$TARGET_CHOICE" = "Q" ]; then
        echo -e "${C_YELLOW}[*] Operación cancelada.${C_RESET}"
        exit 0
    fi
fi

case "$TARGET_CHOICE" in
    1|vita3k|Vita3K|emu|emulator)
        TARGET_MODE="vita3k"
        TARGET_LABEL="Vita3K (Emulador)"
        ;;
    2|vita|psvita|PSVita|real|hardware)
        TARGET_MODE="psvita"
        TARGET_LABEL="PS Vita Física"
        ;;
    3|local|build|only|compile)
        TARGET_MODE="local"
        TARGET_LABEL="Solo Compilar"
        ;;
    *)
        echo -e "${C_YELLOW}[!] Destino no reconocido, usando 'Vita3K'.${C_RESET}"
        TARGET_MODE="vita3k"
        TARGET_LABEL="Vita3K (Emulador)"
        ;;
esac

# ------------------------------------------------------------------------------
# [PASO 2] Seleccionar Tipo de Build (Preset)
# ------------------------------------------------------------------------------
if [ -n "$CLI_PRESET" ]; then
    BUILD_CHOICE="$CLI_PRESET"
    EXTRA_FLAGS=("${@:3}")
else
    echo ""
    echo -e "${C_BOLD}[2/3] Selecciona la configuración de compilación (Build Type):${C_RESET}"
    echo -e "  ${C_GREEN}1)${C_RESET} ${C_BOLD}Debug${C_RESET}             (Logs soloader activos, -O2 -g — ${C_YELLOW}Recomendado desarrollo${C_RESET})"
    echo -e "  ${C_GREEN}2)${C_RESET} ${C_BOLD}Release${C_RESET}           (Optimizado -O3, recomendado para jugar/producción)"
    echo -e "  ${C_GREEN}3)${C_RESET} ${C_BOLD}Debug Verbose${C_RESET}     (FalsoJNI ALL + OpenGL + Soloader logs)"
    echo -e "  ${C_GREEN}4)${C_RESET} ${C_BOLD}RelWithDebInfo${C_RESET}    (Release optimizado -O3 + símbolos -g para core dumps)"
    echo -e "  ${C_GREEN}5)${C_RESET} ${C_BOLD}Shaders CG${C_RESET}        (Compilación usando motor de shaders CG runtime)"
    echo -e "  ${C_GREEN}6)${C_RESET} ${C_BOLD}Shaders GLSL+Dump${C_RESET} (GLSL con volcado automático de shaders a disco)"
    echo -e "  ${C_GREEN}7)${C_RESET} ${C_BOLD}MinSizeRel${C_RESET}        (Optimizado para tamaño mínimo de binario -Os)"
    echo -e "  ${C_GREEN}8)${C_RESET} ${C_BOLD}Personalizado${C_RESET}     (Configuración manual avanzada)"
    echo -e "  ${C_RED}q)${C_RESET} [ Cancelar ]"
    echo ""
    read -p "Opción [1]: " BUILD_CHOICE
    BUILD_CHOICE="${BUILD_CHOICE:-1}"

    if [ "$BUILD_CHOICE" = "q" ] || [ "$BUILD_CHOICE" = "Q" ]; then
        echo -e "${C_YELLOW}[*] Operación cancelada.${C_RESET}"
        exit 0
    fi
fi

EXTRA_FLAGS=()

case "$BUILD_CHOICE" in
    1|debug|Debug)
        BUILD_PRESET="debug"
        ;;
    2|release|Release)
        BUILD_PRESET="release"
        ;;
    3|verbose|debug_verbose|Verbose)
        BUILD_PRESET="verbose"
        ;;
    4|relwithdebinfo|RelWithDebInfo)
        BUILD_PRESET="relwithdebinfo"
        ;;
    5|cg|CG|shaders_cg)
        BUILD_PRESET="cg"
        ;;
    6|glsl_dump|GLSL_DUMP|glsldump)
        BUILD_PRESET="glsl_dump"
        ;;
    7|minsizerel|MinSizeRel)
        BUILD_PRESET="minsizerel"
        ;;
    8|custom|Custom|personalizado)
        echo ""
        echo -e "${C_CYAN}--- Configuración Personalizada ---${C_RESET}"
        read -p "Tipo de Build CMake [Debug/Release/RelWithDebInfo/MinSizeRel] (Default: Debug): " CUSTOM_TYPE
        CUSTOM_TYPE="${CUSTOM_TYPE:-Debug}"
        
        read -p "Formato de shaders [GLSL/CG/GXP] (Default: GLSL): " CUSTOM_SHADERS
        CUSTOM_SHADERS="${CUSTOM_SHADERS:-GLSL}"
        
        read -p "Nivel de log FalsoJNI [DEFAULT/ALL/INFO/WARN/ERROR/NO] (Default: DEFAULT): " CUSTOM_FALSOJNI
        CUSTOM_FALSOJNI="${CUSTOM_FALSOJNI:-DEFAULT}"
        
        read -p "Activar logs de Soloader? [s/N]: " CUSTOM_SOLOADER
        if [[ "$CUSTOM_SOLOADER" =~ ^[sS]$ ]]; then
            EXTRA_FLAGS+=("-DDEBUG_SOLOADER=ON")
        fi
        
        read -p "Activar logs de OpenGL? [s/N]: " CUSTOM_OPENGL
        if [[ "$CUSTOM_OPENGL" =~ ^[sS]$ ]]; then
            EXTRA_FLAGS+=("-DDEBUG_OPENGL=ON")
        fi
        
        read -p "Volcar shaders compilados a disco? [s/N]: " CUSTOM_DUMP
        if [[ "$CUSTOM_DUMP" =~ ^[sS]$ ]]; then
            EXTRA_FLAGS+=("-DDUMP_COMPILED_SHADERS=ON")
        fi

        EXTRA_FLAGS+=("-DSHADER_FORMAT=$CUSTOM_SHADERS")
        if [ "$CUSTOM_FALSOJNI" != "DEFAULT" ]; then
            EXTRA_FLAGS+=("-DFALSOJNI_DEBUGLEVEL=$CUSTOM_FALSOJNI")
        fi

        BUILD_PRESET="$CUSTOM_TYPE"
        ;;
    *)
        BUILD_PRESET="$BUILD_CHOICE"
        ;;
esac

# ------------------------------------------------------------------------------
# [PASO 3] Compilación
# ------------------------------------------------------------------------------
echo ""
echo -e "${C_CYAN}================================================================${C_RESET}"
echo -e "  🔨 ${C_BOLD}Compilando Advena${C_RESET}"
echo -e "  Destino : ${C_GREEN}$TARGET_LABEL${C_RESET}"
echo -e "  Preset  : ${C_YELLOW}$BUILD_PRESET${C_RESET}"
echo -e "${C_CYAN}================================================================${C_RESET}"

if [ -x "$BUILD_SH" ]; then
    "$BUILD_SH" "$BUILD_PRESET" "${EXTRA_FLAGS[@]}"
else
    echo -e "${C_RED}[-] Error: No se encontró $BUILD_SH ejecutable.${C_RESET}"
    exit 1
fi

VPK_PATH="$PROJECT_DIR/build/advena.vpk"
EBOOT_PATH="$PROJECT_DIR/build/eboot.bin"

if [ ! -f "$VPK_PATH" ]; then
    echo -e "${C_RED}[-] Error: No se generó $VPK_PATH${C_RESET}"
    exit 1
fi

# ------------------------------------------------------------------------------
# [PASO 4] Despliegue según el Destino Elegido
# ------------------------------------------------------------------------------
echo ""
echo -e "${C_CYAN}================================================================${C_RESET}"
echo -e "  📦 ${C_BOLD}[3/3] Despliegue para $TARGET_LABEL${C_RESET}"
echo -e "${C_CYAN}================================================================${C_RESET}"

if [ "$TARGET_MODE" = "vita3k" ]; then
    echo ""
    echo -e "¿Cómo deseas desplegar en Vita3K?"
    echo -e "  ${C_GREEN}1)${C_RESET} ${C_BOLD}Hot-swap eboot.bin + fuente y relanzar Vita3K${C_RESET} (${C_YELLOW}Rápido / Recomendado${C_RESET})"
    echo -e "  ${C_GREEN}2)${C_RESET} ${C_BOLD}Instalar VPK completo en Vita3K y lanzar${C_RESET}"
    echo -e "  ${C_GREEN}3)${C_RESET} ${C_BOLD}Solo copiar eboot.bin + fuente a Vita3K${C_RESET} (sin abrir el emulador)"
    echo -e "  ${C_GREEN}4)${C_RESET} ${C_BOLD}Omitir despliegue${C_RESET} (solo compilar)"
    echo ""
    read -p "Opción [1]: " VITA3K_ACTION
    VITA3K_ACTION="${VITA3K_ACTION:-1}"

    case "$VITA3K_ACTION" in
        1)
            echo -e "${C_CYAN}[*] Desplegando eboot.bin + fuente y relanzando Vita3K...${C_RESET}"
            DEPLOY_SCRIPT="$PROJECT_DIR/porting_tools/build/deploy_and_launch_vita3k.sh"
            if [ -f "$DEPLOY_SCRIPT" ]; then
                # Pasamos el preset para que no vuelva a preguntar
                bash "$DEPLOY_SCRIPT" "$BUILD_PRESET"
            else
                APP_FS="$HOME/Library/Application Support/Vita3K/Vita3K/fs/ux0/app/$TITLE_ID"
                mkdir -p "$APP_FS"
                cp "$EBOOT_PATH" "$APP_FS/eboot.bin"
                if [ -f "$PROJECT_DIR/extras/fonts/DejaVuSans.ttf" ]; then
                    cp "$PROJECT_DIR/extras/fonts/DejaVuSans.ttf" "$APP_FS/DejaVuSans.ttf"
                fi
                pkill -9 -x Vita3K 2>/dev/null || true
                sleep 1
                open -a Vita3K 2>/dev/null || true
                echo -e "${C_GREEN}[+] Vita3K lanzado.${C_RESET}"
            fi
            ;;
        2)
            VITA3K_APP="/Applications/Vita3K.app/Contents/MacOS/Vita3K"
            if [ -x "$VITA3K_APP" ]; then
                echo -e "${C_CYAN}[*] Instalando VPK y lanzando Vita3K...${C_RESET}"
                "$VITA3K_APP" -B OpenGL "$VPK_PATH" > /dev/null 2>&1 &
                echo -e "${C_GREEN}[+] Vita3K instalado y lanzado.${C_RESET}"
            else
                echo -e "${C_YELLOW}[!] Vita3K no encontrado en /Applications/Vita3K.app. Abriendo con 'open -a Vita3K'...${C_RESET}"
                open -a Vita3K "$VPK_PATH" 2>/dev/null || true
            fi
            ;;
        3)
            APP_FS="$HOME/Library/Application Support/Vita3K/Vita3K/fs/ux0/app/$TITLE_ID"
            mkdir -p "$APP_FS"
            cp "$EBOOT_PATH" "$APP_FS/eboot.bin"
            if [ -f "$PROJECT_DIR/extras/fonts/DejaVuSans.ttf" ]; then
                cp "$PROJECT_DIR/extras/fonts/DejaVuSans.ttf" "$APP_FS/DejaVuSans.ttf"
            fi
            echo -e "${C_GREEN}[+] eboot.bin y fuentes copiados a $APP_FS${C_RESET}"
            ;;
        4)
            echo -e "${C_YELLOW}[*] Despliegue en Vita3K omitido.${C_RESET}"
            ;;
    esac

elif [ "$TARGET_MODE" = "psvita" ]; then
    echo ""
    echo -e "¿Cómo deseas desplegar a tu PS Vita física (FTP)?"
    echo -e "  ${C_GREEN}1)${C_RESET} ${C_BOLD}Subir SOLO eboot.bin a ux0:app/$TITLE_ID/${C_RESET} (${C_YELLOW}Rápido - no requiere reinstalar VPK${C_RESET})"
    echo -e "  ${C_GREEN}2)${C_RESET} ${C_BOLD}Subir VPK completo a ux0:downloads/${C_RESET} (Para instalar desde VitaShell)"
    echo -e "  ${C_GREEN}3)${C_RESET} ${C_BOLD}Abrir panel de control interactivo${C_RESET} (manage_vita.py)"
    echo -e "  ${C_GREEN}4)${C_RESET} ${C_BOLD}Omitir subida por FTP${C_RESET}"
    echo ""
    read -p "Opción [1]: " PSVITA_ACTION
    PSVITA_ACTION="${PSVITA_ACTION:-1}"

    case "$PSVITA_ACTION" in
        1)
            echo -e "${C_CYAN}[*] Subiendo eboot.bin por FTP a la consola...${C_RESET}"
            if [ -f "$MANAGE_VITA_PY" ]; then
                python3 "$MANAGE_VITA_PY" --upload-eboot
            else
                echo -e "${C_RED}[-] No se encontró manage_vita.py${C_RESET}"
            fi
            ;;
        2)
            echo -e "${C_CYAN}[*] Subiendo VPK por FTP a la consola...${C_RESET}"
            if [ -f "$MANAGE_VITA_PY" ]; then
                python3 "$MANAGE_VITA_PY" --upload-vpk
            else
                echo -e "${C_RED}[-] No se encontró manage_vita.py${C_RESET}"
            fi
            ;;
        3)
            if [ -f "$MANAGE_VITA_PY" ]; then
                python3 "$MANAGE_VITA_PY"
            fi
            ;;
        4)
            echo -e "${C_YELLOW}[*] Subida a PS Vita omitida.${C_RESET}"
            ;;
    esac

else
    echo -e "${C_GREEN}✅ Binarios generados exitosamente en build/:${C_RESET}"
    ls -lh "$PROJECT_DIR/build/" | grep -E '\.(vpk|elf|bin)$' | awk '{printf "  📦 %-24s (%s)\n", $9, $5}'
    echo ""
    echo -e "${C_DIM}Tip: Para desplegar en cualquier momento usa:${C_RESET}"
    echo -e "  • Vita3K : ${C_CYAN}porting_tools/build/deploy_and_launch_vita3k.sh${C_RESET}"
    echo -e "  • PS Vita: ${C_CYAN}python3 porting_tools/manage_vita.py${C_RESET}"
fi

echo ""
echo -e "${C_GREEN}${C_BOLD}✨ Proceso finalizado.${C_RESET}"

