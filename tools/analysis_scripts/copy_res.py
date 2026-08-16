import os
import shutil

SRC_RES = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/Advena-1.0.1/res"
DST_DATA = "/Volumes/Seagate/PSVITA Develop/Advena-Vita/ux0_data/advena"
DST_RES = os.path.join(DST_DATA, "res")

# Korean-specific exclusions
KOR_EXCLUDE_PATTERNS = [
    "_k.png", "_k_s.png", "_kr.png", "_kor.png",
    "menu-ko", "keyboard_bg_kr.png", "ui_custom_bg_kr.png",
    "ui_quickslot_bg_kor.png", "back_k.png", "freevena_k.png", "freevena_k_s.png"
]

def should_exclude(rel_path):
    rel_lower = rel_path.lower()
    for pattern in KOR_EXCLUDE_PATTERNS:
        if pattern in rel_lower:
            return True
    return False

copied_count = 0
for root, dirs, files in os.walk(SRC_RES):
    for f in files:
        src_file = os.path.join(root, f)
        rel_path = os.path.relpath(src_file, SRC_RES)
        if should_exclude(rel_path):
            print(f"Skipping Korean file: {rel_path}")
            continue
        
        dst_file = os.path.join(DST_RES, rel_path)
        os.makedirs(os.path.dirname(dst_file), exist_ok=True)
        shutil.copy2(src_file, dst_file)
        copied_count += 1

print(f"Successfully copied {copied_count} resource files to {DST_RES}")
