# Reconstruction status

## Stock target

- Device: Infinix X683 / H694
- Platform: MT6768-compatible
- Kernel: Linux 4.14.141+
- Build date: 2021-11-05
- Android base: Android 10

## F2FS

The stock ramdisk uses `tran_gc` as an actual userdata F2FS mount option. `CONFIG_F2FS_TRAN_GC=y` is enabled in the recovered configuration.

The Transsion GC path wraps the stock X683 four-argument `f2fs_gc()` and separately reaches filesystem synchronization/balance machinery. It is coupled to charging, USB, framebuffer events, wakelock state, free-segment state, fragmentation, GC mode, and vendor policy state.

## Current layout reconstruction

High-confidence `f2fs_sb_info` offsets are documented in `f2fs-layout.md`.

```text
sbi +0x4b8  mount_opt.opt
sbi +0x508  gc_mutex
sbi +0x534  gc_mode
sbi +0x568  f2fs_stat_info *
```

The X683 `f2fs_gc()` call ABI is:

```c
int f2fs_gc(struct f2fs_sb_info *sbi,
            bool sync,
            bool background,
            unsigned int segno);
```

## Current GC reconstruction state

- four-argument X683 `f2fs_gc(sbi, sync, background, segno):` high confidence.
- `__get_victim()` / dirty-manager victim boundary: high confidence.
- Dirty-segment victim selection and SIT `last_victim[]`: high confidence at historical-implementation level.
- `gc_mode` at `0x534`: high confidence.
- Transsion controller mapping: controller `0` unchanged, `1 -> gc_mode 2`, `2 -> gc_mode 3`.
- Stop-4/5 raw controller write `2` therefore selects the temporary GREEDY (`gc_mode=3`) path.

## `0x366cd4` vendor policy — fresh-byte correction

Fresh re-analysis of the supplied stock boot image supersedes the older policy prose:

- actual live boundary extends through `0x366f2c` (canary-failure tail); next function starts `0x366f30`;
- the seven fields at `sbi+0x44c/+0x450/+0x454/+0x448/+0x444/+0x45c/+0x458` are a discriminator, not an all-zero prerequisite;
- any nonzero field selects the active path at `0x366da4`;
- all seven zero selects the alternate path at `0x366ee0`;
- `gc_mode == 3` bypasses that discriminator and enters at `0x366de4`;
- the clean path can escalate back to `0x366da4` based on nested manager state;
- the terminal path at `0x366e7c` conditionally runs the TLS trio only when `sbi+0x4b9` bit7 is set, but always executes `0x341250(sbi->sb,1)` and increments `stat_info+0x16c`;
- `0x34e224` is called with the SBI pointer `(sbi,1)`, not the stack object;
- the shared policy time/global comparison uses Image `+0x16c6980`.

Authoritative detail:
`docs/reverse-engineering/x683-366cd4-byte-sanity-pass.md`.

## Build strategy

1. Match the exact X683-era 4.14 F2FS revision.
2. Integrate the recovered Transsion vendor layer.
3. Bring in MT6768 common kernel source.
4. Apply recovered X683/H694 configuration.
5. Reconstruct DTB/DTBO as DTS/DTSI.
6. Restore required MT6768 and X683-specific drivers.
7. Build with the stock-era Android Clang toolchain target.
8. Compare symbols/control-flow to the stock image before boot testing.
9. Package and boot-test before modernization.
