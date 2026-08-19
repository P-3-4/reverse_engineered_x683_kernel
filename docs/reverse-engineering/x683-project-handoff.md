# X683 Kernel Reverse Engineering — Project Handoff

## Canonical state

- Repository: `P-3-4/reverse_engineered_x683_kernel`
- Branch: `kernel-reconstruction-current`
- Pre-pass canonical commit: `75273606d654df536f61f5879ae981d4dcf1e7f2`
- Target: Infinix X683 / MT6768 / ARM64 / Linux `4.14.141+`

## Completed before this pass

F2FS `f2fs_sm_info`, `sit_info`, `free_segmap_info`, `dirty_seglist_info`, `curseg_info[6]`, flush/discard control layouts, stock four-argument `f2fs_gc()`, victim selection, migration and the Transsion GC controller/state machine are established. Do not reopen them without contradictory binary evidence.

## Completed in this pass

- Whole-image executable inventory: 52,784 unique executable addresses.
- Direct ARM64 BL cross-reference scan: 44,852 functions have exact-symbol direct BL edges.
- 619 compiler-embedded source/header paths.
- F2FS checkpoint, segment allocation, node/NAT, data I/O, recovery, discard, shrinker and sysfs surfaces.
- Actual X683 eMMC/MSDC storage path, including request/DMA/IRQ/tuning/PM.
- MM/reclaim/kswapd/OOM/PSI/ION/M4U/Binder integration.
- schedtune/PPM/cpufreq/cpuidle/thermal policy surface.
- framebuffer/display-to-GC and display-to-DVFS callback surface.
- MT6358 gauge, charger detection, power-supply and Transsion battery integration.
- 13 new binary-backed reconstructed C files.

## Key addresses

```text
f2fs_gc                    0xffffff92d0dd03a8
f2fs_write_checkpoint      0xffffff92d0dce5d0
f2fs_build_segment_manager 0xffffff92d0ded138
f2fs_allocate_data_block  0xffffff92d0dea3c8
f2fs_build_node_manager    0xffffff92d0de49a8
f2fs_recover_fsync_data    0xffffff92d0df0d08
issue_discard_thread       0xffffff92d0df0120
msdc_drv_probe             0xffffff92d1564ba0
msdc_ops_request           0xffffff92d1566448
msdc_irq                   0xffffff92d1565aa0
mt6358_gauge_probe         0xffffff92d0fbb448
tran_battery_probe         0xffffff92d150cb90
fb_event                   0xffffff92d0dfacf8
schedtune_enqueue_task     0xffffff92d0b26944
binder_transaction         0xffffff92d15e7d1c
ion_alloc                  0xffffff92d15adb68
```

## Remaining

1. Exact Transsion source git revision.
2. Formal vendor global-state struct declaration.
3. Adjacent unresolved `f2fs_sb_info` fields and exact `sit_info +0x10` member name.
4. Indirect callback/proc-op container layouts.
5. Byte/structure-accurate reconstruction of the remaining generic and MTK driver surface.
6. Static DT contents; the supplied DT artifact contains only a symlink.

## Evidence discipline

HIGH = direct binary/disassembly proof. MEDIUM = binary plus historical-source correlation. LOW = inference. Binary wins over historical source. Unknowns stay offset-backed.

## Next phase rule

Choose the next target from the persistent inventory, not arbitrarily. Highest-value candidates are remaining F2FS internal fields, MSDC error/PM paths, MM+ION+M4U reclaim, PPM/scheduler/thermal callbacks, and device-specific driver probe/PM paths.
