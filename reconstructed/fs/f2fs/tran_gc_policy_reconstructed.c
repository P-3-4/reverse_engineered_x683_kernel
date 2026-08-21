/*
 * X683/H694 Transsion GC policy/state-machine reconstruction.
 *
 * Binary-derived semantic model for Linux 4.14.141+ / MT6768.
 *
 * This is NOT claimed to be original Transsion source and is NOT build-proven.
 * It deliberately keeps unresolved global/state fields offset-based.
 *
 * Proven boundary:
 *   tran_gc_thread_func()
 *       -> tran_do_f2fs_gc()
 *           -> f2fs_gc(sbi, sync, true, NULL_SEGNO)
 *
 * The stock victim-selection and migration logic is reconstructed separately.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define X683_SBI_GC_MODE_OFF             0x534
#define X683_SBI_MOUNT_OPT_OFF          0x4b8
#define X683_SBI_SM_INFO_OFF            0x80
#define X683_SBI_LOG_BLOCKS_PER_SEG     0x3d8
#define X683_SBI_USER_BLOCK_COUNT       0x408
#define X683_SBI_GC_MUTEX_OFF           0x508

#define X683_SM_SIT_INFO_OFF            0x00
#define X683_SM_FREE_INFO_OFF           0x08
#define X683_SM_DIRTY_INFO_OFF          0x10
#define X683_SM_RESERVED_SEGMENTS_OFF   0x60

#define X683_GLOBAL_SELECTOR0_OFF       0x890
#define X683_GLOBAL_SELECTOR1_OFF       0x894
#define X683_GLOBAL_THREAD_ACTIVE_OFF   0x898
#define X683_GLOBAL_SBI_OFF             0x8a0
#define X683_GLOBAL_TASK_OFF            0x8a8
#define X683_GLOBAL_WAKELOCK_CTL_OFF    0x968
#define X683_GLOBAL_CHARGER_CTL_OFF     0x970
#define X683_GLOBAL_FB_STATE_OFF        0x974
#define X683_GLOBAL_WAITQ_OFF           0x978
#define X683_GLOBAL_GC_COUNT_OFF        0x990
#define X683_GLOBAL_GC_TYPE_OFF         0x998
#define X683_GLOBAL_POST_GC_COUNT_OFF   0x9a0
#define X683_GLOBAL_SSR_FLAG_OFF        0x9c0
#define X683_GLOBAL_RETRY_COUNT_OFF     0x9c8
#define X683_GLOBAL_URGENT_OFF          0x9d0
#define X683_GLOBAL_PHASE_OFF           0x9d4
#define X683_GLOBAL_THREAD_CREATE_OFF   0x9e0
#define X683_GLOBAL_THREAD_DESTROY_OFF  0x9e8
#define X683_GLOBAL_FREE_SEG_OFF        0x9f0
#define X683_GLOBAL_START_METRIC_OFF    0x9f4
#define X683_GLOBAL_TYPE_STATE_OFF      0x9f8
#define X683_GLOBAL_STATUS_STATE_OFF    0x9fc
#define X683_GLOBAL_CAPACITY_STATE_OFF  0xa00
#define X683_GLOBAL_THRESHOLD_HIT_OFF   0xa04
#define X683_GLOBAL_WAKELOCK_GATE_OFF   0xa05
#define X683_GLOBAL_CONTINUE_OFF        0xa06
#define X683_GLOBAL_REMEMBERED0_OFF     0xa08
#define X683_GLOBAL_REMEMBERED1_OFF     0xa0c
#define X683_GLOBAL_LAST_METRIC_OFF     0xa10
#define X683_GLOBAL_LAST_GC_OFF         0xa18
#define X683_GLOBAL_PROC_DIR_OFF        0xa20

static inline uint8_t *x683_bytes(void *p)
{
    return (uint8_t *)p;
}

static inline uint32_t x683_u32(void *p, size_t off)
{
    uint32_t v;
    __builtin_memcpy(&v, x683_bytes(p) + off, sizeof(v));
    return v;
}

static inline void x683_put_u32(void *p, size_t off, uint32_t v)
{
    __builtin_memcpy(x683_bytes(p) + off, &v, sizeof(v));
}

static inline uint8_t x683_u8(void *p, size_t off)
{
    return x683_bytes(p)[off];
}

/* X683 evidence: ((mount_opt.opt >> 14) & 1). */
static inline unsigned int x683_gc_sync(void *sbi)
{
    return (x683_u32(sbi, X683_SBI_MOUNT_OPT_OFF) >> 14) & 1U;
}

/* Exact vendor tables observed in tran_has_enough_free_segment(). */
static const uint32_t x683_free_table_a[8] = {
    2048, 3072, 4096, 4096, 100, 100, 100, 80,
};

static const uint32_t x683_free_table_b[8] = {
    80, 80, 80, 70, 70, 70, 60, 60,
};

/*
 * Semantic helper for the proven tran_has_enough_free_segment() arithmetic.
 * The actual X683 function uses reciprocal multiplies; integer division is
 * used here only to state the recovered mathematical relation exactly.
 *
 * The SIT field at +0x10 is intentionally opaque because its original C
 * member name was not proven by the binary.
 */
bool x683_tran_has_enough_free_segment(
    void *global,
    void *sbi,
    uint32_t sit_field_plus_10,
    uint32_t free_segments,
    uint32_t reserved_segments)
{
    const unsigned int log_blocks = x683_u32(sbi, X683_SBI_LOG_BLOCKS_PER_SEG);
    const uint32_t user_segments =
        x683_u32(sbi, X683_SBI_USER_BLOCK_COUNT) >> log_blocks;
    const uint32_t u_sub = sit_field_plus_10 >> log_blocks;
    const uint32_t delta = user_segments - u_sub;

    uint8_t a = x683_u8(global, X683_GLOBAL_SELECTOR0_OFF);
    uint8_t b = x683_u8(global, X683_GLOBAL_SELECTOR1_OFF);
    uint8_t row = (a > b) ? a : b;

    uint32_t base;
    if ((user_segments >> 15) != 0) {
        base = 6144;
    } else if ((user_segments >> 13) <= 7) {
        base = x683_free_table_a[user_segments >> 13];
    } else {
        base = 0;
    }

    uint32_t free_minus_reserved = free_segments - reserved_segments;

    if (row <= 7) {
        uint32_t threshold_a = (x683_free_table_a[row] * base) / 100U;
        if (free_minus_reserved > threshold_a)
            return true;
    }

    if (row <= 7) {
        uint32_t threshold_b = (x683_free_table_b[row] * delta) / 100U;
        if (free_minus_reserved > threshold_b)
            return true;
    }

    return false;
}

/* Exact gc_type -> temporary sbi->gc_mode mapping. */
static inline uint32_t x683_mode_for_gc_type(uint32_t gc_type,
                                               uint32_t old_mode)
{
    switch (gc_type) {
    case 0:
        return old_mode;
    case 1:
        return 2;
    case 2:
        return 3;
    default:
        return old_mode;
    }
}

typedef int (*x683_stock_f2fs_gc_fn)(void *sbi,
                                     unsigned int sync,
                                     bool background,
                                     unsigned int segno);

typedef void (*x683_after_gc_fn)(void *global);

/* Semantic wrapper model of tran_do_f2fs_gc(). */
int x683_tran_do_f2fs_gc_semantic(void *global,
                                  void *sbi,
                                  x683_stock_f2fs_gc_fn stock_f2fs_gc,
                                  x683_after_gc_fn after_gc)
{
    uint32_t gc_type;
    uint32_t old_mode;
    int ret;

    ++*(uint64_t *)(x683_bytes(global) + X683_GLOBAL_GC_COUNT_OFF);
    gc_type = x683_u32(global, X683_GLOBAL_GC_TYPE_OFF);

    old_mode = x683_u32(sbi, X683_SBI_GC_MODE_OFF);
    x683_put_u32(sbi, X683_SBI_GC_MODE_OFF,
                 x683_mode_for_gc_type(gc_type, old_mode));

    ret = stock_f2fs_gc(sbi, x683_gc_sync(sbi), true, (unsigned int)-1);

    if (gc_type != 0)
        x683_put_u32(sbi, X683_SBI_GC_MODE_OFF, old_mode);

    ++*(uint64_t *)(x683_bytes(global) + X683_GLOBAL_POST_GC_COUNT_OFF);

    if (after_gc)
        after_gc(global);

    return ret;
}

/*
 * Control-flow model extracted from tran_gc_thread_func(). This is a
 * documentation reconstruction, not a source claim.
 */
void x683_tran_gc_thread_policy_model(void *global, void *sbi)
{
    uint32_t initial_gc_type = x683_u32(global, X683_GLOBAL_GC_TYPE_OFF);
    (void)sbi;

    x683_put_u32(global, X683_GLOBAL_TYPE_STATE_OFF, 0);
    x683_put_u32(global, X683_GLOBAL_STATUS_STATE_OFF, 0);
    ++*(uint64_t *)(x683_bytes(global) + X683_GLOBAL_THREAD_CREATE_OFF);

    /*
     * Main wait loop: prepare_to_wait_event() -> schedule_timeout(250) ->
     * wake/stop/freezer tests. Framebuffer events wake global+0x978.
     */

    /*
     * Urgent path: if +0x9d0 is set, the worker calls
     * tran_has_enough_free_segment(). A true result is followed by
     * tran_get_charger_type(); only return value 1 reaches the special path.
     */

    /*
     * Pressure path: direct X683 tests include dirty > ~40%, pressure >=351,
     * explicit 25%, 13%, and 27% relationships. Passing all sets +0xa00=1
     * and +0xa04=1; otherwise +0xa00=2.
     */

    /*
     * Admission can require +0xa05 == 1 plus kernel/app wakelock checks.
     * The final collector path does:
     *   __sb_start_write()
     *   mutex_trylock(sbi+0x508)
     *   tran_do_f2fs_gc(sbi)
     *   f2fs_balance_fs_bg()
     *   __sb_end_write()
     */

    x683_put_u32(global, X683_GLOBAL_GC_TYPE_OFF, initial_gc_type);
    x683_put_u32(global, X683_GLOBAL_PHASE_OFF, 4);
    ++*(uint64_t *)(x683_bytes(global) + X683_GLOBAL_THREAD_DESTROY_OFF);
}
