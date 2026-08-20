# X683 MM / Android Memory Deep Reconstruction — 2026-08-20

The config proves PSI, ION, MTK ION, M4U, IOMMU API/DMA, Binder with binder/hwbinder/vndbinder, Transsion Binder block/buffer monitoring, and ashmem.

Key reclaim symbols: `wakeup_kswapd` `0xffffff92d0c0f6c0`; `kswapd` `0xffffff92d0c0f95c`; `try_to_free_pages` `0xffffff92d0c0e0e0`; `shrink_slab` `0xffffff92d0c0c194`; `shrink_node` `0xffffff92d0c10f28`; `out_of_memory` `0xffffff92d0bf8e80`; `oom_kill_process` `0xffffff92d0bf960c`.

The source fingerprint includes `mm/vmscan.c`, `mm/oom_kill.c`, `mm/memcontrol.c`, `mm/page_alloc.c`, `mm/page-writeback.c`, `mm/migrate.c`, `mm/slub.c`, `mm/zsmalloc.c`, `kernel/sched/psi.c`, Binder and ION sources.

ION/Binder shrinkers are mapped: `ion_heap_init_shrinker` `0xffffff92d15b1d24`; `ion_heap_shrink_count` `0xffffff92d15b1d5c`; `ion_heap_shrink_scan` `0xffffff92d15b1dc4`; `ion_system_heap_shrink` `0xffffff92d15b2bec`; `ion_mm_heap_shrink` `0xffffff92d15c1eac`; `binder_alloc_shrinker_init` `0xffffff92d15efe00`; `binder_shrink_count` `0xffffff92d15f0ac4`; `binder_shrink_scan` `0xffffff92d15f0ae4`.

M4U entry points include `m4u_probe`, `m4u_alloc_mva_sg`, `m4u_dealloc_mva_sg`, `m4u_map_sgtable`, and SG-table lifecycle helpers.

The evidence supports a real Android memory-pressure chain through reclaim/shrinker infrastructure. It does not prove a classic LMK kernel implementation; that distinction is preserved.
