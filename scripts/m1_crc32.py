#!/usr/bin/env python3
"""
Monstatek M1 STM32H573 Hardware CRC-32 Firmware Image Signer
Exact bit-for-bit equivalent of:
  srec_cat <in.bin> -binary -crop 0x0 0x000FFC14 -fill 0xFF 0x0 0x000FFC14 \
           -STM32_Little_Endian 0x000FFC14 -o <out_wCRC.bin> -binary

The STM32 hardware CRC unit uses:
  - Polynomial: 0x04C11DB7 (normal / MSB-first representation)
  - Initial value: 0xFFFFFFFF
  - Word width: 32-bit little-endian words (no bit reflection, no final XOR)
  - Fixed M1 image boundary: 0x000FFC14 (1,047,572 bytes); output is 1,047,576 bytes.
"""

import argparse
import pathlib
import struct
import sys

M1_CRC_OFFSET = 0x000FFC14
STM32_POLY = 0x04C11DB7


def stm32_hw_crc32(data: bytes) -> int:
    if len(data) % 4 != 0:
        raise ValueError(f"Data length ({len(data)}) must be a multiple of 4 bytes")
    crc = 0xFFFFFFFF
    # Unpack as 32-bit little-endian unsigned integers, matching srec_cat -STM32_Little_Endian
    for (word,) in struct.iter_unpack("<I", data):
        crc ^= word
        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) & 0xFFFFFFFF) ^ STM32_POLY
            else:
                crc = (crc << 1) & 0xFFFFFFFF
    return crc


def sign_m1_firmware(input_bin: pathlib.Path, output_bin: pathlib.Path, crop_offset: int = M1_CRC_OFFSET) -> int:
    raw = input_bin.read_bytes()
    if len(raw) > crop_offset:
        image = raw[:crop_offset]
    else:
        image = raw + (b"\xFF" * (crop_offset - len(raw)))

    crc_val = stm32_hw_crc32(image)
    signed_image = image + struct.pack("<I", crc_val)
    output_bin.parent.mkdir(parents=True, exist_ok=True)
    output_bin.write_bytes(signed_image)
    return crc_val


def main():
    parser = argparse.ArgumentParser(description="Append STM32 hardware CRC-32 at 0x000FFC14 for Monstatek M1.")
    parser.add_argument("input", type=pathlib.Path, help="Input unsigned .bin file")
    parser.add_argument("-o", "--output", type=pathlib.Path, required=True, help="Output *_wCRC.bin file")
    parser.add_argument("--offset", type=lambda x: int(x, 0), default=M1_CRC_OFFSET, help="CRC offset (default: 0x000FFC14)")
    args = parser.parse_args()

    crc = sign_m1_firmware(args.input, args.output, args.offset)
    print(f"[m1_crc32] Signed {args.output.name}: size={args.output.stat().st_size} bytes (0x{args.output.stat().st_size:X}), STM32_CRC32=0x{crc:08X}")


if __name__ == "__main__":
    main()
