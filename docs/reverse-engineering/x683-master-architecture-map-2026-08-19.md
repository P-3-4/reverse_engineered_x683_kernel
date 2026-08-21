# X683 Master Architecture Map — 2026-08-19

## Evidence base

Target: Infinix X683 / MT6768 / ARM64 / Linux 4.14.141+.

The Image contains 56,976 kallsyms records and 52,784 unique executable symbols in the recovered text range. A direct ARM64 BL scan found exact-symbol call edges for 44,852 functions.

## Runtime architecture

```text
                         X683 KERNEL
                              |
        +---------------------+---------------------+
        |                     |                     |
       MM                  SCHED/PPM              PM/DVFS
        |                     |                     |
  reclaim/kswapd        schedtune/CFS          cpufreq/cpuidle
  PSI/OOM                thermal/limits         EEM/PBM/SPM
  ION/Binder/M4U              |                     |
        |                     +----------+----------+
        |                                |
        +--------------------+-----------+-----------+
                             |                       |
                          VFS/block                display
                             |                       |
                           F2FS              framebuffer notifier
                             |                       |
                    segment manager            PPM/GED policy
                    checkpoint/GC                   |
                             |                       |
                          bio/MMC                    |
                             |                       |
                       MSDC/eMMC                 GPU/panel
                             |
                           DMA/IRQ

Battery/charger -> power_supply -> PPM/thermal/USB policy
Binder/ION ------> MM pressure + multimedia memory
```

## Key proven bridges

- F2FS `f2fs_write_checkpoint()` is central to GC, sync, recovery and unmount.
- F2FS allocation reaches normal bio/page-write paths; GC uses the same allocation/migration machinery.
- F2FS reaches the block/MMC stack; X683 storage is eMMC/MSDC.
- `msdc_ops_request()` reaches DMA, crypto, tuning and controller command paths.
- `schedtune_enqueue_task()` is reached from both fair and RT enqueue paths.
- `fb_register_client()` is called by `tran_gc_init()` and PPM/GED/display clients.
- `fb_event()` wakes the Transsion GC waitqueue.
- `battery_update()` calls `power_supply_changed()`.
- PPM exposes low-battery, thermal, game and display-off policy callbacks.
- ION/Binder shrinkers connect Android multimedia/IPC memory to MM pressure.

## Major source surfaces

F2FS source paths: checkpoint, data, extent cache, f2fs core, file, inline, inode, namei, node, recovery, segment, super and xattr.

Storage source paths: MMC core/block/host plus MediaTek ComboA/MT6768 MSDC and tuning code.

Power source paths: MT6768 cpufreq, cpuidle, EEM, PPM, PBM, SPM, thermal and PMIC.

Android source paths: Binder, ION, DMA-buf and Android memory interfaces.

## Coverage status

Function mapping: 100% of recovered executable kallsyms.
Direct-call graph: 84.97% of executable functions have an exact-symbol BL edge.
Actual source reconstruction: substantially below 80–90%; unresolved source equivalence and indirect-call structure are explicit.
