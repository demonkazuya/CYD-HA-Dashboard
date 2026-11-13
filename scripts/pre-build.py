import shutil
from pathlib import Path

Import("env")

libdeps_dir = Path(".pio/libdeps/cyd")
if libdeps_dir.exists():
    tft_dest = libdeps_dir / "TFT_eSPI" / "User_Setup.h"
    if tft_dest.parent.exists():
        shutil.copy("include/User_Setup.h", tft_dest)
        print("[PRE-BUILD] Synced User_Setup.h to TFT_eSPI")
    lv_dest = libdeps_dir / "lvgl" / "lv_conf.h"
    if lv_dest.parent.exists():
        shutil.copy("include/lv_conf.h", lv_dest)
        print("[PRE-BUILD] Synced lv_conf.h to lvgl")
    lv_label_src = libdeps_dir / "lvgl" / "src" / "widgets" / "lv_label.c"
    if lv_label_src.exists():
        content = lv_label_src.read_text()
        if "#define LV_LABEL_SCROLL_DELAY       300" in content:
            content = content.replace("#define LV_LABEL_SCROLL_DELAY       300", "#define LV_LABEL_SCROLL_DELAY       3500")
            lv_label_src.write_text(content)
            print("[PRE-BUILD] Set LV_LABEL_SCROLL_DELAY to 3500ms in lv_label.c")
