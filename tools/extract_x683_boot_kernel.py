#!/usr/bin/env python3
"""Reproducibly extract and validate the X683 Android v2 boot kernel payload."""
from __future__ import annotations
import argparse, hashlib, json, struct, zlib
from pathlib import Path

def sha256(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()

def u32(b: bytes, o: int) -> int:
    return struct.unpack_from('<I', b, o)[0]

def u64(b: bytes, o: int) -> int:
    return struct.unpack_from('<Q', b, o)[0]

def cstr(b: bytes) -> str:
    return b.split(b'\0', 1)[0].decode('ascii', 'replace')

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument('boot')
    ap.add_argument('--out-dir', default='.')
    args = ap.parse_args()
    boot = Path(args.boot).read_bytes()
    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)
    if boot[:8] != b'ANDROID!':
        raise SystemExit('not an Android boot image')
    fields = [u32(boot, 8 + i * 4) for i in range(10)]
    (kernel_size, kernel_addr, ramdisk_size, ramdisk_addr, second_size,
     second_addr, tags_addr, page_size, header_version, os_version) = fields
    if header_version != 2:
        raise SystemExit(f'unsupported/unknown header version: {header_version}')
    kernel_offset = page_size
    kernel_end = kernel_offset + kernel_size
    payload = boot[kernel_offset:kernel_end]
    if payload[:2] != b'\x1f\x8b':
        raise SystemExit(f'kernel payload is not gzip: {payload[:4].hex()}')
    dec = zlib.decompressobj(31)
    image = dec.decompress(payload) + dec.flush()
    dtb = dec.unused_data
    if len(dtb) < 4 or dtb[:4] != b'\xd0\x0d\xfe\xed':
        raise SystemExit('gzip tail does not begin with DTB')
    gzip_length = len(payload) - len(dtb)
    text_offset = u64(image, 0x08)
    image_size = u64(image, 0x10)
    flags = u64(image, 0x18)
    result = {
        'boot_sha256': sha256(boot),
        'boot_header_version': header_version,
        'page_size': page_size,
        'kernel_size': kernel_size,
        'kernel_addr': hex(kernel_addr),
        'kernel_offset': kernel_offset,
        'kernel_payload_sha256': sha256(payload),
        'gzip_offset': kernel_offset,
        'gzip_length': gzip_length,
        'gzip_sha256': sha256(payload[:gzip_length]),
        'dtb_offset': kernel_offset + gzip_length,
        'dtb_size': len(dtb),
        'dtb_sha256': sha256(dtb),
        'decompressed_image_size': len(image),
        'image_sha256': sha256(image),
        'image_text_offset': hex(text_offset),
        'image_size_header': hex(image_size),
        'image_flags': hex(flags),
        'image_magic': image[0x38:0x40].hex(),
        'ramdisk_size': ramdisk_size,
        'ramdisk_addr': hex(ramdisk_addr),
        'second_size': second_size,
        'second_addr': hex(second_addr),
        'tags_addr': hex(tags_addr),
        'os_version_raw': hex(os_version),
        'board_name': cstr(boot[48:64]),
    }
    (out / 'x683_kernel.decompressed').write_bytes(image)
    (out / 'x683.dtb').write_bytes(dtb)
    (out / 'x683-executable-recovery.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))

if __name__ == '__main__':
    main()
