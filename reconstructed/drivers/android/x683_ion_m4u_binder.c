/* X683 Android memory integration: ION/M4U/Binder evidence model. */
#include <stdint.h>

struct x683_symbol_ref { const char *name; uint64_t address; };

static const struct x683_symbol_ref x683_ion[] __attribute__((used)) = {
    { "ion_alloc", 0xffffff92d15adb68ULL },
    { "ion_heap_freelist_shrink", 0xffffff92d15b1b00ULL },
    { "ion_heap_init_shrinker", 0xffffff92d15b1d24ULL },
    { "ion_heap_shrink_count", 0xffffff92d15b1d5cULL },
    { "ion_heap_shrink_scan", 0xffffff92d15b1dc4ULL },
    { "ion_page_pool_shrink", 0xffffff92d15b2224ULL },
    { "ion_system_heap_shrink", 0xffffff92d15b2becULL },
    { "ion_mm_heap_shrink", 0xffffff92d15c1eacULL },
};

static const struct x683_symbol_ref x683_m4u[] __attribute__((used)) = {
    { "m4u_probe", 0xffffff92d101fc00ULL },
    { "m4u_alloc_mva_sg", 0xffffff92d101b1c4ULL },
    { "m4u_dealloc_mva_sg", 0xffffff92d101b304ULL },
    { "m4u_map_sgtable", 0xffffff92d10238c4ULL },
    { "m4u_get_sgtable_pages", 0xffffff92d101a048ULL },
    { "m4u_create_sgtable", 0xffffff92d101a660ULL },
    { "m4u_destroy_sgtable", 0xffffff92d101aa88ULL },
};

static const struct x683_symbol_ref x683_binder[] __attribute__((used)) = {
    { "binder_transaction", 0xffffff92d15e7d1cULL },
    { "binder_alloc_shrinker_init", 0xffffff92d15efe00ULL },
    { "binder_shrink_count", 0xffffff92d15f0ac4ULL },
    { "binder_shrink_scan", 0xffffff92d15f0ae4ULL },
};

/* Integration proven by configuration and symbol/source-path fingerprints:
 * Android Binder + ION are built in; M4U and IOMMU APIs are enabled. The exact
 * cross-subsystem ownership transitions are partly indirect and remain in the
 * persistent analysis rather than being guessed here. */
