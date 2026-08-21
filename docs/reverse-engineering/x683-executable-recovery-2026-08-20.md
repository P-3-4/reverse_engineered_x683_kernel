# X683 Executable Recovery — 2026-08-20

## Result

The authoritative ARM64 kernel executable has been recovered from the supplied `x683_boot.img`. The payload is a gzip-compressed kernel followed by an appended DTB. This closes the executable-access blocker from the previous pass.

## Boot container

- Android boot header: v2
- page size: `0x800` (2048)
- kernel load address: `0x40080000`
- kernel payload offset: `0x800`
- kernel payload size: `9,755,348`
- kernel payload format: gzip stream followed by an appended DTB
- gzip length: `9,640,652`
- appended DTB offset: `9,642,700`
- appended DTB size: `114,696`
- ramdisk size: `943,464`
- second stage size: `0`

The gzip member decompresses to `26,615,820` bytes. The output is an ARM64 Linux Image with header magic `ARMd`, text offset `0x80000`, image-size field `0x1f09000`, and flags `0xa`.

## Hashes

- boot image: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- compressed kernel payload: `6701980890b0b18d34e88369ef50d624e3f3bee0b5a481d833141b2d256e20bd`
- compressed gzip member: `6ddfd017d9ee7152a856f46657f9ddd5287adf69d49cb853f7e747c2b7c18dfd`
- decompressed ARM64 Image: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`
- recovered DTB: `de123d41bd398f20e97ecc01a21721437ee1698f9c1cbc178096946c4aedf1d6`

## Kallsyms-to-Image mapping

`_stext = 0xffffff92d0a80800`; subtracting the ARM64 Image text offset `0x80000` gives file mapping base `0xffffff92d0a00800`.

Fresh parsing yields `56,976` kallsyms entries, `56,975` function-symbol entries, and `52,784` unique function starts inside the recovered kernel Image.

Known anchors remain:

- `f2fs_gc = 0xffffff92d0dd03a8`
- `schedule = 0xffffff92d1885428`
- `_einittext = 0xffffff92d1d7910c`

## Fresh call-site scan

Scanning only symbol-derived function ranges produces:

- `295,805` direct `BL` instruction sites
- `270,139` direct `BL` edges whose targets fall inside a known kernel function range
- `1,772` direct `BL` edges landing exactly on a known function start
- `35,034` functions with at least one mapped direct call target
- direct-call caller coverage: `66.3724%` of the `52,784` kernel function starts
- `11,692` `BLR` sites

The exact `11,692` BLR count matches the prior executable-level count, independently validating the extraction and address mapping.

## Conservative BLR triage

A static pass identifies `922` BLR sites where the target register is fed by an immediately recoverable static `ADRP + ADD + LDR` chain. None were promoted to exact function targets because the loaded values did not directly resolve to a kallsyms function start. Runtime structure/ops tables therefore remain unresolved unless further data-flow evidence proves the callback target.

## Build identity recovered from the executable

The Image contains:

`Linux version 4.14.141+ (nobody@android-build) (Android (5484270 based on r353983c) clang version 9.0.3 (https://android.googlesource.com/toolchain/clang 745b335211bb9eadfa6aa6301f84715cee4b37c5) (https://android.googlesource.com/toolchain/llvm 60cf23e54e46c807513f7a36d0a7b777920b5881) (based on LLVM 9.0.3svn)) #1 SMP PREEMPT Fri Nov 5 15:56:25 CST 2021`

This is executable evidence rather than inference. It still does not identify an exact historical vendor Git revision.

## Reproducibility

```sh
python3 tools/extract_x683_boot_kernel.py x683_boot.img --out-dir out/x683
python3 tools/rebuild_x683_executable_analysis.py out/x683/x683_kernel.decompressed x683_kallsyms.txt -o analysis/x683-direct-indirect-call-metrics-2026-08-20.json
```

The 26 MiB decompressed executable is intentionally not committed; extraction, hashes and analysis are reproducible from the supplied boot image.
