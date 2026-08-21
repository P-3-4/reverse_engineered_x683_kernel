# X683 Current State — 2026-08-20 Executable Recovery + Reconstruction Pass

## Canonical state

- Repository: `P-3-4/reverse_engineered_x683_kernel`
- Branch: `kernel-reconstruction-current`
- Pass started from canonical tip `c1dd0eaa3fdf7e384b0f6089c2defeba2cca3213`.
- Device: Infinix X683 / MT6768 / ARM64 / Linux `4.14.141+`.

## Authoritative executable recovery

The previous executable-access blocker is closed. `x683_boot.img` contains an Android boot v2 kernel payload beginning at offset `0x800`: a gzip member followed by an appended DTB.

- boot SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- compressed kernel payload SHA-256: `6701980890b0b18d34e88369ef50d624e3f3bee0b5a481d833141b2d256e20bd`
- decompressed Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`
- DTB SHA-256: `de123d41bd398f20e97ecc01a21721437ee1698f9c1cbc178096946c4aedf1d6`
- gzip length: `9,640,652`
- DTB offset: `9,642,700`
- DTB size: `114,696`
- decompressed Image size: `26,615,820`
- ARM64 text offset: `0x80000`

The Image maps with `file_base = _stext - 0x80000 = 0xffffff92d0a00800`.

## Fresh binary measurements

- kallsyms entries: `56,976`
- function-symbol entries: `56,975`
- unique kernel function starts: `52,784`
- direct BL instruction sites: `295,805`
- direct BL edges into known function ranges: `270,139`
- exact symbol-start BL edges: `1,772`
- functions with at least one mapped direct BL caller: `35,034`
- direct-call caller coverage: `66.3724%`
- BLR sites: `11,692`
- conservative static ADRP+ADD+LDR BLR chains: `922`

The fresh `11,692` BLR count exactly matches the previous executable-level count.

## Build identity

The executable contains the kernel build identity `4.14.141+`, Android clang `9.0.3`, LLVM `9.0.3svn`, and build timestamp `Fri Nov 5 15:56:25 CST 2021`. It also contains hundreds of `kernel-4.14/...` source-path strings, including scheduler, power, ARM64, MMC, F2FS and VFS paths.

## Subsystems

F2FS, storage/MSDC, MM/reclaim, ION/M4U/DMA-BUF, Binder, scheduler/schedtune, PPM/cpufreq/thermal, power/PM, display/GPU, battery/charger, input, audio/network, security/crypto and DT/driver surfaces remain evidence-backed mappings. The newly recovered Image now allows those mappings to be deepened with real executable references instead of relying only on symbol tables.

The canonical F2FS private layouts and GC ABI are preserved unchanged.

## Buildability gate

- complete Linux 4.14.141 source baseline: **not recovered**
- `make olddefconfig`: **not run**
- `make prepare`: **not run**
- `make modules_prepare`: **not run**
- `make Image`: **not run**
- required modules build: **not run**

No buildability claim is made.

## Boot/functionality gate

- replacement kernel boot: **not tested**
- Android userspace boot: **not verified**
- storage/display/touch/USB/audio/Wi-Fi/Bluetooth/battery/suspend functionality: **not verified**

## Remaining hard blockers

1. Resolve high-value runtime BLR/ops tables and callback structures from the recovered Image.
2. Recover the closest exact historical Transsion/X683 4.14.141 vendor source revision.
3. Obtain missing runtime module binaries, especially WLAN/WMT/FPSGO and other code outside the built-in Image.
4. Integrate the verified baseline into a genuine ARM64 kernel tree and start the build gates.
