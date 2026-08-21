# X683 Current State — 2026-08-20

## Image identity

The supplied boot image contains Linux `4.14.141+`, Android clang `9.0.3`, build date `2021-11-05`, ARM64 SMP PREEMPT. The decompressed kernel Image is 26,615,820 bytes.

## Binary inventory

- 52,784 unique executable Image addresses.
- 56,975 kallsyms entries total.
- 3,679 module symbols outside the Image.
- 540 cleaned embedded `kernel-4.14/...` source/header paths.
- 11,692 indirect BLR sites.
- 66.46% direct-BL caller coverage under the current reproducible definition.

## Hardware initialization evidence

The boot-image DT contains two enabled MSDC nodes, M4U, MT6358 gauge/PMIC/regulator, MT CPUFREQ, GPUFREQ, charger/BTS charger, touch, audio, DSI/display and Wi-Fi nodes. The standalone DT archive remains incomplete because it contains only a symlink.

## Major reconstructed boundaries

- F2FS segment manager/checkpoint/node/data/recovery/discard and Transsion GC policy.
- eMMC/MSDC request/DMA/IRQ/tuning/CQ/crypto/system+runtime PM entry points.
- MM reclaim/kswapd/OOM/PSI and ION/M4U/Binder shrinker surfaces.
- schedtune/PPM/cpufreq/thermal/GPU PM surfaces.
- MT6358 gauge/charger/Transsion battery integration.
- TPD/Ilitek touch, MT6768 AFE audio and WMT/WLAN/BTIF surfaces.
- SELinux/fscrypt/dm-verity/crypto/Microtrust feature boundary.

## Important limitation

The repository contains real binary-backed source models and persistent machine-readable analysis, but it is not yet a source-identical Linux 4.14 tree. Exact private structures, indirect callbacks, module bodies and the exact vendor git revision remain unresolved.
