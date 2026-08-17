# X683 F2FS API correlation

## Stock call ABI

Direct disassembly of the X683/H694 stock `Image` now proves the Transsion GC wrapper calls `f2fs_gc` with four arguments:

```c
f2fs_gc(sbi, sync, true, NULL_SEGNO);
```

The AArch64 call record is:

```text
x0 = sbi
x1 = (sbi->mount_opt.opt >> 14) & 1
x2 = true
x3 = NULL_SEGNO (-1)
```

The wrapper at X683 `Image` offset `0x37ada8` loads `sbi->gc_mode` from `0x534`, extracts bit 14 from `mount_opt.opt` at `0x4b8`, temporarily writes GC mode 2 or 3, calls the stock `f2fs_gc` at `0x3503a8`, and restores the previous GC mode.

The third argument is therefore the historical `background` parameter, while the fourth argument is a victim-segment selector. This matches the later 4.14-era API shape rather than the older three-argument form.

## Direct stock evidence inside `f2fs_gc()`

The X683 `f2fs_gc()` implementation begins at `Image` offset `0x3503a8`.

Its entry path independently accesses the same reconstructed fields:

```text
sbi + 0x528   GC thread/state pointer
sbi + 0x428   reserved_blocks
sbi + 0x434   high/word access into current_reserved_blocks
sbi + 0x440   nquota_files candidate
sbi + 0x80    sm_info
sbi + 0x3d8   log_blocks_per_seg
sbi + 0x3dc   blocks_per_seg
sbi + 0x3e0   segs_per_sec
sbi + 0x4b8   mount_opt.opt
sbi + 0x548   GC-related pointer/state
sbi + 0x550   GC-related field
sbi + 0x534   gc_mode (used by the caller and GC core)
```

The GC core also uses the `sm_info` chain and segment-manager fields when deciding whether to proceed and when selecting/processing garbage-collection work.

## Historical public correlation

Older F2FS history contains a three-argument `f2fs_gc(sbi, sync, background)` form. Later development added a victim-segment argument and subsequently additional control parameters. The X683 binary therefore belongs to the later side of that ABI transition, not the original three-argument revision.

The appropriate public reference window remains the vendor-era 4.14 lineage, but source selection must be based on the X683 structure layout and call behavior rather than on a branch name alone.

## Current conclusion

- `sbi + 0x4b8`: **confirmed** `mount_opt.opt`; bit 14 is passed as `sync`.
- `sbi + 0x534`: **confirmed** `gc_mode`.
- X683 `f2fs_gc` call: **confirmed four-argument form** `f2fs_gc(sbi, sync, background, segno)`.
- X683 call passes `background = true` and `segno = NULL_SEGNO`.
- Stock `f2fs_gc()` entry is **confirmed at Image offset `0x3503a8`**.

Do not replace the X683 ABI with a newer `gc_control` API merely to make a source tree compile. The matching historical `gc.c`, `segment.c`, `segment.h`, and `f2fs.h` must be selected from the actual binary structure and behavior.
