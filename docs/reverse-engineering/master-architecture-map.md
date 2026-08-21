# X683 Master Architecture Map

## Evidence status

Primary authority is the recovered X683 ARM64 Image, correlated with kallsyms, config and recovered DTB. Direct executable analysis currently covers 295,805 BL sites and 11,692 BLR sites.

## Proven execution spine

`VFS -> F2FS -> block/bio -> MMC/MSDC` is established as the target storage architecture, with F2FS vendor GC symbols and the four-argument `f2fs_gc` ABI already identified.

## Proven F2FS structural anchors

`f2fs_sm_info=0xA8`, `sit_info=0xA8`, `free_segmap_info=0x20`, `dirty_seglist_info=0x90`, `curseg_info=0x70`, `curseg_info[6]=0x2A0`.

## Unresolved runtime edges

MSDC private state, generic block request-queue callbacks, DMA/IRQ completions, Android ION/DMA-BUF/M4U callbacks, Binder allocator callbacks, scheduler vendor hooks, PPM/cpufreq/thermal callbacks, GPU/display callbacks and PM callback tables remain subject to binary data-flow recovery.

No inferred callback is represented as proven in this map.
