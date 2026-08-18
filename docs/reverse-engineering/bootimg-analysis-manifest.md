# X683/H694 boot.img analysis manifest

This file records the reusable evidence needed for future reverse-engineering passes so the original 32 MiB boot.img does not need to be uploaded again for these GC investigations.

## Source image

- SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- Size: `33554432`
- Board: `CY-X683-H694-E`
- Kernel compressed size: `9755348`
- Ramdisk compressed size: `943464`
- Page size: `2048`
- Kernel load address: `0x40080000`
- AArch64 Image text offset: `0x80000`

## Reusable kernel offsets

| Image offset | Meaning |
|---:|---|
| `0x345d58` | read `sbi + 0x534` |
| `0x345d6c` | temporary `gc_mode = 3` |
| `0x345d78` | restore `gc_mode` |
| `0x3503a8` | stock `f2fs_gc` entry point |
| `0x352f10` | `gc_mode` read in GC/victim path |
| `0x365918` | `gc_mode` read in GC logic |
| `0x374d4c` | F2FS sysfs handler region |
| `0x3750f4` | standard urgent `gc_mode = 3` writer |
| `0x37515c` | standard idle `gc_mode` writer (1/2) |
| `0x375168` | standard normal `gc_mode = 0` writer |
| `0x376f84` | `tran_gc_thread_func` literal reference |
| `0x37793c` | `tran_gc loop static detect` reference |
| `0x37ada8` | Transsion GC wrapper |
| `0x37adc4` | controller read at global `+0x998` |
| `0x37adfc` | Transsion temporary `gc_mode = 3` |
| `0x37ae00` | Transsion call to `f2fs_gc` |
| `0x37ae04` | restore old `gc_mode` |
| `0x37ae68` | Transsion temporary `gc_mode = 2` |
| `0x37ae6c` | Transsion call to `f2fs_gc` |
| `0x37ae7c` | restore old `gc_mode` |

## Important constants/literals

- `gc_mode` string: image offset `0x10a30b1`
- `tran_gc`: `0x10a4313`
- `gc_urgent_sleep_time`: `0x10a521c`
- `gc_urgent`: `0x10a526b`
- `gc_idle`: `0x10a52a7`
- `tran_f2fs_gc`: `0x10a5e20`
- `gc mode is COST`: `0x10a5e7a`
- `gc mode is URGENT`: `0x10a5e8b`
- `gc mode is GREEDY`: `0x10a5e9e`
- `tran_gc_usb_wakelock`: `0x10a5ee7`
- `tran_gc_thread_func create`: `0x10a6039`
- `tran_gc loop static detect`: `0x10a60b9`
- `kernel or os is holding wakelock!`: `0x10a60d5`
- `f2fs is writing data`: `0x10a6133`
- `match: Stop condition 4, dec_seg=%d, inc_written_seg=%d, switch to SSR`: `0x10a626b`
- `tran_urgent_gc`: `0x10a63ac`
- `need_switch_ssr`: `0x10a6359`
- `detect_charger_type`: `0x10a6414`

## Re-analysis rule

The SHA-256 identifies the exact source image. The offsets above are file offsets in the decompressed AArch64 kernel image, whose runtime mapping begins from the Android boot kernel load address plus the Image text offset. Do not treat unrelated structures using the same numeric offset `0x534` as `sbi->gc_mode`; pointer provenance must be established from surrounding code.
