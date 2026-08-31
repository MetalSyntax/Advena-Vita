/**
 * @brief java.c — FalsoJNI bindings and callbacks for Advena (PS Vita).
 * @note See `docs/source/java.md:1` for detailed design rationale.
 */

#include <falso_jni/FalsoJNI_Impl.h>
#include <falso_jni/FalsoJNI.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "java.h"
#include "audio.h"
#include "font.h"
#include "ksc5601_table.h"
#include "utils/logger.h"

volatile int g_ui_status = -1;

static int advena_resolve_asset_path(const char *name, char *out, size_t out_size) {
    if (!name || !out) return 0;

    snprintf(out, out_size, "ux0:data/advena/assets/%s", name);
    if (access(out, F_OK) == 0) return 1;

    snprintf(out, out_size, "ux0:data/advena/%s", name);
    if (access(out, F_OK) == 0) return 1;

    snprintf(out, out_size, "ux0:data/advena/res/drawable/%s", name);
    if (access(out, F_OK) == 0) return 1;

    snprintf(out, out_size, "ux0:data/advena/res/raw/%s", name);
    if (access(out, F_OK) == 0) return 1;

    snprintf(out, out_size, "ux0:data/advena/res/%s", name);
    if (access(out, F_OK) == 0) return 1;

    return 0;
}

#define ADVENA_DALVIK_REGISTRY_MAX 2048
static void *g_dalvik_registry[ADVENA_DALVIK_REGISTRY_MAX];
static int g_dalvik_registry_count = 0;

static void advena_register_dalvik_array(void *ptr) {
    if (g_dalvik_registry_count < ADVENA_DALVIK_REGISTRY_MAX) {
        g_dalvik_registry[g_dalvik_registry_count++] = ptr;
    }
}

static int advena_is_dalvik_array(void *ptr) {
    for (int i = 0; i < g_dalvik_registry_count; i++) {
        if (g_dalvik_registry[i] == ptr) return 1;
    }
    return 0;
}

static jsize (*advena_orig_GetArrayLength)(JNIEnv *, jarray) = NULL;
static jbyte *(*advena_orig_GetByteArrayElements)(JNIEnv *, jbyteArray, jboolean *) = NULL;
static void (*advena_orig_GetByteArrayRegion)(JNIEnv *, jbyteArray, jsize, jsize, jbyte *) = NULL;
static void (*advena_orig_ReleaseByteArrayElements)(JNIEnv *, jbyteArray, jbyte *, jint) = NULL;

static jsize Advena_GetArrayLength_wrapper(JNIEnv *env, jarray array) {
    if (advena_is_dalvik_array(array)) {
        return (jsize) *(uint32_t *)((char *) array + 8);
    }
    return advena_orig_GetArrayLength(env, array);
}

static jbyte *Advena_GetByteArrayElements_wrapper(JNIEnv *env, jbyteArray array, jboolean *isCopy) {
    if (advena_is_dalvik_array(array)) {
        if (isCopy) *isCopy = JNI_FALSE;
        return (jbyte *)((char *) array + 16);
    }
    return advena_orig_GetByteArrayElements(env, array, isCopy);
}

static void Advena_GetByteArrayRegion_wrapper(JNIEnv *env, jbyteArray array, jsize start, jsize len, jbyte *buf) {
    if (advena_is_dalvik_array(array)) {
        memcpy(buf, (char *) array + 16 + start, len);
        return;
    }
    advena_orig_GetByteArrayRegion(env, array, start, len, buf);
}

static void Advena_ReleaseByteArrayElements_wrapper(JNIEnv *env, jbyteArray array, jbyte *elems, jint mode) {
    if (advena_is_dalvik_array(array)) {
        return;
    }
    advena_orig_ReleaseByteArrayElements(env, array, elems, mode);
}

void advena_install_array_hooks(void) {
    struct JNINativeInterface *funcs = (struct JNINativeInterface *)(uintptr_t) jni;
    advena_orig_GetArrayLength = funcs->GetArrayLength;
    advena_orig_GetByteArrayElements = funcs->GetByteArrayElements;
    advena_orig_GetByteArrayRegion = funcs->GetByteArrayRegion;
    advena_orig_ReleaseByteArrayElements = funcs->ReleaseByteArrayElements;
    funcs->GetArrayLength = Advena_GetArrayLength_wrapper;
    funcs->GetByteArrayElements = Advena_GetByteArrayElements_wrapper;
    funcs->GetByteArrayRegion = Advena_GetByteArrayRegion_wrapper;
    funcs->ReleaseByteArrayElements = Advena_ReleaseByteArrayElements_wrapper;
}

static jobject advena_new_byte_array_str(const char *s) {
    int len = (int) strlen(s);
    JavaDynArray *jda = jda_alloc(len, FIELD_TYPE_BYTE);
    if (!jda) return NULL;
    memcpy(jda->array, s, len);
    return (jobject) jda;
}

/**< @brief JNI Handlers. */

jobject Advena_readAssets(jmethodID id, va_list args) {
    jstring filename = va_arg(args, jstring);
    const char *name = (const char *) filename;
    if (!name) return NULL;

    char path[256];
    if (!advena_resolve_asset_path(name, path, sizeof(path))) {
        l_debug("[Java] readAssets not found: %s", name);
        return NULL;
    }

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    struct stat st;
    long size = -1;
    if (fstat(fileno(f), &st) == 0) {
        size = st.st_size;
    }

    if (size < 0 || size > 64 * 1024 * 1024) {
        fclose(f);
        return NULL;
    }

    void *array_obj = malloc(16 + size);
    if (!array_obj) {
        fclose(f);
        return NULL;
    }

    memset(array_obj, 0, 16);
    *(uint32_t *)((char *)array_obj + 8) = (uint32_t)size;

    fread((char *) array_obj + 16, 1, size, f);
    fclose(f);

    advena_register_dalvik_array(array_obj);
    return array_obj;
}

jint Advena_isAssetExist(jmethodID id, va_list args) {
    jstring filename = va_arg(args, jstring);
    const char *name = (const char *) filename;
    if (!name) return 0;

    char path[256];
    if (advena_resolve_asset_path(name, path, sizeof(path))) {
        struct stat st;
        if (stat(path, &st) == 0 && !S_ISDIR(st.st_mode)) {
            return (jint)st.st_size;
        }
    }
    return 0;
}

jobject Advena_loadFileFromStorage(jmethodID id, va_list args) {
    return Advena_readAssets(id, args);
}

jint Advena_getGLOptionLinear(jmethodID id, va_list args) {
    return 1;
}

void Advena_SetSpeed(jmethodID id, va_list args) {
}

jobject Advena_getPhoneModel(jmethodID id, va_list args) {
    return advena_new_byte_array_str("PSVita");
}

jobject Advena_getAbsolueFilePath(jmethodID id, va_list args) {
    /**
     * @brief On real Android this mirrors Context.
     * @note See `docs/source/java.md:192` for detailed design rationale.
     */
    mkdir("ux0:data/advena/saves", 0777);
    return (jobject) "ux0:data/advena/saves";
}

jobject Advena_getPhoneNumber(jmethodID id, va_list args) {
    return advena_new_byte_array_str("01012345678");
}

jobject Advena_getSimSerialNumber(jmethodID id, va_list args) {
    return advena_new_byte_array_str("8982000012345678901");
}

jobject Advena_getMacAddress(jmethodID id, va_list args) {
    return advena_new_byte_array_str("00:11:22:33:44:55");
}

jobject Advena_getDeviceID(jmethodID id, va_list args) {
    return advena_new_byte_array_str("ADVENA_VITA_001");
}

jobject Advena_getAndroidID(jmethodID id, va_list args) {
    return advena_new_byte_array_str("1234567890abcdef");
}

jobject Advena_getCarrierName(jmethodID id, va_list args) {
    return advena_new_byte_array_str("Gamevil");
}

jobject Advena_getDeviceType(jmethodID id, va_list args) {
    return advena_new_byte_array_str("AD Default");
}

jint Advena_getLocaleID(jmethodID id, va_list args) {
    return 2; // English default (1 = Korean)
}

void Advena_getLanguage(jmethodID id, va_list args) {
    int lang = va_arg(args, int);
    l_debug("[Java] getLanguage: %d", lang);
}

jlong Advena_getTotalMemory(jmethodID id, va_list args) {
    return 64 * 1024 * 1024;
}

jlong Advena_getFreeMemory(jmethodID id, va_list args) {
    return 32 * 1024 * 1024;
}

jlong Advena_getMaxMemory(jmethodID id, va_list args) {
    return 64 * 1024 * 1024;
}

jlong Advena_getUsedMemory(jmethodID id, va_list args) {
    return 32 * 1024 * 1024;
}

jint Advena_isNetAvailable(jmethodID id, va_list args) {
    return 1;
}

jint Advena_netConnect(jmethodID id, va_list args) {
    return 1;
}

void Advena_OnUIStatusChange(jmethodID id, va_list args) {
    int status = va_arg(args, int);
    g_ui_status = status;
    l_debug("[Java] OnUIStatusChange: %d", status);
}

void Advena_OnSoundPlay(jmethodID id, va_list args) {
    int snd_id = va_arg(args, int);
    int vol = va_arg(args, int);
    int is_loop = va_arg(args, int);

    /**
     * @brief Sound IDs 16-21 are always one-shot SFX on Android: AdvenaUIControllerView.OnSoundPlay
     * routes them straight to NexusSound.playSFXSound(), which plays through a SoundPool with a
     * hardcoded loop=0 and ignores the isLoop argument entirely (decompiled/apk_jadx/sources/com/
     * gamevil/advena/ui/AdvenaUIControllerView.java:485-491). The engine sometimes calls
     * OnSoundPlay() with is_loop=1 for these IDs anyway; honoring it here made them loop forever
     * instead of playing once.
     */
    if (snd_id >= 16 && snd_id <= 21) {
        is_loop = 0;
    }

    audio_play(snd_id, vol, is_loop);
}

void Advena_OnStopSound(jmethodID id, va_list args) {
    audio_stop_all();
}

void Advena_OnVibrate(jmethodID id, va_list args) {
}

void Advena_OnEvent(jmethodID id, va_list args) {
    int ev = va_arg(args, int);
    l_debug("[Java] OnEvent: %d", ev);
}

void Advena_OnAsyncTimerSet(jmethodID id, va_list args) {
}

void Advena_VoidNoop(jmethodID id, va_list args) {
}

jint Advena_IntZero(jmethodID id, va_list args) {
    return 0;
}

/**
 * @brief Save file handlers (ux0:data/advena/saves/<name>).
 * @note See `docs/source/java.md:304` for detailed design rationale.
 */

static void resolve_save_path(const char *name, char *out, size_t out_size) {
    snprintf(out, out_size, "ux0:data/advena/saves/%s", name);
}

jint Advena_isFileExist(jmethodID id, va_list args) {
    jstring name_str = va_arg(args, jstring);
    const char *name = (const char *) name_str;
    if (!name) return 0;

    char path[256];
    resolve_save_path(name, path, sizeof(path));

    struct stat st;
    if (stat(path, &st) == 0) {
        return (jint)st.st_size;
    }
    return 0;
}

jint Advena_saveFile(jmethodID id, va_list args) {
    jstring name_str = va_arg(args, jstring);
    jbyteArray data_arr = va_arg(args, jbyteArray);
    const char *name = (const char *) name_str;
    if (!name || !data_arr) return 0;

    mkdir("ux0:data/advena/saves", 0777);

    char path[256];
    resolve_save_path(name, path, sizeof(path));

    jsize len = jni->GetArrayLength(&jni, data_arr);
    jbyte *elems = jni->GetByteArrayElements(&jni, data_arr, NULL);
    if (!elems) return 0;

    FILE *f = fopen(path, "wb");
    if (!f) {
        jni->ReleaseByteArrayElements(&jni, data_arr, elems, JNI_ABORT);
        return 0;
    }

    fwrite(elems, 1, len, f);
    fclose(f);

    jni->ReleaseByteArrayElements(&jni, data_arr, elems, JNI_ABORT);
    l_info("[Java] Saved %s (%d bytes)", path, (int)len);
    return len;
}

jobject Advena_loadFile(jmethodID id, va_list args) {
    jstring name_str = va_arg(args, jstring);
    const char *name = (const char *) name_str;
    if (!name) return NULL;

    char path[256];
    resolve_save_path(name, path, sizeof(path));

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    struct stat st;
    if (fstat(fileno(f), &st) != 0 || st.st_size <= 0) {
        fclose(f);
        return NULL;
    }

    JavaDynArray *jda = jda_alloc((int)st.st_size, FIELD_TYPE_BYTE);
    if (!jda) {
        fclose(f);
        return NULL;
    }

    fread(jda->array, 1, st.st_size, f);
    fclose(f);

    l_info("[Java] Loaded %s (%ld bytes)", path, (long)st.st_size);
    return (jobject) jda;
}

jint Advena_deleteFile(jmethodID id, va_list args) {
    jstring name_str = va_arg(args, jstring);
    const char *name = (const char *) name_str;
    if (!name) return 0;

    char path[256];
    resolve_save_path(name, path, sizeof(path));
    unlink(path);
    return 1;
}

/**
 * @brief Font GFA Handlers.
 * @note See `docs/source/java.md:397` for detailed design rationale.
 */

#define GFA_MAX_FONTS 5
#define GFA_MAX_STR 1024
static int g_gfa_initialized = 0;
static float g_gfa_text_size = 12.0f;
static int g_gfa_color = 0xff000000;
static int g_gfa_cur_font = -1;
static int g_gfa_font_used[GFA_MAX_FONTS] = {0};
static int g_gfa_width = 0, g_gfa_height = 0, g_gfa_bpp = 32;
static int g_gfa_string_len = 0;
static uint32_t g_gfa_str[GFA_MAX_STR];
static int g_gfa_str_n = 0;
static JavaDynArray *g_gfa_pixels32_jda = NULL;
static JavaDynArray *g_gfa_pixels16_jda = NULL;

static int gfa_decode_utf8(const char *s, uint32_t *out, int max) {
    int n = 0;
    const unsigned char *p = (const unsigned char *) s;
    while (*p && n < max) {
        uint32_t cp;
        if (*p < 0x80) { cp = *p++; }
        else if ((*p & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
            cp = ((*p & 0x1F) << 6) | (p[1] & 0x3F); p += 2;
        } else if ((*p & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
            cp = ((*p & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3;
        } else if ((*p & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
            cp = ((*p & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F); p += 4;
        } else { cp = '?'; p++; }
        out[n++] = cp;
    }
    return n;
}

jboolean Advena_GFA_IsInitialized(jmethodID id, va_list args) {
    return g_gfa_initialized ? JNI_TRUE : JNI_FALSE;
}

jboolean Advena_GFA_Init(jmethodID id, va_list args) {
    int w = va_arg(args, int);
    int h = va_arg(args, int);
    int bpp = va_arg(args, int);
    int colorkey = va_arg(args, int);
    int anti_alias = va_arg(args, int);
    int locale = va_arg(args, int);

    g_gfa_width = w;
    g_gfa_height = h;
    g_gfa_bpp = bpp;
    g_gfa_initialized = 1;

    gfa_font_init("app0:font.ttf");

    int count = w * h;
    if (bpp == 16) {
        g_gfa_pixels16_jda = jda_alloc(count, FIELD_TYPE_SHORT);
    } else {
        g_gfa_pixels32_jda = jda_alloc(count, FIELD_TYPE_INT);
    }
    return JNI_TRUE;
}

void Advena_GFA_SetTextSize(jmethodID id, va_list args) {
    g_gfa_text_size = (float) va_arg(args, double);
}

jint Advena_GFA_CreateFont(jmethodID id, va_list args) {
    for (int i = 0; i < GFA_MAX_FONTS; i++) {
        if (!g_gfa_font_used[i]) {
            g_gfa_font_used[i] = 1;
            g_gfa_cur_font = i;
            return i;
        }
    }
    return 0;
}

void Advena_GFA_SetColor(jmethodID id, va_list args) {
    g_gfa_color = va_arg(args, int);
}

jint Advena_GFA_SetFont(jmethodID id, va_list args) {
    int f = va_arg(args, int);
    g_gfa_cur_font = f;
    return f;
}

jint Advena_GFA_CharWidth(jmethodID id, va_list args) {
    int ch = va_arg(args, int);
    return (jint) gfa_font_advance(g_gfa_text_size, (uint32_t) ch);
}

jint Advena_GFA_CharHeight(jmethodID id, va_list args) {
    return (jint) (gfa_font_ascent(g_gfa_text_size) + gfa_font_descent(g_gfa_text_size));
}

jint Advena_GFA_GetAscent(jmethodID id, va_list args) {
    return (jint) -gfa_font_ascent(g_gfa_text_size);
}

jint Advena_GFA_GetDescent(jmethodID id, va_list args) {
    return (jint) gfa_font_descent(g_gfa_text_size);
}

jint Advena_GFA_GetCurrentFont(jmethodID id, va_list args) {
    return g_gfa_cur_font;
}

jint Advena_GFA_GetColor(jmethodID id, va_list args) {
    return g_gfa_color;
}

jint Advena_GFA_GetStringLength(jmethodID id, va_list args) {
    return g_gfa_string_len;
}

void Advena_GFA_SetString(jmethodID id, va_list args) {
    jstring s = va_arg(args, jstring);
    const char *str = (const char *) s;
    if (!str) {
        g_gfa_str_n = 0;
        g_gfa_string_len = 0;
        return;
    }
    g_gfa_str_n = gfa_decode_utf8(str, g_gfa_str, GFA_MAX_STR);
    g_gfa_string_len = (int) strlen(str);
}

jobject Advena_GFA_DrawFont(jmethodID id, va_list args) {
    int ch = va_arg(args, int);
    float pen_x = (float) va_arg(args, double);
    float pen_y = (float) va_arg(args, double);

    if (g_gfa_pixels32_jda) {
        uint32_t cp = (uint32_t) ch;
        uint32_t *buf = (uint32_t *) g_gfa_pixels32_jda->array;
        memset(buf, 0, g_gfa_width * g_gfa_height * 4);
        float baseline = pen_y + gfa_font_ascent(g_gfa_text_size);
        gfa_font_draw_line(g_gfa_text_size, &cp, 1, buf, g_gfa_width, g_gfa_height, pen_x, baseline, (uint32_t) g_gfa_color);
    }
    return (jobject) g_gfa_pixels32_jda;
}

jobject Advena_GFA_GetPixels32(jmethodID id, va_list args) {
    return (jobject) g_gfa_pixels32_jda;
}

jobject Advena_GFA_GetPixels16(jmethodID id, va_list args) {
    return (jobject) g_gfa_pixels16_jda;
}

/**
 * @brief Method ID Table.
 * @note See `docs/source/java.md:550` for detailed design rationale.
 */

NameToMethodID nameToMethodId[] = {
    { 1, "readAssets", METHOD_TYPE_OBJECT },
    { 2, "isAssetExist", METHOD_TYPE_INT },
    { 3, "readAssete", METHOD_TYPE_OBJECT },
    { 4, "getGLOptionLinear", METHOD_TYPE_INT },
    { 5, "SetSpeed", METHOD_TYPE_VOID },
    { 6, "getPhoneModel", METHOD_TYPE_OBJECT },
    { 7, "getAbsolueFilePath", METHOD_TYPE_OBJECT },
    { 8, "OnUIStatusChange", METHOD_TYPE_VOID },
    { 9, "OnSoundPlay", METHOD_TYPE_VOID },
    { 10, "OnStopSound", METHOD_TYPE_VOID },
    { 11, "hideLoadingDialog", METHOD_TYPE_VOID },
    { 12, "OnShowSaveButton", METHOD_TYPE_VOID },
    { 13, "OnVibrate", METHOD_TYPE_VOID },
    { 14, "OnEvent", METHOD_TYPE_VOID },
    { 15, "GFA_IsInitialized", METHOD_TYPE_BOOLEAN },
    { 16, "GFA_Init", METHOD_TYPE_BOOLEAN },
    { 17, "GFA_SetTextSize", METHOD_TYPE_VOID },
    { 18, "GFA_CreateFont", METHOD_TYPE_INT },
    { 19, "GFA_SetColor", METHOD_TYPE_VOID },
    { 20, "GFA_GetWordwrapPositionEx", METHOD_TYPE_INT },
    { 21, "GFA_SetFont", METHOD_TYPE_INT },
    { 22, "GFA_CharWidth", METHOD_TYPE_INT },
    { 23, "GFA_CharHeight", METHOD_TYPE_INT },
    { 24, "GFA_GetAscent", METHOD_TYPE_INT },
    { 25, "GFA_GetDescent", METHOD_TYPE_INT },
    { 26, "GFA_GetCurrentFont", METHOD_TYPE_INT },
    { 27, "GFA_GetColor", METHOD_TYPE_INT },
    { 28, "GFA_GetStringLength", METHOD_TYPE_INT },
    { 29, "GFA_SetStringFromKSC5601", METHOD_TYPE_VOID },
    { 30, "GFA_SetStringFromUnicode", METHOD_TYPE_VOID },
    { 31, "GFA_SetString", METHOD_TYPE_VOID },
    { 32, "GFA_SetTextAlign", METHOD_TYPE_VOID },
    { 33, "GFA_SetAntiAlias", METHOD_TYPE_VOID },
    { 34, "GFA_SetLocale", METHOD_TYPE_VOID },
    { 35, "GFA_ReleaseFont", METHOD_TYPE_VOID },
    { 36, "GFA_Release", METHOD_TYPE_VOID },
    { 37, "GFA_DrawFont", METHOD_TYPE_OBJECT },
    { 38, "GFA_DrawText", METHOD_TYPE_OBJECT },
    { 39, "GFA_MeasureText", METHOD_TYPE_OBJECT },
    { 40, "GFA_GetPixels32", METHOD_TYPE_OBJECT },
    { 41, "GFA_GetPixels16", METHOD_TYPE_OBJECT },
    { 42, "getPhoneNumber", METHOD_TYPE_OBJECT },
    { 43, "getSimSerialNumber", METHOD_TYPE_OBJECT },
    { 44, "getMacAddress", METHOD_TYPE_OBJECT },
    { 45, "getDeviceID", METHOD_TYPE_OBJECT },
    { 46, "getLocaleID", METHOD_TYPE_INT },
    { 47, "getLanguage", METHOD_TYPE_VOID },
    { 48, "getAndroidID", METHOD_TYPE_OBJECT },
    { 49, "getCarrierName", METHOD_TYPE_OBJECT },
    { 50, "getDeviceType", METHOD_TYPE_OBJECT },
    { 51, "isFileExist", METHOD_TYPE_INT },
    { 52, "saveFile", METHOD_TYPE_INT },
    { 53, "loadFile", METHOD_TYPE_OBJECT },
    { 54, "deleteFile", METHOD_TYPE_INT },
    { 55, "loadFileFromStorage", METHOD_TYPE_OBJECT },
    { 56, "getTotalMemory", METHOD_TYPE_LONG },
    { 57, "getFreeMemory", METHOD_TYPE_LONG },
    { 58, "getMaxMemory", METHOD_TYPE_LONG },
    { 59, "getUsedMemory", METHOD_TYPE_LONG },
    { 60, "isNetAvailable", METHOD_TYPE_INT },
    { 61, "netConnect", METHOD_TYPE_INT },
    { 62, "OnAsyncTimerSet", METHOD_TYPE_VOID },
    { 63, "showTitleComponent", METHOD_TYPE_VOID },
    { 64, "hideTitleComponent", METHOD_TYPE_VOID },
    { 65, "showLoadingDialog", METHOD_TYPE_VOID },
    { 66, "showVpointComponent", METHOD_TYPE_VOID },
    { 67, "hideVpointComponent", METHOD_TYPE_VOID },
    { 68, "showPurchaseComponent", METHOD_TYPE_VOID },
    { 69, "hidePurchaseComponent", METHOD_TYPE_VOID },
    { 70, "openPurchasePopup", METHOD_TYPE_VOID },
    { 71, "startPurchase", METHOD_TYPE_VOID },
    { 72, "getDoubleKeyStatus", METHOD_TYPE_INT },
    { 73, "stopAndroidSound", METHOD_TYPE_VOID },
    { 74, "vibrateAndroid", METHOD_TYPE_VOID },
    { 75, "changeUIStatus", METHOD_TYPE_VOID },
    { 76, "getVPoint", METHOD_TYPE_VOID },
};

MethodsBoolean methodsBoolean[] = {
    { 15, Advena_GFA_IsInitialized },
    { 16, Advena_GFA_Init },
};

MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {};
MethodsDouble methodsDouble[] = {};
MethodsFloat methodsFloat[] = {};

MethodsInt methodsInt[] = {
    { 2, Advena_isAssetExist },
    { 4, Advena_getGLOptionLinear },
    { 18, Advena_GFA_CreateFont },
    { 20, Advena_IntZero },
    { 21, Advena_GFA_SetFont },
    { 22, Advena_GFA_CharWidth },
    { 23, Advena_GFA_CharHeight },
    { 24, Advena_GFA_GetAscent },
    { 25, Advena_GFA_GetDescent },
    { 26, Advena_GFA_GetCurrentFont },
    { 27, Advena_GFA_GetColor },
    { 28, Advena_GFA_GetStringLength },
    { 46, Advena_getLocaleID },
    { 51, Advena_isFileExist },
    { 52, Advena_saveFile },
    { 54, Advena_deleteFile },
    { 60, Advena_isNetAvailable },
    { 61, Advena_netConnect },
    { 72, Advena_IntZero },
};

MethodsLong methodsLong[] = {
    { 56, Advena_getTotalMemory },
    { 57, Advena_getFreeMemory },
    { 58, Advena_getMaxMemory },
    { 59, Advena_getUsedMemory },
};

MethodsObject methodsObject[] = {
    { 1, Advena_readAssets },
    { 3, Advena_readAssets },
    { 6, Advena_getPhoneModel },
    { 7, Advena_getAbsolueFilePath },
    { 37, Advena_GFA_DrawFont },
    { 38, Advena_GFA_GetPixels32 },
    { 39, Advena_GFA_GetPixels32 },
    { 40, Advena_GFA_GetPixels32 },
    { 41, Advena_GFA_GetPixels16 },
    { 42, Advena_getPhoneNumber },
    { 43, Advena_getSimSerialNumber },
    { 44, Advena_getMacAddress },
    { 45, Advena_getDeviceID },
    { 48, Advena_getAndroidID },
    { 49, Advena_getCarrierName },
    { 50, Advena_getDeviceType },
    { 53, Advena_loadFile },
    { 55, Advena_loadFileFromStorage },
};

MethodsShort methodsShort[] = {};

MethodsVoid methodsVoid[] = {
    { 5, Advena_SetSpeed },
    { 8, Advena_OnUIStatusChange },
    { 9, Advena_OnSoundPlay },
    { 10, Advena_OnStopSound },
    { 11, Advena_VoidNoop },
    { 12, Advena_VoidNoop },
    { 13, Advena_OnVibrate },
    { 14, Advena_OnEvent },
    { 17, Advena_GFA_SetTextSize },
    { 19, Advena_GFA_SetColor },
    { 29, Advena_VoidNoop },
    { 30, Advena_VoidNoop },
    { 31, Advena_GFA_SetString },
    { 32, Advena_VoidNoop },
    { 33, Advena_VoidNoop },
    { 34, Advena_VoidNoop },
    { 35, Advena_VoidNoop },
    { 36, Advena_VoidNoop },
    { 47, Advena_getLanguage },
    { 62, Advena_OnAsyncTimerSet },
    { 63, Advena_VoidNoop },
    { 64, Advena_VoidNoop },
    { 65, Advena_VoidNoop },
    { 66, Advena_VoidNoop },
    { 67, Advena_VoidNoop },
    { 68, Advena_VoidNoop },
    { 69, Advena_VoidNoop },
    { 70, Advena_VoidNoop },
    { 71, Advena_VoidNoop },
    { 73, Advena_OnStopSound },
    { 74, Advena_OnVibrate },
    { 75, Advena_OnUIStatusChange },
    { 76, Advena_VoidNoop },
};

/*
 * Fields
 */

char WINDOW_SERVICE[] = "window";
const int SDK_INT = 19;

NameToFieldID nameToFieldId[] = {
    { 0, "WINDOW_SERVICE", FIELD_TYPE_OBJECT }, 
    { 1, "SDK_INT", FIELD_TYPE_INT },
};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = {
    { 1, SDK_INT },
};
FieldsObject fieldsObject[] = {
    { 0, WINDOW_SERVICE },
};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

__FALSOJNI_IMPL_CONTAINER_SIZES
