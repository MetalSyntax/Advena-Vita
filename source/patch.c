/*
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

/**
 * @file  patch.c
 * @brief Patching some of the .so internal functions or bridging them to native
 *        for better compatibility.
 */

#include <kubridge.h>
#include <so_util/so_util.h>
#include <string.h>
#include "utils/logger.h"

extern so_module so_mod;


// --- Teamstrike [T] button position fix -------------------------------------
//
// GVUIPlayerController::ShowBtn (patched below) only ever toggles Show()/
// Hide() on the 5 command buttons -- confirmed by reading its full pseudo-C
// (decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c:30159-30192): it never
// touches any X/Y coordinate. The actual placement of those buttons happens
// in a *different* function, GVUIPlayerController::InitialPlayerPadSet
// (.so offset 0x896cc, called once from the GVUIPlayerController
// constructor -- decompiled/.../out_ghidra.c:30518,30739,30755).
//
// Verified via live disassembly of ux0_data/advena/libgameDSO.so
// (arm-vita-eabi-objdump -d -M force-thumb):
//   .so+0x896f8-0x8970e: this[0x1ec] = ScreenWidth - 120        (anchor_x)
//                        this[0x1f0] = HeightComponent - 80     (anchor_y)
//     where ScreenWidth/HeightComponent come from the
//     CGsSingleton<CGsGraphics>::ms_pSingleton+0x48/+0x4c fields, which
//     CGsGraphics::InitialScreen (.so+0x76234) fills in from the engine's
//     OWN internal software-rasterizer framebuffer object
//     (GcxGetMainScreenBuffer()/MC_grpInitContext) -- NOT from any of the
//     NativeInitWithBufferSize/NativeInitDeviceInfo/NativeResize JNI calls
//     this port makes in main.c. So neither of those calls can steer it.
//   .so+0x8995e-0x8997a: Teamstrike's button object (this[0x1b0]) gets
//     SetPosition(anchor_x - 360, anchor_y - 140), i.e. (ScreenWidth-480,
//     HeightComponent-220).
//   .so+0x899d2-0x899ec: the neighboring Tag/Switch-Character "swap" button
//     gets SetPosition(anchor_x + 77, anchor_y - 33), i.e. (ScreenWidth-43,
//     HeightComponent-113) -- always within ~43px of the true right edge,
//     REGARDLESS of what ScreenWidth actually is. That's why it always
//     "looks right" while Teamstrike does not: Teamstrike's offset (-360)
//     was tuned by the original devs for a small assumed ScreenWidth (so it
//     lands just off the LEFT edge, next to the D-pad, matching the
//     reference layout in advena-13.webp); on this port, the engine's
//     internal ScreenWidth ends up much larger, so "ScreenWidth-480" lands
//     near mid/right-screen instead -- right next to the Tag/swap button,
//     matching exactly what's seen in 2026-08-15-233550.jpg.
//
// Patching the JNI resolution feed (main.c) is NOT a safe fix here: those
// calls don't reach CGsGraphics's internal framebuffer at all (traced: no
// data path from initPlatformSize()/getDeviceInfo() into
// CGsGraphics::InitialScreen), and forcing NativeResize's argument away
// from the real physical size would additionally corrupt the sprite-scale
// ratio math in glDrawFrame (.so+0x11fc64, glResize), which legitimately
// needs the real display size.
//
// Fix: hook InitialPlayerPadSet, let the ORIGINAL implementation run to
// completion (untouched behavior for the D-pad, analog stick, Attack/Jump/
// Tag buttons), then re-derive the engine's own anchor_x/anchor_y from the
// fields it just wrote (this+0x1ec/this+0x1f0) and re-issue SetPosition for
// the Teamstrike object (this+0x1b0) using a screen-relative percentage
// instead of the original hardcoded pixel deltas. This is resolution-
// independent: it lands in the right spot no matter what ScreenWidth/
// HeightComponent the engine's internal framebuffer actually reports.
typedef void (*fn_GVUIObject_SetPosition)(void *obj, int x, int y);
static fn_GVUIObject_SetPosition GVUIObject_SetPosition_real = NULL;
static so_hook InitialPlayerPadSet_hook;

#define TEAMSTRIKE_POS_X_PCT 0.06f  // ~6% in from the left edge (next to the D-pad)
#define TEAMSTRIKE_POS_Y_PCT 0.40f  // ~40% down (mid-height, above the D-pad)

static void InitialPlayerPadSet_patched(void *this_ptr) {
    // Run the original layout logic first, unmodified. Can't use the
    // SO_CONTINUE() macro here: it declares "type r = ...", which isn't
    // valid for a void-returning function, so this replicates it by hand.
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

// --- Load Game crash fix (Data Abort in CGsEncryptFile::ReadPtr) -----------
//
// Reproduced on-console: choosing "Load Game" reliably Data Aborts. Two
// separate captures (logs/Advena-psp2core-1786909652-...psp2dmp and
// -1786913294-...) show the IDENTICAL fault: PC lands inside a
// SceLibKernel/libc primitive reached via a `blx memcpy@plt` from inside
// the .so, with R1 (memcpy's src argument) == 0x0, and R5 == 0x277.
//
// Confirmed via live disassembly of ux0_data/advena/libgameDSO.so
// (arm-vita-eabi-objdump -d -M force-thumb):
//   .so+0x74714 CGsEncryptFile::ReadPtr(void*, unsigned int):
//     74718: ldr r3, [r4, #80]   ; r3 = this->readCursor (this+0x50)
//     7471c: ldr r1, [r4, #84]   ; r1 = this->buffer     (this+0x54)
//     74720: adds r1, r1, r3     ; r1 = buffer + cursor  (memcpy src)
//     74722: blx  memcpy@plt     ; memcpy(dst=r0, src=r1, len=r2) <-- FAULTS
//                                  here whenever this->buffer (this+0x54)
//                                  is NULL.
//
// Root cause traced back through the caller, CSaveMgr::LoadPlayData()
// (.so+0xab4b0; pseudo-C in
// decompiled/libgameDSO_armeabi/ghidra/out_ghidra.c around line 75217): it
// calls CGsEncryptFile::LoadBegin(this, path, true) and DISCARDS its return
// value, then unconditionally issues 4 ReadPtr() calls (0x277, 0x420, 0x178,
// 0xa9c bytes -- note the very first length, 0x277, matches R5 in both
// crash dumps exactly). CGsEncryptFile::LoadBegin (.so+0x74848) only
// populates this->buffer (this+0x54) INSIDE its "if (GsFSFileSize(path) !=
// 0)" branch -- if the save file is missing/empty/truncated at that instant
// (GsFSFileSize returns 0), LoadBegin bails out early and this->buffer is
// left NULL (from the caller's own memset of the CGsEncryptFile object).
// LoadPlayData never checks for that failure before running the ReadPtr
// chain, so it unconditionally memcpy()s from the NULL buffer.
//
// This is reachable in real gameplay on this port: logs/advena_003.log
// (lines 314-340) and logs/advena_004.log (lines 228-246) show CSaveMgr
// opening the save slot in truncate mode (fopen(".../s0.dat", "w+")) and
// closing it, and shortly after, the player enters the Load menu
// (OnUIStatusChange 19/14) and CSaveMgr::LoadPlayData's own file-exists
// check on that same slot succeeds (stat(...s0.dat...): 0) while its
// *size* (as seen by GsFSFileSize/LoadBegin) can still read back as 0 for
// that slot -- landing exactly in this path.
//
// Fix: hook ReadPtr itself. If this->buffer is NULL (LoadBegin bailed out),
// zero-fill the destination and advance the read cursor exactly like the
// original would have, instead of dereferencing the NULL buffer. Otherwise
// run the untouched original implementation. This one guard covers all 4
// ReadPtr call sites in LoadPlayData (and any other caller) without having
// to restructure CSaveMgr::LoadPlayData's own control flow.
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

    // Buffer is valid: run the original implementation untouched. Same
    // manual restore/call/repatch dance as InitialPlayerPadSet_patched
    // above (can't use SO_CONTINUE here for the same reason: it declares
    // "type r = ...", which isn't valid for a void-returning function).
    kuKernelCpuUnrestrictedMemcpy((void *)ReadPtr_hook.addr, ReadPtr_hook.orig_instr,
                                  sizeof(ReadPtr_hook.orig_instr));
    kuKernelFlushCaches((void *)ReadPtr_hook.addr, sizeof(ReadPtr_hook.orig_instr));
    ((fn_CGsEncryptFile_ReadPtr)ReadPtr_hook.thumb_addr)(this_ptr, dst, len);
    kuKernelCpuUnrestrictedMemcpy((void *)ReadPtr_hook.addr, ReadPtr_hook.patch_instr,
                                  sizeof(ReadPtr_hook.patch_instr));
    kuKernelFlushCaches((void *)ReadPtr_hook.addr, sizeof(ReadPtr_hook.patch_instr));
}

// ---------------------------------------------------------------------------
// Crash fix: CUIMenuStatus::PointerRelease (SO offset 0xe8608)
// ---------------------------------------------------------------------------
// Decompilation (out_ghidra.c:173961-173970) shows the very first 10 lines
// dereference this+0xe0 through this+0xf0 WITHOUT any NULL checks:
//
//   *(undefined *)(*(int *)(this + 0xe0) + 0x12) = 0;
//   *(undefined *)(*(int *)(this + 0xe0) + 0x13) = 0;
//   *(undefined *)(*(int *)(this + 0xe4) + 0x12) = 0;
//   *(undefined *)(*(int *)(this + 0xe4) + 0x13) = 0;
//   ...
//   *(undefined *)(*(int *)(this + 0xf0) + 0x12) = 0;
//   *(undefined *)(*(int *)(this + 0xf0) + 0x13) = 0;
//
// If the CUIMenuStatus object exists (vtable is valid, PointerRelease is
// called) but its sub-objects at 0xe0..0xf0 were never allocated (e.g.
// during a load-game UI transition where the menu is partially
// constructed), the first dereference of NULL causes the Data Abort.
// The 5 fields are pointers to UI display elements / touch areas.
//
// Fix: check all 5 pointers at entry. If ANY is NULL, skip the function
// entirely (the menu is not ready to handle the touch release).
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

// ---------------------------------------------------------------------------
// Crash fix: CUISubMenuReinForced::PointerRelease (SO offset 0xfcb78)
// ---------------------------------------------------------------------------
// Decompilation (out_ghidra.c:190443-190452) shows two branches, both with
// raw pointer dereferences and NO NULL guards:
//
//   if (this[0xec] == 0) {
//       *(undefined *)(*(int *)(this + 0xd8) + 0x12) = 0;  // line 190444
//       ...
//   } else {
//       piVar1 = (int *)(**(code **)(**(int **)(*(int *)(this + 0x44) + 0x14)
//                          + 0x4c))(...);                     // line 190452
//       ...
//   }
//
// When the Reinforced sub-menu is partially constructed during a load
// transition, this+0xd8 (path A) or this+0x44 (path B) can be NULL.
// Fix: validate the pointer used by the active branch before calling the
// original; skip if NULL.
//
// NOTE: "this[0xec]" in the pseudo-C above is Ghidra's byte-array shorthand
// (its placeholder type for this unresolved class is 1 byte wide), NOT a
// 4-byte int -- confirmed against the real disassembly at .so+0xfcb86:
//   fcb86: movs r3, #0xec
//   fcb88: ldrb  r3, [r0, r3]   ; single-byte load, not ldr (word)
// Reading it as `int` here would pull in 3 unrelated bytes at +0xed..+0xef
// and can make this guard pick the WRONG branch (letting the real crash
// through), so this must be a single-byte read to match.
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

// ---------------------------------------------------------------------------
// Crash fix: CCharObject::ClearMsgState (SO offset 0x9746c)
// ---------------------------------------------------------------------------
// Root cause traced directly from a live .psp2dmp (Data Abort reproduced
// while loading a save game): the dump's captured memory pages around PC/LR
// show the real fault site is a `bl` from CMapMgr::InitHero() (.so+0xca0a4,
// decompiled at out_ghidra.c:135905-135965) into CCharObject::ClearMsgState()
// (.so+0x9746c, decompiled at out_ghidra.c:47947-47965) -- confirmed by
// locating the exact byte pattern of ClearMsgState's prologue in the static
// .so and matching the call-site offset against the dump's own LR.
//
// InitHero() re-derives the hero's on-screen position right after a map/save
// loads and calls ClearMsgState() on the hero object fetched from
// CGsSingleton<CTotalObjMgr>::ms_pSingleton+0x5c. ClearMsgState dereferences
// three pointer fields on `this` with NO NULL checks:
//   this+0x5c  (a message-box sub-object, invoked virtually twice)
//   this+0x64  (a message-icon sub-object -- this is what actually faults:
//               .so+0x9746c: ldr r3,[r0,#0x64]; .so+0x97474: strb r2,[r3,#18])
//   this+0x74  (a third sub-object, this+0x74 -> [+5] = 0)
// During the Load Game -> InitHero() transition these UI sub-objects on the
// hero's CCharObject haven't been (re)constructed yet, so this+0x64 is still
// NULL and the unconditional strb at .so+0x97474 Data Aborts.
//
// Fix: same strategy as the CUIMenuStatus/CUISubMenuReinForced guards above
// -- if any of the 3 required sub-objects is NULL, there's no message state
// to clear yet, so skip the call entirely instead of crashing.
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

// ---------------------------------------------------------------------------
// Crash fix: CSaveMgr::Save (SO offset 0xab1e4)
// ---------------------------------------------------------------------------
// Reproduced on-console (logs/advena_029.log + advena-psp2core-1787200198-...
// .psp2dmp): the crash-dump analysis tool auto-detected the wrong .so base
// (0x9806f000 instead of the real 0x98000000, the same recurring pitfall
// documented for Bug 14), which misattributed this crash to unrelated shop/
// popup functions. Recomputing PC/LR against the real base and the .so's own
// dynamic symbol table (objdump -T) puts both squarely inside
// CSaveMgr::Save(): PC = .so+0xab238 (Save()+0x54), LR = .so+0xab223
// (Save()+0x3f). A stale return address further up the captured stack
// resolves to CMapMgr::ChangeField()+0x167, confirming the trigger: entering
// a new map/field fires an autosave via CSaveMgr::Save().
//
// Decompiled at out_ghidra.c:75085-75136, CSaveMgr::Save() calls all three
// of CTotalObjMgr::GetRealPlayer()/GetRealFellow1()/GetRealFellow2() and
// immediately writes through their results. GetRealFellow1() and
// GetRealFellow2() are BOTH properly guarded ("if (iVar4 == 0) { ... } else
// { ...dereference... }"), but GetRealPlayer()'s result is dereferenced
// completely unconditionally right at the top of the function
// ("in_r0[...] = *(CSaveMgr *)(iVar3 + 0x7c); ... *(undefined2 *)(iVar3 +
// 0xb8)") -- no NULL check at all. Right after a Load Game / map transition,
// the "real player" object isn't necessarily re-attached in CTotalObjMgr yet
// when ChangeField's autosave fires, so GetRealPlayer() can still be NULL at
// that exact instant, and the unconditional dereference Data Aborts.
//
// Fix: call GetRealPlayer() ourselves before running the original. If it's
// NULL, skip the autosave entirely for this trigger (no worse than any
// other event that doesn't happen to call Save() at that moment) instead of
// crashing the whole process -- which, combined with the truncating "w+"
// open CSaveMgr::SavePlayData/tagGameData::Save perform on s0.dat/g.dat,
// is also how a save could end up destroyed with no crash-time warning at
// all: the truncate lands, then the process dies before the rewrite.
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

void so_patch(void) {
    static int patches_applied = 0;
    if (patches_applied) {
        l_warn("[Patch] so_patch called more than once; ignoring duplicate invocation.");
        return;
    }
    patches_applied = 1;

    // 1. Patch GVUIPlayerController::ShowBtn(int mask) at offset 0x88f82 and 0x88fba:
    // a) Force mask = 0x1F (0x261f) so all 5 buttons are requested.
    // b) Replace "beq 88fda" at 0x88fba with NOP (0xbf00) to PREVENT calling GVUIObject::Hide().
    // This ensures all 5 UI buttons (Teamstrike [T], Tag, Attack, Jump Left, Jump Right) are ALWAYS visible on screen!
    uint16_t show_all_btns_patch = 0x261f;
    uint16_t nop_hide_patch = 0xbf00;
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x88f82), &show_all_btns_patch, sizeof(show_all_btns_patch));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x88fba), &nop_hide_patch, sizeof(nop_hide_patch));
    l_info("[Patch] Patched GVUIPlayerController::ShowBtn to force all UI buttons (including [T] Teamstrike) visible.");

    // 2. Patch CSaveMgr::IsLeaglSaveData(int) at offset 0xab424:
    // Replace function entry with "movs r0, #1; bx lr;" (0x2001, 0x4770 -> 0x47702001)
    // to bypass the header checksum check that falsely flags saved games as corrupted.
    uint32_t always_legal_save_patch = 0x47702001;
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0xab424), &always_legal_save_patch, sizeof(always_legal_save_patch));
    l_info("[Patch] Patched CSaveMgr::IsLeaglSaveData to always return true.");

    // 3. Patch CGsEncryptFile::LoadBegin(char const*, bool) at offset 0x7492e and 0x74952:
    // a) At 0x7492e: replace "bne 74872" with NOP (0xbf00) to ignore phone/device ID mismatch.
    // b) At 0x74952: replace "beq 74964" with unconditional branch "b 74964" (0xe007)
    //    to bypass savefile checksum mismatch and prevent deleting the save file!
    uint16_t nop_phone_check = 0xbf00;
    uint16_t branch_always_valid = 0xe007;
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x7492e), &nop_phone_check, sizeof(nop_phone_check));
    kuKernelCpuUnrestrictedMemcpy((void *)(so_mod.text_base + 0x74952), &branch_always_valid, sizeof(branch_always_valid));
    l_info("[Patch] Patched CGsEncryptFile::LoadBegin to bypass save encryption checksum and phone ID checks.");

    // 4. Hook GVUIPlayerController::InitialPlayerPadSet at offset 0x896cc:
    // reposition the Teamstrike [T] button after the original layout logic
    // runs. See the detailed rationale in the comment block above this
    // function for why this is a hook rather than a raw byte patch.
    // +1 sets the Thumb bit: this is called through a plain C function
    // pointer (BLX Rn), which -- like the LDR-PC trampoline hook_thumb()
    // installs -- is an interworking branch that switches ARM/Thumb mode
    // based on bit 0 of the target address. Without it, the call would
    // enter this Thumb-compiled function in ARM mode and decode its own
    // instruction bytes as ARM, corrupting execution immediately.
    GVUIObject_SetPosition_real = (fn_GVUIObject_SetPosition)(so_mod.text_base + 0x88388 + 1);
    InitialPlayerPadSet_hook = hook_thumb(so_mod.text_base + 0x896cc + 1, (uintptr_t)InitialPlayerPadSet_patched);
    l_info("[Patch] Hooked GVUIPlayerController::InitialPlayerPadSet to fix the Teamstrike [T] button position.");


    // 6. Hook CGsEncryptFile::ReadPtr at offset 0x74714:
    // prevent a Data Abort (NULL-pointer memcpy) when Load Game runs against
    // a save slot whose CGsEncryptFile::LoadBegin failed to populate a
    // decrypt buffer (empty/missing/truncated save file at that instant).
    // See the detailed rationale in the comment block above this function
    // for the crash-dump/log evidence and root-cause trace.
    ReadPtr_hook = hook_thumb(so_mod.text_base + 0x74714 + 1, (uintptr_t)CGsEncryptFile_ReadPtr_patched);
    l_info("[Patch] Hooked CGsEncryptFile::ReadPtr to prevent Data Abort on Load Game when the save buffer is NULL.");

    // 7. Hook CUIMenuStatus::PointerRelease at offset 0xe8608:
    // prevent Data Abort when sub-objects (this+0xe0..0xf0) are NULL during
    // a load-game UI transition.  See the detailed rationale in the comment
    // block above the patched function.
    CUIMenuStatus_PR_hook = hook_thumb(so_mod.text_base + 0xe8608 + 1,
                                       (uintptr_t)CUIMenuStatus_PR_patched);
    l_info("[Patch] Hooked CUIMenuStatus::PointerRelease to prevent Data Abort on NULL sub-objects.");

    // 8. Hook CUISubMenuReinForced::PointerRelease at offset 0xfcb78:
    // prevent Data Abort when sub-objects (this+0xd8 or this+0x44) are NULL.
    // See the detailed rationale in the comment block above the patched function.
    CUISubMenuReinForced_PR_hook = hook_thumb(so_mod.text_base + 0xfcb78 + 1,
                                              (uintptr_t)CUISubMenuReinForced_PR_patched);
    l_info("[Patch] Hooked CUISubMenuReinForced::PointerRelease to prevent Data Abort on NULL sub-objects.");

    // 9. Hook CCharObject::ClearMsgState at offset 0x9746c:
    // prevent the Data Abort reproduced on-console when loading a save game
    // (CMapMgr::InitHero calls this on the hero object before its message
    // sub-objects at this+0x5c/0x64/0x74 have been (re)constructed). See the
    // detailed rationale in the comment block above the patched function.
    CCharObject_ClearMsgState_hook = hook_thumb(so_mod.text_base + 0x9746c + 1,
                                                (uintptr_t)CCharObject_ClearMsgState_patched);
    l_info("[Patch] Hooked CCharObject::ClearMsgState to prevent Data Abort on Load Game (NULL message sub-objects).");

    // 10. Hook CSaveMgr::Save at offset 0xab1e4:
    // prevent Data Abort when CTotalObjMgr::GetRealPlayer() is NULL at the
    // moment a map transition (CMapMgr::ChangeField) fires an autosave. See
    // the detailed rationale in the comment block above the patched function.
    CTotalObjMgr_GetRealPlayer_real = (fn_CTotalObjMgr_GetRealPlayer)(so_mod.text_base + 0xd26f0 + 1);
    CSaveMgr_Save_hook = hook_thumb(so_mod.text_base + 0xab1e4 + 1,
                                    (uintptr_t)CSaveMgr_Save_patched);
    l_info("[Patch] Hooked CSaveMgr::Save to prevent Data Abort on autosave when the player object isn't attached yet.");
}
