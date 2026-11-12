import json
import subprocess
import os
import hashlib
import shutil
from pathlib import Path

Import("env")


def calculate_hashes_and_size(file_path):
    md5_hash = hashlib.md5()
    sha256_hash = hashlib.sha256()
    file_size = 0

    try:
        with open(file_path, "rb") as file:
            chunk_size = 4096
            while True:
                chunk = file.read(chunk_size)
                if not chunk:
                    break
                md5_hash.update(chunk)
                sha256_hash.update(chunk)
                file_size += len(chunk)
        return md5_hash.hexdigest(), sha256_hash.hexdigest(), file_size
    except FileNotFoundError:
        print(f"File not found: {file_path}")
        return None, None, None


def post_build_action(source, target, env):
    version_value = "0.1.0"
    version_file = Path("version.json")
    if version_file.exists():
        try:
            with open(version_file, "r") as vf:
                vdata = json.load(vf)
                version_value = vdata.get("version", "0.1.0")
        except Exception as e:
            print(f"[POST-BUILD] Warning reading version.json: {e}")

    dist_dir = Path("dist")
    dist_dir.mkdir(exist_ok=True)

    build_dir = Path(".pio/build/cyd")
    fw_bin_source = build_dir / "firmware.bin"

    # Copy firmware binaries
    shutil.copy(str(fw_bin_source), "dist/firmware.bin")
    shutil.copy(str(fw_bin_source), "dist/cyd-ha-dashboard.bin")
    shutil.copy(str(fw_bin_source), f"dist/cyd-ha-dashboard.{version_value}.bin")
    shutil.copy(str(fw_bin_source), "firmware.bin")

    # Generate merged factory binary (bootloader + partitions + firmware at 0x0)
    factory_bin = "dist/cyd-ha-dashboard.factory.bin"
    platform = env.PioPlatform()
    esptool_pkg = platform.get_package_dir("tool-esptoolpy")
    esptool_path = os.path.join(esptool_pkg, "esptool.py") if esptool_pkg else "esptool.py"

    esptool_cmd = [
        env.subst("$PYTHONEXE"),
        esptool_path,
        '--chip', 'ESP32',
        'merge_bin',
        '-o', factory_bin,
        '--flash_mode', 'dio',
        '--flash_size', '4MB',
        '0x1000', str(build_dir / 'bootloader.bin'),
        '0x8000', str(build_dir / 'partitions.bin'),
        '0x10000', str(build_dir / 'firmware.bin')
    ]
    subprocess.run(esptool_cmd, check=True)

    shutil.copy(factory_bin, "dist/firmware.factory.bin")
    shutil.copy(factory_bin, f"dist/cyd-ha-dashboard.factory.{version_value}.bin")
    shutil.copy(factory_bin, "firmware.factory.bin")

    # Generate ota.json
    md5, sha256, size = calculate_hashes_and_size("dist/cyd-ha-dashboard.bin")
    ota_data = {
        "version": version_value,
        "firmware_url": f"https://github.com/demonkazuya/CYD-HA-Dashboard/releases/download/v{version_value}/cyd-ha-dashboard.bin",
        "url": f"https://github.com/demonkazuya/CYD-HA-Dashboard/releases/download/v{version_value}/cyd-ha-dashboard.bin",
        "sha256": sha256,
        "size_bytes": size,
        "md5": md5,
        "size": size
    }

    ota_json_path = dist_dir / "ota.json"
    with open(ota_json_path, "w") as of:
        json.dump(ota_data, of, indent=2)

    # Also keep ota.json at project root for GitHub Pages / raw link
    with open("ota.json", "w") as of:
        json.dump(ota_data, of, indent=2)

    print(f"[POST-BUILD] Generated v{version_value} release assets:")
    print(f"             - dist/cyd-ha-dashboard.bin (MD5: {md5})")
    print(f"             - dist/cyd-ha-dashboard.factory.bin (Offset 0x0)")
    print(f"             - dist/ota.json & ota.json")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", post_build_action)
