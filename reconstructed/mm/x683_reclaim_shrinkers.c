/* X683 MM/reclaim evidence model; bodies remain unresolved where indirect. */
#include <stdint.h>

struct x683_reclaim_symbol {
    const char *symbol;
    uint64_t address;
    const char *role;
    const char *evidence;
};

static const struct x683_reclaim_symbol x683_reclaim_path[] __attribute__((used)) = {
    { "kswapd",            0xffffff92d0c0f95cULL, "background reclaim worker", "kallsyms" },
    { "wakeup_kswapd",     0xffffff92d0c0f6c0ULL, "reclaim wakeup", "kallsyms" },
    { "try_to_free_pages", 0xffffff92d0c0e0e0ULL, "global reclaim entry", "kallsyms" },
    { "shrink_node",       0xffffff92d0c10f28ULL, "node reclaim", "kallsyms" },
    { "shrink_slab",       0xffffff92d0c0c194ULL, "shrinker dispatch", "kallsyms" },
    { "out_of_memory",     0xffffff92d0bf8e80ULL, "OOM decision", "kallsyms" },
};

/* The authoritative machine-readable XREF generator contains the complete
 * executable-symbol inventory; this C unit intentionally exposes only the
 * evidence model, not a guessed source implementation. */
struct x683_shrinker_binding {
    const char *owner;
    const char *count_symbol;
    const char *scan_symbol;
    const char *registration_symbol;
};

static const struct x683_shrinker_binding x683_android_shrinkers[] __attribute__((used)) = {
    { "ION heap", "ion_heap_shrink_count", "ion_heap_shrink_scan", "ion_heap_init_shrinker" },
    { "ION system heap", "ion_heap_shrink_count", "ion_system_heap_shrink", "ion_heap_init_shrinker" },
    { "ION MM heap", "ion_heap_shrink_count", "ion_mm_heap_shrink", "ion_heap_init_shrinker" },
    { "Binder", "binder_shrink_count", "binder_shrink_scan", "binder_alloc_shrinker_init" },
};

/* X683 config: PSI=y, ION=y, MTK_ION=y, M4U=y, IOMMU_API=y, IOMMU_DMA=y. */
static const uint8_t x683_mm_pressure_features[] __attribute__((used)) = { 1, 1, 1, 1, 1, 1 };
