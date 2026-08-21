# X683 Android Binder / ION Integration — 2026-08-19

`binder_transaction` = `0xffffff92d15e7d1c`, size `0x3aa0`, directly reached from `binder_ioctl_write_read()` and directly calls Binder allocator copy/ref-management paths.

Binder allocator functions include `binder_alloc_new_buf`, `binder_alloc_prepare_to_free`, `binder_alloc_free_buf`, `binder_alloc_mmap_handler`, `binder_alloc_deferred_release` and `binder_alloc_shrinker_init`.

`ion_alloc` = `0xffffff92d15adb68`, size `0x84c`, with direct callers from display/ISP/vendor allocation paths. ION heap freelist and shrinker functions are present.

Configuration proves `CONFIG_ION=y`, `CONFIG_DMA_SHARED_BUFFER=y`, `CONFIG_IOMMU_API=y`, `CONFIG_IOMMU_DMA=y` and `CONFIG_MTK_M4U=y`.

Runtime relationship:

```text
Binder / display / ISP / GPU
 -> Binder allocator or ION
 -> DMA/M4U
 -> shrinker/reclaim
 -> MM/PSI pressure
```

Source model: `reconstructed/drivers/android/x683_binder_ion_reconstructed.c`.
