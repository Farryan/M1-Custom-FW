#!/usr/bin/env python3
"""
Phase 3: Automated UberGuidoZ -> Monstatek M1 SD Card Asset Conversion Pipeline
Normalizes Flipper Zero .ir, .sub, .nfc, and .rfid files into Monstatek M1 FatFs R0.15
SD card directory hierarchy (INFRARED/db/, Sub_Ghz/, NFC/, lfrfid/, System/) and packages
M1_SD_Assets.zip.
"""

import argparse
import pathlib
import re
import shutil
import zipfile

PRESET_MAP = {
    "FuriHalSubGhzPresetOok270Async": "AM270",
    "FuriHalSubGhzPresetOok650Async": "AM650",
    "FuriHalSubGhzPreset2FSKDev238Async": "FM238",
    "FuriHalSubGhzPreset2FSKDev476Async": "FM476",
}


def sanitize_fatfs_filename(name: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9._-]", "_", name)
    return cleaned[:48]


def convert_sub_file(content: str) -> str:
    lines = []
    for raw_line in content.splitlines():
        line = raw_line.strip()
        if line.startswith("Preset:"):
            preset_val = line.split(":", 1)[1].strip()
            m1_tag = PRESET_MAP.get(preset_val, "AM650")
            lines.append(f"Preset: {preset_val}")
            lines.append(f"M1_Si4463_Mod: {m1_tag}")
        else:
            lines.append(line)
    return "\n".join(lines) + "\n"


def generate_curated_seed_assets(staging_root: pathlib.Path):
    """Populates curated reference assets if an external UberGuidoZ clone is not passed."""
    (staging_root / "INFRARED/db").mkdir(parents=True, exist_ok=True)
    (staging_root / "Sub_Ghz").mkdir(parents=True, exist_ok=True)
    (staging_root / "NFC").mkdir(parents=True, exist_ok=True)
    (staging_root / "lfrfid").mkdir(parents=True, exist_ok=True)
    (staging_root / "System").mkdir(parents=True, exist_ok=True)

    (staging_root / "INFRARED/db/Universal_TV.ir").write_text(
        "Filetype: IR signals file\nVersion: 1\n#\n"
        "name: Power\ntype: parsed\nprotocol: NECext\naddress: 04 FB 00 00\ncommand: 08 F7 00 00\n#\n"
        "name: Vol_Up\ntype: parsed\nprotocol: Samsung32\naddress: 07 07 00 00\ncommand: 07 F8 00 00\n",
        encoding="utf-8",
    )

    sub_raw = (
        "Filetype: Flipper SubGhz Key File\nVersion: 1\n"
        "Frequency: 433920000\nPreset: FuriHalSubGhzPresetOok650Async\n"
        "Protocol: Princeton\nBit: 24\nKey: 00 00 00 00 00 41 52 54\nTE: 350\n"
    )
    (staging_root / "Sub_Ghz/Lab_Test_Princeton_433.sub").write_text(
        convert_sub_file(sub_raw), encoding="utf-8"
    )

    (staging_root / "NFC/Mifare_Classic_1K_Diag.nfc").write_text(
        "Filetype: Flipper NFC device\nVersion: 4\nDevice type: Mifare Classic\n"
        "UID: 04 A1 B2 C3\nATQA: 00 04\nSAK: 08\nMifare Classic type: 1K\n",
        encoding="utf-8",
    )

    (staging_root / "lfrfid/EM4100_Demo_Badge.rfid").write_text(
        "Filetype: Flipper RFID key\nVersion: 1\nKey type: EM4100\nData: 01 02 03 04 05\n",
        encoding="utf-8",
    )


def build_sd_bundle(source_dir: pathlib.Path | None, output_zip: pathlib.Path) -> int:
    staging = output_zip.parent / "_sd_staging"
    if staging.exists():
        shutil.rmtree(staging)
    generate_curated_seed_assets(staging)

    if source_dir and source_dir.exists():
        ext_map = {
            ".ir": "INFRARED/db",
            ".sub": "Sub_Ghz",
            ".nfc": "NFC",
            ".rfid": "lfrfid",
        }
        for path in source_dir.rglob("*"):
            if path.is_file() and path.suffix.lower() in ext_map:
                dest_folder = staging / ext_map[path.suffix.lower()]
                dest_folder.mkdir(parents=True, exist_ok=True)
                safe_name = sanitize_fatfs_filename(path.name)
                text = path.read_text(encoding="utf-8", errors="ignore")
                if path.suffix.lower() == ".sub":
                    text = convert_sub_file(text)
                (dest_folder / safe_name).write_text(text, encoding="utf-8")

    output_zip.parent.mkdir(parents=True, exist_ok=True)
    count = 0
    with zipfile.ZipFile(output_zip, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for file_path in sorted(staging.rglob("*")):
            if file_path.is_file():
                zf.write(file_path, file_path.relative_to(staging))
                count += 1
    shutil.rmtree(staging)
    return count


def main():
    parser = argparse.ArgumentParser(description="Package Monstatek M1 SD card asset bundle.")
    parser.add_argument("--source", type=pathlib.Path, default=None, help="Optional path to UberGuidoZ/Flipper clone")
    parser.add_argument("-o", "--output", type=pathlib.Path, required=True, help="Output M1_SD_Assets.zip path")
    args = parser.parse_args()

    total = build_sd_bundle(args.source, args.output)
    print(f"[sync_uberguidoz] Packaged {total} M1-compatible SD files into {args.output}")


if __name__ == "__main__":
    main()
