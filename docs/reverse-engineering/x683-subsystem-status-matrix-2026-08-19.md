# X683 Subsystem Status Matrix — 2026-08-19

The table is a name/classification inventory over the 52,784 executable kallsyms addresses. It is an inventory metric, not a claim that every function has been decompiled.

| subsystem | functions | status |
|---|---:|---|
| F2FS | 533 | deep: GC + segment/checkpoint/node/data/recovery/discard/shrinker integration |
| storage | 2,406 | substantial: MMC/MSDC/CMDQ/DMA/IRQ/PM mapped |
| battery/charger | 5,738 | broad vendor + power-supply surface mapped |
| USB | 1,891 | broad configured/runtime surface mapped |
| network | 1,492 | broad symbol surface; device-specific depth remains |
| display/GPU | 1,272 | framebuffer/GED/PPM integration mapped |
| power | 1,219 | PM/MTK power paths mapped |
| audio | 1,041 | symbol/source surface identified; deeper driver reconstruction remains |
| IOMMU/DMA | 1,019 | IOMMU/M4U/ION/DMA integration mapped |
| input/sensors | 892 | source/symbol surface identified |
| Android IPC | 886 | Binder/ION/Android memory mapped |
| cpufreq/DVFS/thermal | 832 | PPM/cpufreq/cpuidle/EEM/thermal mapped |
| security | 752 | SELinux/verity/fscrypt/TEE surface identified |
| ARM64 architecture | 592 | exception/IRQ/SMP/PM surface identified |
| VFS | 580 | generic VFS surface identified |
| crypto | 570 | configured crypto surface identified |
| scheduler | 334 | schedtune/PPM and scheduler hooks mapped |
| memory | 246 | reclaim/kswapd/OOM/PSI mapped |
| Transsion GC | 9 | deeply reconstructed; prior phase complete |

`other` contains 30,480 symbols that do not match the conservative subsystem naming rules and therefore are not silently assigned.

## Highest-value remaining depth

1. F2FS adjacent structure fields and indirect callbacks.
2. MSDC error/tuning/PM control flow.
3. MM + ION + M4U reclaim ownership.
4. PPM/scheduler/thermal callback graph.
5. Device-specific input/audio/display probes and PM.
