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
