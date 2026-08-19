# X683 Kernel Reverse Engineering — Current State Snapshot

Date: 2026-08-19
Canonical branch: `kernel-reconstruction-current`
Previous canonical commit: `75273606d654df536f61f5879ae981d4dcf1e7f2`

## Whole-image deep-pass result

The completed F2FS GC/Transsion policy boundary was not reopened. This pass expanded the reconstruction into F2FS checkpoint/segment/node/data/recovery/discard/shrinker paths and connected those paths to the actual X683 storage, MM, Android IPC/memory, scheduler, display, battery and power surfaces.

## Inventory

- Kallsyms records: **56,976**
- Unique executable symbols in recovered Image range: **52,784**
- Functions with at least one exact-symbol direct ARM64 `BL` edge: **44,852 / 52,784 = 84.97%**
- Embedded source/header paths: **619**
- F2FS source/header paths: **14**
- New reconstructed C artifacts: **13**
- New unique model functions: **78**

## F2FS expansion

Key functions now connected:

```text
f2fs_write_checkpoint      0xffffff92d0dce5d0  size 0x1400
f2fs_flush_nat_entries     0xffffff92d0de3f2c  size 0x85c
f2fs_flush_sit_entries     0xffffff92d0dec660  size 0xad8
f2fs_build_segment_manager 0xffffff92d0ded138  size 0x1cd4
f2fs_allocate_data_block   0xffffff92d0dea3c8  size 0x734
f2fs_allocate_new_segments 0xffffff92d0de94fc  size 0xd8
allocate_segment_by_default 0xffffff92d0df0454 size 0x394
new_curseg                 0xffffff92d0df07e8  size 0x4e4
f2fs_build_node_manager    0xffffff92d0de49a8  size 0x690
f2fs_recover_fsync_data    0xffffff92d0df0d08  size 0x1bdc
issue_discard_thread       0xffffff92d0df0120  size 0x334
f2fs_shrink_scan           0xffffff92d0df29c8  size 0x148
```

The checkpoint graph is `f2fs_write_checkpoint -> NAT flush -> SIT flush -> discard/prefree handling -> unblock/statistics`. Segment manager construction is directly called by `f2fs_fill_super()` and teardown by mount cleanup/unmount. Recovery is directly reached from `f2fs_fill_super()`.

## Storage

X683 storage is eMMC/MSDC, not a UFS primary path:

```text
F2FS -> VFS/bio -> block/MMC/CMDQ -> MediaTek MSDC -> DMA/IRQ -> eMMC
```

Key driver addresses:

```text
msdc_drv_probe       0xffffff92d1564ba0
msdc_ops_request     0xffffff92d1566448
msdc_irq             0xffffff92d1565aa0
msdc_execute_tuning  0xffffff92d1564068
msdc_runtime_suspend 0xffffff92d1567718
```

## MM / Android memory

Proven surfaces: kswapd/reclaim, OOM/reaper, PSI, F2FS shrinker, ION shrinker, Binder allocator shrinker, M4U reclaim callbacks and vendor `trigger_lowmem_hint`. No classic `lowmemorykiller` function family was found.

## Scheduler / power / display / battery

Proven integrations include schedtune enqueue/dequeue/boost, MediaTek PPM callbacks, cpufreq/cpuidle/EEM/PBM/SPM/thermal, framebuffer notifier registration, display-off/GED policy, MT6358 gauge, charger detection, power-supply updates, USB charger events and Transsion battery probe.

A concrete display-to-GC edge exists: `tran_gc_init()` registers a framebuffer notifier and `fb_event()` wakes the vendor GC waitqueue.

## New source

13 binary-backed C models were added under `reconstructed/`, covering F2FS segment/checkpoint/node/data/recovery, MM, MSDC/eMMC, scheduler/PPM, display, battery/gauge, power, Binder/ION and ARM64 runtime paths. They are syntax-checked semantic reconstructions, not claims of byte-identical original source.

## Coverage interpretation

Function **mapping** is 100% of the recovered executable kallsyms surface. Direct-call graph coverage is 84.97%. Actual source reconstruction remains below the requested 80–90% threshold; no inflated percentage is claimed. The major gain in this pass is persistent whole-image inventory plus real cross-subsystem control-flow mapping.

## Remaining

- exact Transsion source revision;
- formal vendor global-state declaration;
- adjacent unresolved `f2fs_sb_info` fields;
- exact `sit_info + 0x10` historical member name;
- indirect callback/proc-op container layouts;
- byte/structure-accurate reconstruction of the remaining large generic/MTK driver surface;
- static DT contents (current DT artifact contains only a symlink).
