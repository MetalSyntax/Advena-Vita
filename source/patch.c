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
}
