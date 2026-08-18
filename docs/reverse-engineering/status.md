# Reconstruction status

## Stock target

- Device: Infinix X683 / H694
- Platform: MT6768-compatible
- Kernel: Linux 4.14.141+
- Build date: 2021-11-05
- Android base: Android 10

## F2FS

The stock ramdisk uses `tran_gc` as an actual userdata F2FS mount option. `CONFIG_F2FS_TRAN_GC=y` is enabled in the recovered configuration.

The Transsion GC path calls the stock `f2fs_gc()` and `f2fs_balance_fs_bg()` and is coupled to charging, USB, framebuffer events, wakelock state, free-segment state, fragmentation, and GC mode.

## Current layout reconstruction

High-confidence `f2fs_sb_info` offsets are documented in `f2fs-layout.md`. `sbi + 0x534` is now treated as internal `gc_mode`, not the older 4.14 `fggc_threshold` field. The victim-selection reconstruction also now uses the historical type-sensitive greedy cost model.

## Current GC reconstruction state

- 3-argument `f2fs_gc(sbi, sync, background)`: high confidence.
- `__get_victim()` manager boundary: high confidence.
- Dirty-segment victim selection and `last_victim[]`: high confidence at historical-implementation level.
- `gc_mode` at `0x534`: high confidence.
- Historical `fggc_threshold` / `no_fggc_candidate()`: reference-only; exact X683 retention is unresolved.
- Exact Transsion GC-state transitions: unresolved and now the primary target.

## Build strategy

1. Match the exact X683-era 4.14 F2FS revision.
2. Integrate reconstructed Transsion GC.
3. Bring in MT6768 common kernel source.
4. Apply recovered X683/H694 configuration.
5. Reconstruct DTB/DTBO as DTS/DTSI.
6. Restore required MT6768 and X683-specific drivers.
7. Build with the stock-era Android Clang toolchain target.
8. Package and boot-test before modernization.
