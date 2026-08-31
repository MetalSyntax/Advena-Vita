/**
 * @brief Copyright (C) 2023 Volodymyr Atamanenko This software may be modified and distributed under the terms of the MIT license.
 * @note See `docs/source/patch.md:1` for detailed design rationale.
 */

/**
 * @brief Patching some of the .
 */

#include <kubridge.h>
#include <so_util/so_util.h>
#include <string.h>
#include <stdio.h>
#include "utils/logger.h"

extern so_module so_mod;


/**
 * @brief Teamstrike [T] button position fix ------------------------------------- GVUIPlayerController::ShowBtn (patched below) only ever toggles.
 * @note See `docs/source/patch.md:23` for detailed design rationale.
 */
typedef void (*fn_GVUIObject_SetPosition)(void *obj, int x, int y);
static fn_GVUIObject_SetPosition GVUIObject_SetPosition_real = NULL;
static so_hook InitialPlayerPadSet_hook;

#define TEAMSTRIKE_POS_X_PCT 0.06f  // ~6% in from the left edge (next to the D-pad)
#define TEAMSTRIKE_POS_Y_PCT 0.40f  // ~40% down (mid-height, above the D-pad)

static void InitialPlayerPadSet_patched(void *this_ptr) {
    /**< @brief Run the original layout logic first, unmodified. */
    kuKernelCpuUnrestrictedMemcpy((void *)InitialPlayerPadSet_hook.addr,
                                  InitialPlayerPadSet_hook.orig_instr,
                                  sizeof(InitialPlayerPadSet_hook.orig_instr));
    kuKernelFlushCaches((void *)InitialPlayerPadSet_hook.addr, sizeof(InitialPlayerPadSet_hook.orig_instr));
    ((void (*)(void *))InitialPlayerPadSet_hook.thumb_addr)(this_ptr);
    kuKernelCpuUnrestrictedMemcpy((void *)InitialPlayerPadSet_hook.addr,
                                  InitialPlayerPadSet_hook.patch_instr,
                                  sizeof(InitialPlayerPadSet_hook.patch_instr));
    kuKernelFlushCaches((void *)InitialPlayerPadSet_hook.addr, sizeof(InitialPlayerPadSet_hook.patch_instr));

    int anchor_x = *(int *)((char *)this_ptr + 0x1ec);
    int anchor_y = *(int *)((char *)this_ptr + 0x1f0);
    int screen_w = anchor_x + 120;
    int height_comp = anchor_y + 80;

    void *teamstrike_obj = *(void **)((char *)this_ptr + 0x1b0);
    if (teamstrike_obj && GVUIObject_SetPosition_real) {
        int corrected_x = (int)(screen_w * TEAMSTRIKE_POS_X_PCT);
        int corrected_y = (int)(height_comp * TEAMSTRIKE_POS_Y_PCT);
        GVUIObject_SetPosition_real(teamstrike_obj, corrected_x, corrected_y);
        l_info("[Patch] Teamstrike [T] repositioned: engine ScreenW=%d HeightComp=%d -> corrected pos (%d,%d)",
               screen_w, height_comp, corrected_x, corrected_y);
    } else {
        l_warn("[Patch] Teamstrike button object missing at +0x1b0; position NOT corrected.");
    }
}

/**
 * @brief Load Game crash fix (Data Abort in CGsEncryptFile::ReadPtr) ----------- Reproduced on-console: choosing "Load Game" reliably Data Aborts.
 * @note See `docs/source/patch.md:113` for detailed design rationale.
 */
typedef void (*fn_CGsEncryptFile_ReadPtr)(void *this_ptr, void *dst, uint32_t len);
static so_hook ReadPtr_hook;

static void CGsEncryptFile_ReadPtr_patched(void *this_ptr, void *dst, uint32_t len) {
    void *buffer = *(void **)((char *)this_ptr + 0x54);
    if (buffer == NULL) {
        l_warn("[Patch] CGsEncryptFile::ReadPtr: buffer is NULL (LoadBegin failed to load "
               "the save file) -- zero-filling %u bytes instead of crashing.", (unsigned int)len);
        memset(dst, 0, len);
        *(uint32_t *)((char *)this_ptr + 0x50) += len;
        return;
    }

    /**
     * @brief Buffer is valid: run the original implementation untouched.
     * @note See `docs/source/patch.md:173` for detailed design rationale.
     */
    kuKernelCpuUnrestrictedMemcpy((void *)ReadPtr_hook.addr, ReadPtr_hook.orig_instr,
                                  sizeof(ReadPtr_hook.orig_instr));
    kuKernelFlushCaches((void *)ReadPtr_hook.addr, sizeof(ReadPtr_hook.orig_instr));
    ((fn_CGsEncryptFile_ReadPtr)ReadPtr_hook.thumb_addr)(this_ptr, dst, len);
    kuKernelCpuUnrestrictedMemcpy((void *)ReadPtr_hook.addr, ReadPtr_hook.patch_instr,
                                  sizeof(ReadPtr_hook.patch_instr));
    kuKernelFlushCaches((void *)ReadPtr_hook.addr, sizeof(ReadPtr_hook.patch_instr));
}

/**
 * @brief Crash fix: CUIMenuStatus::PointerRelease (SO offset 0xe8608).
 * @note See `docs/source/patch.md:186` for detailed design rationale.
 */
typedef void (*fn_CUIMenuStatus_PointerRelease)(void *this_ptr, void *param_1);
static so_hook CUIMenuStatus_PR_hook;

static void CUIMenuStatus_PR_patched(void *this_ptr, void *param_1) {
    int *sub = (int *)((char *)this_ptr + 0xe0);
    for (int i = 0; i < 5; i++) {
        if (sub[i] == 0) {
            l_warn("[Patch] CUIMenuStatus::PointerRelease: sub-object at 0x%x is NULL "
                   "(menu not fully initialised), skipping.", 0xe0 + i * 4);
            return;
        }
    }
    kuKernelCpuUnrestrictedMemcpy((void *)CUIMenuStatus_PR_hook.addr,
                                  CUIMenuStatus_PR_hook.orig_instr,
                                  sizeof(CUIMenuStatus_PR_hook.orig_instr));
    kuKernelFlushCaches((void *)CUIMenuStatus_PR_hook.addr,
                        sizeof(CUIMenuStatus_PR_hook.orig_instr));
    ((fn_CUIMenuStatus_PointerRelease)CUIMenuStatus_PR_hook.thumb_addr)(this_ptr, param_1);
    kuKernelCpuUnrestrictedMemcpy((void *)CUIMenuStatus_PR_hook.addr,
                                  CUIMenuStatus_PR_hook.patch_instr,
                                  sizeof(CUIMenuStatus_PR_hook.patch_instr));
    kuKernelFlushCaches((void *)CUIMenuStatus_PR_hook.addr,
                        sizeof(CUIMenuStatus_PR_hook.patch_instr));
}

/**
 * @brief Crash fix: CUISubMenuReinForced::PointerRelease (SO offset 0xfcb78).
 * @note See `docs/source/patch.md:233` for detailed design rationale.
 */
typedef void (*fn_CUISubMenuReinForced_PointerRelease)(void *this_ptr, void *param_1);
static so_hook CUISubMenuReinForced_PR_hook;

static void CUISubMenuReinForced_PR_patched(void *this_ptr, void *param_1) {
    uint8_t flag = *(uint8_t *)((char *)this_ptr + 0xec);
    if (flag == 0) {
        if (*(int *)((char *)this_ptr + 0xd8) == 0) {
            l_warn("[Patch] CUISubMenuReinForced::PointerRelease: sub-object at 0xd8 "
                   "is NULL (0xec==0), skipping.");
            return;
        }
    } else {
        if (*(int *)((char *)this_ptr + 0x44) == 0) {
            l_warn("[Patch] CUISubMenuReinForced::PointerRelease: sub-object at 0x44 "
                   "is NULL (0xec!=0), skipping.");
            return;
        }
    }
    kuKernelCpuUnrestrictedMemcpy((void *)CUISubMenuReinForced_PR_hook.addr,
                                  CUISubMenuReinForced_PR_hook.orig_instr,
                                  sizeof(CUISubMenuReinForced_PR_hook.orig_instr));
    kuKernelFlushCaches((void *)CUISubMenuReinForced_PR_hook.addr,
                        sizeof(CUISubMenuReinForced_PR_hook.orig_instr));
    ((fn_CUISubMenuReinForced_PointerRelease)CUISubMenuReinForced_PR_hook.thumb_addr)(this_ptr, param_1);
    kuKernelCpuUnrestrictedMemcpy((void *)CUISubMenuReinForced_PR_hook.addr,
                                  CUISubMenuReinForced_PR_hook.patch_instr,
                                  sizeof(CUISubMenuReinForced_PR_hook.patch_instr));
    kuKernelFlushCaches((void *)CUISubMenuReinForced_PR_hook.addr,
                        sizeof(CUISubMenuReinForced_PR_hook.patch_instr));
}

/**
 * @brief Crash fix: CCharObject::ClearMsgState (SO offset 0x9746c) --------------------------------------------------------------------------- Root.
 * @note See `docs/source/patch.md:292` for detailed design rationale.
 */
typedef void (*fn_CCharObject_ClearMsgState)(void *this_ptr);
static so_hook CCharObject_ClearMsgState_hook;

static void CCharObject_ClearMsgState_patched(void *this_ptr) {
    void *msgbox = *(void **)((char *)this_ptr + 0x5c);
    void *msgicon = *(void **)((char *)this_ptr + 0x64);
    void *other = *(void **)((char *)this_ptr + 0x74);
    if (!msgbox || !msgicon || !other) {
        l_warn("[Patch] CCharObject::ClearMsgState: sub-object missing (msgbox=%p msgicon=%p "
               "other=%p), skipping (object not fully initialised yet).", msgbox, msgicon, other);
        return;
    }

    kuKernelCpuUnrestrictedMemcpy((void *)CCharObject_ClearMsgState_hook.addr,
                                  CCharObject_ClearMsgState_hook.orig_instr,
                                  sizeof(CCharObject_ClearMsgState_hook.orig_instr));
    kuKernelFlushCaches((void *)CCharObject_ClearMsgState_hook.addr,
                        sizeof(CCharObject_ClearMsgState_hook.orig_instr));
    ((fn_CCharObject_ClearMsgState)CCharObject_ClearMsgState_hook.thumb_addr)(this_ptr);
    kuKernelCpuUnrestrictedMemcpy((void *)CCharObject_ClearMsgState_hook.addr,
                                  CCharObject_ClearMsgState_hook.patch_instr,
                                  sizeof(CCharObject_ClearMsgState_hook.patch_instr));
    kuKernelFlushCaches((void *)CCharObject_ClearMsgState_hook.addr,
                        sizeof(CCharObject_ClearMsgState_hook.patch_instr));
}

/**
 * @brief Crash fix: CSaveMgr::Save (SO offset 0xab1e4) --------------------------------------------------------------------------- Reproduced.
 * @note See `docs/source/patch.md:344` for detailed design rationale.
 */
typedef int (*fn_CTotalObjMgr_GetRealPlayer)(void);
static fn_CTotalObjMgr_GetRealPlayer CTotalObjMgr_GetRealPlayer_real = NULL;
typedef void (*fn_CSaveMgr_Save)(void *this_ptr);
static so_hook CSaveMgr_Save_hook;

static void CSaveMgr_Save_patched(void *this_ptr) {
    int player = CTotalObjMgr_GetRealPlayer_real ? CTotalObjMgr_GetRealPlayer_real() : 0;
    if (!player) {
        l_warn("[Patch] CSaveMgr::Save: CTotalObjMgr::GetRealPlayer() is NULL, skipping this "
               "autosave instead of crashing (player object not attached yet).");
        return;
    }

    kuKernelCpuUnrestrictedMemcpy((void *)CSaveMgr_Save_hook.addr,
                                  CSaveMgr_Save_hook.orig_instr,
                                  sizeof(CSaveMgr_Save_hook.orig_instr));
    kuKernelFlushCaches((void *)CSaveMgr_Save_hook.addr,
                        sizeof(CSaveMgr_Save_hook.orig_instr));
    ((fn_CSaveMgr_Save)CSaveMgr_Save_hook.thumb_addr)(this_ptr);
    kuKernelCpuUnrestrictedMemcpy((void *)CSaveMgr_Save_hook.addr,
                                  CSaveMgr_Save_hook.patch_instr,
                                  sizeof(CSaveMgr_Save_hook.patch_instr));
    kuKernelFlushCaches((void *)CSaveMgr_Save_hook.addr,
                        sizeof(CSaveMgr_Save_hook.patch_instr));
}

/**< @brief Perf diagnostic (Bug 16, PORTING_PLAN.md, 4th pass). */
#ifdef INSTRUMENT_BLIT_CALLS
typedef void (*fn_PutCompressImg)(int, int, int, int, unsigned char *, unsigned short *,
                                   int /* enumDrawOP */, int, long);
static so_hook PutCompressImg_hook;
static uint32_t instr_blit_calls = 0;

/**
 * @brief enumDrawOP -> operation name, straight from CMvGraphics::InitialBlend() (out_ghidra.c:214383-214425), which registers each op's concrete.
 * @note See `docs/source/patch.md:435` for detailed design rationale.
 */
#define BLIT_OP_HIST_SIZE 32
static uint32_t instr_blit_by_op[BLIT_OP_HIST_SIZE] = {0};

static const char *blit_op_name(int op) {
    switch (op) {
        case 0x0: return "COPY";
        case 0x1: return "BLEND16";
        case 0x2: return "ADD";
        case 0x4: return "VOID";
        case 0x5: return "SHADOW";
        case 0x6: return "LIGHTEN";
        case 0x7: return "DARKEN";
        case 0x9: return "NEGATIVE";
        case 0xa: return "GRAY";
        case 0xb: return "RGB";
        case 0xc: return "RGBHALF";
        case 0xd: return "RGBADD";
        case 0xe: return "RGBMULTI";
        case 0xf: return "OUTLINE";
        case 0x10: return "ENLARGE";
        case 0x13: return "FX";
        default: return "?";
    }
}

static void PutCompressImg_counted(int p1, int p2, int p3, int p4, unsigned char *p5,
                                    unsigned short *p6, int p7, int p8, long p9) {
    instr_blit_calls++;
    if (p7 >= 0 && p7 < BLIT_OP_HIST_SIZE) {
        instr_blit_by_op[p7]++;
    }

    kuKernelCpuUnrestrictedMemcpy((void *)PutCompressImg_hook.addr,
                                  PutCompressImg_hook.orig_instr,
                                  sizeof(PutCompressImg_hook.orig_instr));
    kuKernelFlushCaches((void *)PutCompressImg_hook.addr,
                        sizeof(PutCompressImg_hook.orig_instr));
    ((fn_PutCompressImg)PutCompressImg_hook.thumb_addr)(p1, p2, p3, p4, p5, p6, p7, p8, p9);
    kuKernelCpuUnrestrictedMemcpy((void *)PutCompressImg_hook.addr,
                                  PutCompressImg_hook.patch_instr,
                                  sizeof(PutCompressImg_hook.patch_instr));
    kuKernelFlushCaches((void *)PutCompressImg_hook.addr,
                        sizeof(PutCompressImg_hook.patch_instr));
}

uint32_t patch_get_and_reset_blit_calls() {
    uint32_t v = instr_blit_calls;
    instr_blit_calls = 0;
    return v;
}

/**
 * @brief Writes a compact "NAME:count,NAME:count,.
 * @note See `docs/source/patch.md:495` for detailed design rationale.
 */
void patch_format_and_reset_blit_histogram(char *buf, int buflen) {
    int off = 0;
    buf[0] = '\0';
    for (int i = 0; i < BLIT_OP_HIST_SIZE; i++) {
        if (instr_blit_by_op[i] > 0 && off < buflen - 1) {
            int written = snprintf(buf + off, buflen - off, "%s%s:%u",
                                   (off > 0) ? "," : "", blit_op_name(i), instr_blit_by_op[i]);
            if (written > 0) {
                off += written;
            }
        }
        instr_blit_by_op[i] = 0;
    }
}
#endif

// ---------------------------------------------------------------------------
// Perf fix (Bug 16, PORTING_PLAN.md, 5th pass): full replacement of
// DrawOP_COPY_Compress_16 (SO offset 0x142588)
// ---------------------------------------------------------------------------
// The 4th perf pass identified DrawOP_COPY_Compress_16_Auto (dispatching,
// for the common case, straight to DrawOP_COPY_Compress_16) as ~79% of all
// PutCompressImg sprite blits during real combat -- by far the dominant
// cost. This is a genuine algorithm replacement, NOT a guard/crash-fix
// hook: unlike every other hook in this file, the patched function NEVER
// calls the original -- it fully replaces it, so it changes what code
// actually runs on every opaque sprite pixel in the game. That means it
// needs on-console visual verification before it's trusted, which is why
// it's gated behind its own opt-in flag (OPT_COPY_BLIT_REWRITE, default
// OFF) instead of being always-on like the crash-fix hooks above.
//
// Format (reverse-engineered from the real pseudo-C,
// out_ghidra.c:261348-261434, and cross-checked byte-for-byte against this
// reimplementation): a per-scanline RLE stream of unsigned char* compressed
// data, decoded as a run of little-endian uint16 tokens:
//   - optional 10-byte extended header, present iff the very first int16 at
//     the start of the stream == -5 (0xFFFB) -- the real token stream then
//     starts 10 bytes in instead of at the very start.
//   - 0xFFFF: end of sprite data, stop.
//   - 0xFFFE: next scanline -- dst += stride (row-advance marker; does NOT
//     consume any further bytes from the compressed stream).
//   - high bit set (token & 0x8000): "opaque run" -- token & 0x7FFF is a
//     pixel count; for each pixel, read one index byte from the compressed
//     stream and write palette[index] (16-bit RGB565) to the destination.
//   - otherwise (token < 0x8000): "transparent run" -- skip `token` pixels
//     in the destination WITHOUT reading from the compressed stream or
//     writing anything (the transparent pixels already show whatever the
//     software framebuffer had there).
//
// WHY this is the real hot loop and what actually gets fixed here: the
// live disassembly of the ORIGINAL .so's DrawOP_COPY_Compress_16 (`arm-
// vita-eabi-objdump -Mforce-thumb`, `.so+0x142588`) shows the compiler that
// built this game's armeabi/ARMv5TE-targeted .so spilled the opaque-run
// pixel counter (the decompiled `local_22`) to the STACK and reloaded it
// via `ldrh`/`strh` on almost every single loop iteration, instead of
// keeping it in a register -- ~9-10 Thumb-1 instructions per pixel for
// what should be 3-4. This is NOT something a NEON rewrite fixes: the
// per-pixel work is a byte-indexed lookup into a 256-entry x 16-bit
// palette, and ARMv7 NEON has no gather instruction for that (VTBL only
// covers a 32-byte/32-entry table, 8x too small for a 256-entry palette;
// building a 256-entry lookup out of chained VTBL passes is realistically
// no faster than the fix below and adds real correctness risk for an
// unverified payoff). The actual, low-risk win is just recompiling the
// SAME algorithm with a modern compiler at -O3 -mcpu=cortex-a9 (this
// project's existing flags, see CMakeLists.txt) in ARM mode instead of
// legacy Thumb-1: a sane register allocator keeps the loop counter in a
// register and uses post-increment addressing, cutting per-pixel work to
// a handful of instructions with zero algorithm changes.
#ifdef OPT_COPY_BLIT_REWRITE
static void DrawOP_COPY_Compress_16_rewrite(unsigned short *dst, unsigned char *src,
                                             unsigned short *palette, int stride) {
    unsigned char *p = src;
    // Extended-header marker: identical semantics to
    // `*(short *)param_2 == -5` in the decompiled original, but read
    // byte-wise (matches how the rest of this function reads tokens, and
    // avoids relying on 2-byte alignment of `src`).
    if ((short)((unsigned int)src[0] | ((unsigned int)src[1] << 8)) == -5) {
        p = src + 10;
    }

    for (;;) {
        unsigned int token = (unsigned int)p[0] | ((unsigned int)p[1] << 8);
        if (token == 0xFFFF) {
            return;
        }
        p += 2;
        if (token == 0xFFFE) {
            dst += stride;
        } else if (token & 0x8000) {
            unsigned int count = token & 0x7FFF;
            for (unsigned int i = 0; i < count; i++) {
                *dst++ = palette[*p++];
            }
        } else {
            dst += token;
        }
    }
}

// Unlike every other hook in this file, this one is installed once and left
// in place permanently -- there's no orig_instr to restore-call-repatch
// because the original machine code is never run again after this.
static so_hook DrawOP_COPY_Compress_16_hook;
#endif

// ---------------------------------------------------------------------------
// Perf fix (Bug 16, PORTING_PLAN.md, 7th pass): DrawOP_DARKEN_Compress_16
// and DrawOP_ADD_Compress_16 (full replacement, same rationale/risk as
// DrawOP_COPY_Compress_16 above -- same compiler, same armeabi/Thumb-1
// stack-spill pattern, same fix of just recompiling the identical algorithm
// at this project's existing -O3 -mcpu=cortex-a9 flags), plus a partial fix
// for DrawOP_BLEND16_Compress_16 (see the comment above that one below).
// Real offsets confirmed via `nm -D` on the real .so (all three are
// exported symbols, same as PutCompressImg/DrawOP_COPY_Compress_16 already
// were): DrawOP_DARKEN_Compress_16 = 0x13d888, DrawOP_ADD_Compress_16 =
// 0x13cf0c, DrawOP_BLEND16_Compress_16 = 0x13d388. Per the 5th-pass
// histogram these three ops are ~16.7% of all sprite blits in combat
// (DARKEN 6.2%, BLEND16 5.7%, ADD 4.8%), the next-highest ROI after COPY's
// ~79%. Gated behind its own flag (OPT_MORE_BLIT_REWRITE, default OFF,
// same discipline as OPT_COPY_BLIT_REWRITE before its own hardware
// verification) until confirmed visually on real hardware.
#ifdef OPT_MORE_BLIT_REWRITE
// DrawOP_DARKEN_Compress_16: per-channel min(dst, src) -- no alpha
// parameter, no lookup tables, byte-exact against
// out_ghidra.c:256313-256385.
static void DrawOP_DARKEN_Compress_16_rewrite(unsigned short *dst, unsigned char *src,
                                               unsigned short *palette, int stride) {
    unsigned char *p = src;
    if ((short)((unsigned int)src[0] | ((unsigned int)src[1] << 8)) == -5) {
        p = src + 10;
    }

    for (;;) {
        unsigned int token = (unsigned int)p[0] | ((unsigned int)p[1] << 8);
        if (token == 0xFFFF) {
            return;
        }
        p += 2;
        if (token == 0xFFFE) {
            dst += stride;
        } else if (token & 0x8000) {
            unsigned int count = token & 0x7FFF;
            for (unsigned int i = 0; i < count; i++) {
                unsigned int d = *dst;
                unsigned int s = palette[*p++];
                unsigned int r = (s & 0xf800) < (d & 0xf800) ? (s & 0xf800) : (d & 0xf800);
                unsigned int g = (s & 0x7e0) < (d & 0x7e0) ? (s & 0x7e0) : (d & 0x7e0);
                unsigned int b = (s & 0x1f) < (d & 0x1f) ? (s & 0x1f) : (d & 0x1f);
                *dst++ = (unsigned short)(r | g | b);
            }
        } else {
            dst += token;
        }
    }
}
static so_hook DrawOP_DARKEN_Compress_16_hook;

// DrawOP_ADD_Compress_16: per-channel saturating add, weighted by `alpha`
// (0 < alpha < 0x100; alpha == 0xff is a pure unweighted saturating add,
// any other value scales the source contribution by alpha/256 before
// adding). No lookup tables -- byte-exact against
// out_ghidra.c:255717-255849.
static void DrawOP_ADD_Compress_16_rewrite(unsigned short *dst, unsigned char *src,
                                            unsigned short *palette, int stride, int alpha) {
    if (alpha <= 0 || alpha >= 0x100) {
        return;
    }

    unsigned char *p = src;
    if ((short)((unsigned int)src[0] | ((unsigned int)src[1] << 8)) == -5) {
        p = src + 10;
    }

    for (;;) {
        unsigned int token = (unsigned int)p[0] | ((unsigned int)p[1] << 8);
        if (token == 0xFFFF) {
            return;
        }
        p += 2;
        if (token == 0xFFFE) {
            dst += stride;
        } else if (token & 0x8000) {
            unsigned int count = token & 0x7FFF;
            if (alpha == 0xff) {
                for (unsigned int i = 0; i < count; i++) {
                    unsigned int s = palette[*p++];
                    unsigned int d = *dst;
                    unsigned int b = (s & 0x1f) + (d & 0x1f);
                    if (b > 0x1f) b = 0x1f;
                    unsigned int g = (s & 0x7e0) + (d & 0x7e0);
                    if (g > 0x7e0) g = 0x7e0;
                    unsigned int r = (s >> 11) + (d >> 11);
                    if (r > 0x1f) r = 0x1f;
                    *dst++ = (unsigned short)(g | b | (r << 11));
                }
            } else {
                for (unsigned int i = 0; i < count; i++) {
                    unsigned int s = palette[*p++];
                    unsigned int d = *dst;
                    int r = (((int)(s >> 11) * alpha) >> 8) + (int)(d >> 11);
                    if (r > 0x1f) r = 0x1f;
                    int b = (((int)(s & 0x1f) * alpha) >> 8) + (int)(d & 0x1f);
                    if (b > 0x1f) b = 0x1f;
                    int g = (((int)((s >> 5) & 0x3f) * alpha) >> 8) + (int)((d >> 5) & 0x3f);
                    if (g > 0x3f) g = 0x3f;
                    *dst++ = (unsigned short)(b | (r << 11) | (g << 5));
                }
            }
        } else {
            dst += token;
        }
    }
}
static so_hook DrawOP_ADD_Compress_16_hook;

// DrawOP_BLEND16_Compress_16: PARTIAL fix, unlike the two above. The
// original (out_ghidra.c:256027-256149) has two paths: alpha == 8 (the
// 50/50 midpoint of its 1-15 range) uses a simple mask-and-shift average
// trick with no lookup tables (byte-exact below); every OTHER alpha value
// (1-7, 9-15) reads two precomputed tables in the .so's rodata
// (`DAT_00175d90`, 16 bytes/level of channel masks, and `DAT_00175e80`, 8
// bytes/level of shift amounts) to do a per-channel weighted blend without
// the alpha==8 shortcut. Those table VALUES could not be safely recovered
// from the ELF (the referenced range 0x175d90 falls in a gap between this
// .so's two PT_LOAD segments in the file layout actually shipped, so a
// direct file-offset dump is not trustworthy) -- guessing an
// "equivalent" formula instead of the real table would risk silently
// wrong blend colors on whatever alpha levels are wrong, which is worse
// than leaving that path at its original (slower but correct) speed. So
// this hook only fast-paths the alpha==8 case (a real, common case per
// the 5th-pass histogram: BLEND16 is used for many hit-flash/highlight
// effects, which are plausibly implemented as a flat 50% blend) and
// restore-call-repatches into the untouched original for every other
// alpha, exactly like the guard-style hooks elsewhere in this file (e.g.
// CSaveMgr_Save_patched) -- NOT a full replacement like COPY/DARKEN/ADD
// above.
typedef void (*fn_DrawOP_BLEND16_Compress_16)(unsigned short *, unsigned char *,
                                               unsigned short *, int, int);
static so_hook DrawOP_BLEND16_Compress_16_hook;

#ifdef INSTRUMENT_BLEND16_TABLES
/**
 * @brief Dumps the real bytes of DrawOP_BLEND16_Compress_16's two lookup
 * tables (out_ghidra.c's `DAT_00175d90`/`DAT_00175e80`) straight from this
 * process's own mapped memory the first time gameplay hits a non-8 alpha,
 * instead of guessing from the static ELF file (see the comment block
 * above this function for why that was unsafe). `so_mod.text_base` is the
 * same runtime base every other hook in this file already uses, so this is
 * exactly the memory the running .so itself dereferences -- no gdbstub
 * needed. 15 levels (alpha 1-15, level 7 unused since alpha==8 has its own
 * fast path): 8 ushort masks (16 bytes) + 8 byte shifts (8 bytes) each.
 */
static void dump_blend16_tables_once(void) {
    static int dumped = 0;
    if (dumped) {
        return;
    }
    dumped = 1;

    const unsigned short *masks = (const unsigned short *)(so_mod.text_base + 0x175d90);
    const unsigned char *shifts = (const unsigned char *)(so_mod.text_base + 0x175e80);
    l_info("[Patch] DrawOP_BLEND16_Compress_16 tables dump (level = alpha-1, level 7 unused):");
    for (int level = 0; level < 15; level++) {
        const unsigned short *m = masks + level * 8;
        const unsigned char *s = shifts + level * 8;
        l_info("[Patch]   level=%d masks=%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x shifts=%u,%u,%u,%u,%u,%u,%u,%u",
               level, m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7],
               s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7]);
    }
}
#endif

static void DrawOP_BLEND16_Compress_16_patched(unsigned short *dst, unsigned char *src,
                                                unsigned short *palette, int stride, int alpha) {
    if (alpha != 8) {
#ifdef INSTRUMENT_BLEND16_TABLES
        dump_blend16_tables_once();
#endif
        kuKernelCpuUnrestrictedMemcpy((void *)DrawOP_BLEND16_Compress_16_hook.addr,
                                      DrawOP_BLEND16_Compress_16_hook.orig_instr,
                                      sizeof(DrawOP_BLEND16_Compress_16_hook.orig_instr));
        kuKernelFlushCaches((void *)DrawOP_BLEND16_Compress_16_hook.addr,
                            sizeof(DrawOP_BLEND16_Compress_16_hook.orig_instr));
        ((fn_DrawOP_BLEND16_Compress_16)DrawOP_BLEND16_Compress_16_hook.thumb_addr)(
            dst, src, palette, stride, alpha);
        kuKernelCpuUnrestrictedMemcpy((void *)DrawOP_BLEND16_Compress_16_hook.addr,
                                      DrawOP_BLEND16_Compress_16_hook.patch_instr,
                                      sizeof(DrawOP_BLEND16_Compress_16_hook.patch_instr));
        kuKernelFlushCaches((void *)DrawOP_BLEND16_Compress_16_hook.addr,
                            sizeof(DrawOP_BLEND16_Compress_16_hook.patch_instr));
        return;
    }

    unsigned char *p = src;
    if ((short)((unsigned int)src[0] | ((unsigned int)src[1] << 8)) == -5) {
        p = src + 10;
    }

    for (;;) {
        unsigned int token = (unsigned int)p[0] | ((unsigned int)p[1] << 8);
        if (token == 0xFFFF) {
            return;
        }
        p += 2;
        if (token == 0xFFFE) {
            dst += stride;
        } else if (token & 0x8000) {
            unsigned int count = token & 0x7FFF;
            for (unsigned int i = 0; i < count; i++) {
                unsigned short d = *dst;
                unsigned short s = palette[*p++];
                *dst++ = (unsigned short)(((d & 0xf7de) >> 1) + ((s & 0xf7de) >> 1));
            }
        } else {
            dst += token;
        }
    }
}
#endif

void so_patch(void) {
    static int patches_applied = 0;
    if (patches_applied) {
        l_warn("[Patch] so_patch called more than once; ignoring duplicate invocation.");
        return;
    }
    patches_applied = 1;

    /**< @brief 1. Patch GVUIPlayerController::ShowBtn(int mask) at offset 0x88f82 and 0x88fba. */
    uint16_t show_all_btns_patch = 0x261f;
    uint16_t nop_hide_patch = 0xbf00;
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x88f82), &show_all_btns_patch, sizeof(show_all_btns_patch));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x88fba), &nop_hide_patch, sizeof(nop_hide_patch));
    l_info("[Patch] Patched GVUIPlayerController::ShowBtn to force all UI buttons (including [T] Teamstrike) visible.");

    /**< @brief 2. Patch CSaveMgr::IsLeaglSaveData(int) at offset 0xab424. */
    uint32_t always_legal_save_patch = 0x47702001;
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0xab424), &always_legal_save_patch, sizeof(always_legal_save_patch));
    l_info("[Patch] Patched CSaveMgr::IsLeaglSaveData to always return true.");

    /**
     * @brief 3. Patch CGsEncryptFile::LoadBegin(char const*, bool) at offset 0x7492e and 0x74952.
     * @note See `docs/source/patch.md:539` for detailed design rationale.
     */
    uint16_t nop_phone_check = 0xbf00;
    uint16_t branch_always_valid = 0xe007;
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x7492e), &nop_phone_check, sizeof(nop_phone_check));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x74952), &branch_always_valid, sizeof(branch_always_valid));
    l_info("[Patch] Patched CGsEncryptFile::LoadBegin to bypass save encryption checksum and phone ID checks.");

    /**
     * @brief 4. Hook GVUIPlayerController::InitialPlayerPadSet at offset 0x896cc.
     * @note See `docs/source/patch.md:549` for detailed design rationale.
     */
    GVUIObject_SetPosition_real = (fn_GVUIObject_SetPosition)(so_mod.text_base + 0x88388 + 1);
    InitialPlayerPadSet_hook = hook_thumb(so_mod.text_base + 0x896cc + 1, (uintptr_t)InitialPlayerPadSet_patched);
    l_info("[Patch] Hooked GVUIPlayerController::InitialPlayerPadSet to fix the Teamstrike [T] button position.");


    /**
     * @brief 6. Hook CGsEncryptFile::ReadPtr at offset 0x74714.
     * @note See `docs/source/patch.md:564` for detailed design rationale.
     */
    ReadPtr_hook = hook_thumb(so_mod.text_base + 0x74714 + 1, (uintptr_t)CGsEncryptFile_ReadPtr_patched);
    l_info("[Patch] Hooked CGsEncryptFile::ReadPtr to prevent Data Abort on Load Game when the save buffer is NULL.");

    /**
     * @brief 7. Hook CUIMenuStatus::PointerRelease at offset 0xe8608.
     * @note See `docs/source/patch.md:573` for detailed design rationale.
     */
    CUIMenuStatus_PR_hook = hook_thumb(so_mod.text_base + 0xe8608 + 1,
                                       (uintptr_t)CUIMenuStatus_PR_patched);
    l_info("[Patch] Hooked CUIMenuStatus::PointerRelease to prevent Data Abort on NULL sub-objects.");

    /**
     * @brief 8. Hook CUISubMenuReinForced::PointerRelease at offset 0xfcb78.
     * @note See `docs/source/patch.md:581` for detailed design rationale.
     */
    CUISubMenuReinForced_PR_hook = hook_thumb(so_mod.text_base + 0xfcb78 + 1,
                                              (uintptr_t)CUISubMenuReinForced_PR_patched);
    l_info("[Patch] Hooked CUISubMenuReinForced::PointerRelease to prevent Data Abort on NULL sub-objects.");

    /**
     * @brief 9. Hook CCharObject::ClearMsgState at offset 0x9746c.
     * @note See `docs/source/patch.md:588` for detailed design rationale.
     */
    CCharObject_ClearMsgState_hook = hook_thumb(so_mod.text_base + 0x9746c + 1,
                                                (uintptr_t)CCharObject_ClearMsgState_patched);
    l_info("[Patch] Hooked CCharObject::ClearMsgState to prevent Data Abort on Load Game (NULL message sub-objects).");

    /**
     * @brief 10. Hook CSaveMgr::Save at offset 0xab1e4.
     * @note See `docs/source/patch.md:597` for detailed design rationale.
     */
    CTotalObjMgr_GetRealPlayer_real = (fn_CTotalObjMgr_GetRealPlayer)(so_mod.text_base + 0xd26f0 + 1);
    CSaveMgr_Save_hook = hook_thumb(so_mod.text_base + 0xab1e4 + 1,
                                    (uintptr_t)CSaveMgr_Save_patched);
    l_info("[Patch] Hooked CSaveMgr::Save to prevent Data Abort on autosave when the player object isn't attached yet.");

#ifdef INSTRUMENT_BLIT_CALLS
    /**
     * @brief 11. Hook PutCompressImg at offset 0x140050 (diagnostic only, see the comment block above this function).
     * @note See `docs/source/patch.md:607` for detailed design rationale.
     */
    PutCompressImg_hook = hook_thumb(so_mod.text_base + 0x140050 + 1,
                                     (uintptr_t)PutCompressImg_counted);
    l_info("[Patch] Hooked PutCompressImg (diagnostic call-count probe, Bug 16).");
#endif

#ifdef OPT_COPY_BLIT_REWRITE
    // 12. Replace DrawOP_COPY_Compress_16 at offset 0x142588 (perf fix, see
    // the comment block above this function): the original machine code is
    // never called again after this -- no restore-call-repatch dance.
    DrawOP_COPY_Compress_16_hook = hook_thumb(so_mod.text_base + 0x142588 + 1,
                                              (uintptr_t)DrawOP_COPY_Compress_16_rewrite);
    l_info("[Patch] Replaced DrawOP_COPY_Compress_16 with a faster equivalent (Bug 16, needs visual verification).");
#endif

#ifdef OPT_MORE_BLIT_REWRITE
    // 13. Replace DrawOP_DARKEN_Compress_16 at offset 0x13d888 (7th perf
    // pass, see the comment block above this function): full replacement,
    // same as DrawOP_COPY_Compress_16 -- no restore-call-repatch dance.
    DrawOP_DARKEN_Compress_16_hook = hook_thumb(so_mod.text_base + 0x13d888 + 1,
                                                (uintptr_t)DrawOP_DARKEN_Compress_16_rewrite);
    l_info("[Patch] Replaced DrawOP_DARKEN_Compress_16 with a faster equivalent (Bug 16, needs visual verification).");

    // 14. Replace DrawOP_ADD_Compress_16 at offset 0x13cf0c (7th perf pass):
    // full replacement, same as above.
    DrawOP_ADD_Compress_16_hook = hook_thumb(so_mod.text_base + 0x13cf0c + 1,
                                             (uintptr_t)DrawOP_ADD_Compress_16_rewrite);
    l_info("[Patch] Replaced DrawOP_ADD_Compress_16 with a faster equivalent (Bug 16, needs visual verification).");

    // 15. Guard-hook DrawOP_BLEND16_Compress_16 at offset 0x13d388 (7th
    // perf pass): PARTIAL fix, only fast-paths alpha==8 and falls back to
    // the untouched original for every other alpha (see the comment block
    // above this function for why).
    DrawOP_BLEND16_Compress_16_hook = hook_thumb(so_mod.text_base + 0x13d388 + 1,
                                                 (uintptr_t)DrawOP_BLEND16_Compress_16_patched);
    l_info("[Patch] Hooked DrawOP_BLEND16_Compress_16 to fast-path the alpha==8 case (Bug 16, needs visual verification).");
#endif
}
