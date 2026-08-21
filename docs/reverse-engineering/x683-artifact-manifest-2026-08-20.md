# X683 Artifact Manifest — 2026-08-20

## Boot image

- `x683_boot.img` SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- size: 33,554,432 bytes
- Android boot header v2; page size 2048
- compressed kernel: 9,755,348 bytes at load address `0x40080000`
- compressed ramdisk: 943,464 bytes
- DT table: 114,760 bytes
- decompressed ARM64 Image: 26,615,820 bytes
- decompressed Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

The Image identifies Linux `4.14.141+`, Android clang `9.0.3`, build date `2021-11-05`.

## Kallsyms/config

- `x683_kallsyms.txt` SHA-256: `e47a91e9c933249d9902a06a9d94e6ee0f9ac0f2d33cd1fa00589fc9fed34d56`
- 56,975 symbol entries; 52,784 executable Image addresses; 3,679 module entries.
- `x683_config.txt` SHA-256: `7d789b857f2fd7af52ddbfdd5e36fba33d62162536635de15423e80525010f56`
- `x683_config.gz` SHA-256: `b75860caa0b52e5e8a7b747c59a481e01aa226f64aecfccfff0184623009dda5`

## Device tree

`device-tree.tar.gz` SHA-256: `f205510ac17b721bac36e53979f9fdeb8c6c5104455b8f86dfeb779c7e7d93c8`. The archive contains only a `proc/device-tree` symlink and is therefore incomplete as a static DT artifact.

The complete DT table was recovered from the boot image. It contains an FDT beginning at table offset `0x40`, reports `mediatek,MT6768`, and parses to 542 nodes, with 382 nodes containing `compatible` and/or `reg`.

## Persistent outputs

Compact analysis summaries and the regeneration script are stored under `analysis/` and `tools/`. The full generated function inventory and direct-call graph are reproducible from the supplied Image and kallsyms and are intentionally not replaced by a truncated hand-edited database.
