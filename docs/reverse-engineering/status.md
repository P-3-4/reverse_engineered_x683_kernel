# Reconstruction status

## Stock target

- Device: Infinix X683 / H694
- Platform: MT6768-compatible
- Kernel: Linux 4.14.141+
- Build date: 2021-11-05
- Android base: Android 10

## Fresh evidence consolidation — 2026-08-21

### Boot kernel payload

The supplied 32 MiB stock boot image has SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`. Its boot header reports a 9,755,348-byte kernel payload at page offset `0x800`, load address `0x40080000`.

The separately supplied `x683_kernel_compressed.gz` is the exact first gzip member of that boot kernel payload. Both decompress to the same 26,615,820-byte stream with SHA-256 `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`. The boot payload contains an additional 114,696 bytes after the gzip member. See `boot-kernel-payload-reconciliation.md`.

This removes an apparent kernel-artifact mismatch and establishes one canonical decompressed kernel evidence stream.

### MT6768/MSDC storage path

The recovered configuration is explicit:

```text
CONFIG_MTK_PLATFORM="mt6768"
CONFIG_MMC=y
CONFIG_MMC_BLOCK=y
CONFIG_MMC_MTK_PRO=y
CONFIG_MTK_MMC_DEBUG=y
```

The generic `CONFIG_MMC_MTK` and `CONFIG_MMC_MTK_SDIO` paths are disabled. The stock `kallsyms` contains a large vendor MSDC implementation including request, DMA, tuning, CQHCI, PM, DT parsing and initialization symbols. Representative anchors are `msdc_do_request` at `ffffff92d15631dc`, `msdc_ops_request` at `ffffff92d1566448`, `msdc_drv_probe` at `ffffff92d1564ba0`, `msdc_of_parse` at `ffffff92d1576ce0`, and `msdc_dt_init` at `ffffff92d15772ec`.

This makes MSDC/DT reconstruction a concrete next integration target rather than a generic MMC investigation. See `mt6768-storage-symbol-map.md`.

### Stock DTB/MSDC reconstruction — NEW

The appended stock FDT has been parsed directly: 542 nodes and 2,645 properties. The storage nodes are now recovered at source-property level.

- `/msdc@11230000`: 0x11230000/0x10000, IRQ 0x64, 8-bit eMMC, 200 MHz maximum, HS/DDR/HS200/HS400, non-removable, bootable, VEMC supply, three named clocks.
- `/msdc@11240000`: 0x11240000/0x10000, IRQ 0x65, 4-bit SD, 200 MHz maximum, SDR12/25/50/104 and DDR50, VMCH/VMC supplies, card-detect GPIO index `4`, two named clocks.
- `/msdc0_top@11cd0000` and `/msdc1_top@11c90000` are recovered with their exact compatible strings and register windows.
- Pinctrl phandles `0x30..0x39` and register-setting byte values are recovered.
- Supply phandles resolve to `ldo_vemc`, `ldo_vmch`, and `ldo_vmc`.

A reproducible extractor is now in `tools/reconstruct_x683_dtb.py`; the evidence and storage fragment are under `docs/reconstruction/` and `reconstruction/dts/`.

### MT6768 MSDC source-lineage lock — NEW

A public 4.14 MT6768 vendor tree was found whose `cust_mt6768_msdc.dtsi` matches the stock X683 DT structure and values across the major storage properties. Its MT6768 binding also proves `MSDC_EMMC=0`, `MSDC_SD=1`, `MSDC1_CLKSRC_200MHZ=2`, and `MSDC_SMPL_RISING=0`, matching stock.

The stock X683 DT has a concrete delta from that reference: public source uses SD card-detect GPIO 18, while stock contains GPIO index 4. The public `mtk-sd.c` also acquires clocks using different names (`source`, `hclk`, optional `source_cg`) and therefore is not safe to import unchanged. It remains a high-value structural reference only.

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
- `gc_mode == 3` bypasses this discriminator and enters at `0x366de4`;
- the clean path can escalate back to `0x366da4` based on nested manager state;
- the terminal path at `0x366e7c` conditionally runs the TLS trio only when `sbi+0x4b9` bit7 is set, but always executes `0x341250(sbi->sb,1)` and increments `stat_info+0x16c`;
- `0x34e224` is called with the SBI pointer `(sbi,1)`, not the stack object;
- the shared policy time/global comparison uses Image `+0x16c6980`.

Authoritative detail: `docs/reverse-engineering/x683-366cd4-byte-sanity-pass.md`.

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

## Remaining blockers

- exact 4.14.141 X683/Transsion kernel source baseline;
- exact MSDC source revision and source-tree ownership;
- complete MT6768 platform driver graph;
- unresolved vendor globals/callbacks in Transsion GC;
- integration of reconstructed F2FS files into a complete kernel tree;
- compiler/toolchain reproduction and final Image.gz-dtb packaging.
