# X683 Kernel Reverse Engineering — Project Handoff

## Canonical state

- Repository: `P-3-4/reverse_engineered_x683_kernel`
- Branch: `kernel-reconstruction-current`
- Starting tip for the executable-recovery pass: `c1dd0eaa3fdf7e384b0f6089c2defeba2cca3213`
- Current pass work is committed directly on the canonical branch.
- Target: Infinix X683 / MT6768 / ARM64 / Linux `4.14.141+`.
- Authoritative artifacts: supplied X683 boot image, kallsyms and config; complete DTB is recoverable from the boot image.

## Executable blocker closed

The authoritative ARM64 executable has been recovered from `x683_boot.img` as a gzip-compressed kernel payload followed by an appended DTB. Recovery is reproducible with `tools/extract_x683_boot_kernel.py`.

- boot image SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- decompressed Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`
- recovered DTB SHA-256: `de123d41bd398f20e97ecc01a21721437ee1698f9c1cbc178096946c4aedf1d6`
- Android boot header: v2, page size 2048
- gzip member: offset 2048, length 9,640,652
- appended DTB: offset 9,642,700, size 114,696
- decompressed Image: 26,615,820 bytes

## Fresh executable analysis

- kallsyms entries: `56,976`
- function-symbol entries: `56,975`
- unique kernel function starts: `52,784`
- direct BL sites: `295,805`
- direct BL edges into known function ranges: `270,139`
- exact symbol-start BL edges: `1,772`
- direct-call callers: `35,034`
- direct-call caller coverage: `66.3724%`
- BLR sites: `11,692`
- conservative static ADRP+ADD+LDR BLR sites: `922`

The exact `11,692` BLR count matches the prior executable-level inventory and independently validates the recovered Image/address mapping.

## Build identity

The executable contains the complete build string identifying Linux `4.14.141+`, Android clang `9.0.3`, LLVM `9.0.3svn`, and build timestamp `Fri Nov 5 15:56:25 CST 2021`. This is now executable evidence rather than inference. The exact historical vendor Git revision remains unresolved.

## Source-path evidence

At least `862` `kernel-4.14` path occurrences are present, including architecture, scheduler, power, MMC and other core paths. Public MT6768 4.14 source trees remain correlation references only; no exact Transsion/X683 Git revision is proven.

## Preserved F2FS reconstruction

The canonical proven values are unchanged: `f2fs_sm_info=0xA8`, `sit_info=0xA8`, `free_segmap_info=0x20`, `dirty_seglist_info=0x90`, `curseg_info=0x70`, `curseg_info[6]=0x2A0`, `sm_info+0x98=flush_cmd_control`, `sm_info+0xA0=discard_cmd_control`, `discard_cmd_control=0x20B0`, and the proven four-argument `f2fs_gc` ABI.

## Build/boot gates

- Complete 4.14.141 source tree: **not yet recovered**.
- `make olddefconfig`: **not run**.
- `make prepare`: **not run**.
- `make modules_prepare`: **not run**.
- `make Image`: **not run**.
- replacement boot: **not tested**.
- Android userspace boot: **not verified**.
- hardware functionality: **not verified**.

## Remaining blockers

1. Resolve high-value BLR/ops tables from the recovered executable data-flow.
2. Recover the closest exact historical Transsion/X683 4.14.141 vendor source tree.
3. Recover missing runtime module binaries, especially WLAN/WMT/FPSGO and any vendor code outside the Image.
4. Integrate the verified baseline and begin the real ARM64 build transition.

Unknowns remain explicit and are never promoted from inference to proof.
