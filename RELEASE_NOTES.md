# Advena: Legend of Emeris — PS Vita Port v1.0.0

**Release Tag:** `v1.0.0`

---

## ⚔️ Advena: Legend of Emeris — PlayStation Vita Port (v1.0.0)

First stable release of the native **Advena: Legend of Emeris (Gamevil)** port for the **PlayStation Vita**, with a fully working save/load cycle and initial CPU scheduling optimizations.

---

## 🚀 What's New in v1.0.0

- **💾 Save/Load Fully Working**: Confirmed end-to-end on physical hardware — Save, exit, and Load Game no longer crashes and correctly restores progress. This closes out a long chain of root causes fixed over prior builds:
  - Save file paths now redirect correctly to `ux0:data/advena/saves/`.
  - Fixed a `stat64_bionic` struct layout mismatch (VitaSDK newlib vs. Android Bionic ABI) that made `GsFSFileSize()` always read save files as 0 bytes.
  - Fixed a Data Abort in `CGsEncryptFile::ReadPtr` when loading an empty/truncated save slot.
  - Fixed three Data Aborts during the post-load UI/hero reconstruction (`CUIMenuStatus::PointerRelease`, `CUISubMenuReinForced::PointerRelease`, `CCharObject::ClearMsgState`) caused by the engine touching UI sub-objects before they were rebuilt.
- **🧵 CPU Core Affinity**: Main render/logic thread pinned to **CPU Core 0**; audio mixer pinned to **CPU Core 1**, so the scheduler never migrates either thread mid-frame. Same technique validated on real hardware in the sibling Zenonia 4 Vita port (same Gamevil Nexus2/GxPZx engine family).
- **⚡ CPU/GPU/Bus Overclock**: ARM @ 444 MHz, Bus @ 222 MHz, GPU @ 222 MHz, GPU Xbar @ 166 MHz — Vita's safe maximum, already validated across this engine family.
- **🎮 Full Physical Control Mapping**: D-Pad/analog movement, all combat/skill buttons, Teamstrike, character tag-switch, and menu navigation — no synthesized touch conflicts with physical input.
- **🖥️ Correct 480x320 Logical Canvas**: Matches Advena's real Android design resolution (`AdvenaLauncher.java`), scaled to the Vita's full 960x544 screen with no cropped UI elements.

---

## ⚠️ Known Issues / Next Steps

- Framerate can drop to ~15 FPS or lower in some combat scenes and busy maps. The CPU affinity fix in this release is a first, low-risk pass; a deeper investigation (instrumenting `glDrawArrays`/`glBindTexture` call counts per frame in `source/dynlib.c`, the same methodology used to find Zenonia 4's real hot path) is the next step — see `PORTING_PLAN.md`, Bug 16, for the full analysis and what was ruled out.

---

## 🎮 Controls

| Button / Input | Action |
| :--- | :--- |
| **D-Pad / Left Stick** | Walk / Climb Ladder |
| **Cross (✕)** | Attack / Action / Talk to NPC / Confirm |
| **Circle (◯)** | Jump Right (Forward Jump in combat) |
| **Triangle (△)** | Jump Left (Backward Jump in combat) |
| **Square (□) / Right Stick Left** | Skill Slot 1 |
| **Right Stick Up / Down / Right** | Skill Slot 2 / 3 / 4 |
| **L1 Trigger** | Teamstrike [T] / Quest [!] |
| **R1 Trigger** | Tag / Switch Character |
| **Select** | Inventory / Bag / Cancel / Back |
| **Start** | Pause Menu |
| **Front Touchscreen** | Full native touch controls & menu navigation |

---

## 📦 Installation Instructions

1. Install `advena.vpk` on your PlayStation Vita via **VitaShell**.
2. Copy `libgameDSO.so`, `assets/`, and `res/` from the Android APK into `ux0:data/advena/`.
3. Launch **Advena** from LiveArea and enjoy the game!

---

## 📁 Downloads / Assets

- **`advena.vpk`**: Standard Release build (`-O3`, overclock + core affinity active, debug logging compiled out).
