# X683 MM / Android Memory Integration — 2026-08-19

## Configuration

Proven: `CONFIG_PSI=y`, `CONFIG_SCHED_TUNE=y`, `CONFIG_CMA=y`, `CONFIG_ION=y`, `CONFIG_ASHMEM=y`, `CONFIG_DMA_SHARED_BUFFER=y`, `CONFIG_IOMMU_API=y`, `CONFIG_IOMMU_DMA=y`, `CONFIG_MTK_M4U=y`, `CONFIG_MTK_LOWMEM_HINT=y`, `CONFIG_TRAN_OOM_KILLER_OPT=y`.

## Reclaim/OOM

The standard 4.14 reclaim surface is present: `wakeup_kswapd`, `kswapd`, `shrink_node`, `shrink_slab`, active/inactive LRU shrink and direct reclaim. The stock OOM surface includes `oom_badness`, `oom_kill_process`, `oom_evaluate_task`, `oom_reaper` and OOM notifiers.

No classic `lowmemorykiller` function family was found. Vendor low-memory evidence is instead `trigger_lowmem_hint` plus its proc and tracepoint surface.

## Shrinkers

X683 has F2FS, ION, Binder, zsmalloc and ext4 shrinker surfaces. F2FS `f2fs_shrink_scan()` reaches extent-tree, NAT and NID freeing.

ION `ion_alloc()` is `0xffffff92d15adb68` and is called by display/ISP/vendor multimedia allocation paths. M4U exposes `m4u_register_reclaim_callback`, `m4u_unregister_reclaim_callback` and `m4u_reclaim_notify`.

## Android memory graph

```text
Binder / display / ISP / GPU
          -> ION / DMA / M4U
          -> shrinkers / reclaim
          -> global MM pressure / PSI
```

## Reconstruction

`reconstructed/mm/x683_memory_policy_reconstructed.c` records the proven MM/vendor integration without inventing an LMK algorithm.
