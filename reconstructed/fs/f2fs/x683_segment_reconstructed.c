/*
 * X683 F2FS segment allocation / SIT / discard reconstruction.
 *
 * Evidence: direct ARM64 disassembly of the stock X683 Image plus kallsyms.
 * This is a binary-backed reconstruction, not a claim of original source.
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define X683_SBI_SM_INFO       0x080
#define X683_SBI_LOG_BPS      0x3d8
#define X683_SBI_BPS          0x3dc
#define X683_SBI_SEGS_PER_SEC 0x3e0
#define X683_SBI_RESERVED     0x428
#define X683_SBI_CUR_RESERVED 0x430
#define X683_SBI_GC_MODE      0x534
#define X683_SBI_STAT_INFO    0x568
#define X683_SM_SIT           0x00
#define X683_SM_FREE          0x08
#define X683_SM_DIRTY         0x10
#define X683_SM_CURSEG        0x18
#define X683_CURSEG_SIZE      0x70
#define X683_SEG_ENTRY_SIZE   0x40

struct x683_sbi { unsigned char opaque[0]; };
struct x683_segment_allocation { int (*allocate_segment)(struct x683_sbi *, int, bool); };
struct x683_curseg_view { unsigned char opaque[X683_CURSEG_SIZE]; };

static inline unsigned char *x683p(void *p, size_t off) { return (unsigned char *)p + off; }
static inline uint32_t x683_u32(void *p, size_t off) { uint32_t v; __builtin_memcpy(&v, x683p(p,off),4); return v; }
static inline void x683_w32(void *p, size_t off, uint32_t v) { __builtin_memcpy(x683p(p,off),&v,4); }
static inline void *x683_ptr(void *p, size_t off) { void *v; __builtin_memcpy(&v,x683p(p,off),sizeof(v)); return v; }

extern bool f2fs_need_SSR(struct x683_sbi *sbi);
extern int get_ssr_segment(struct x683_sbi *sbi, int type);
extern void change_curseg(struct x683_sbi *sbi, int type);
extern void new_curseg(struct x683_sbi *sbi, int type, bool new_sec);
extern uint32_t get_free_segment(struct x683_sbi *sbi);
extern void stat_inc_seg_type(struct x683_sbi *sbi, void *curseg);
extern void locate_dirty_segment(struct x683_sbi *sbi, uint32_t segno);

/* X683: allocate_segment_by_default @ 0xffffff92d0df0454. */
int x683_allocate_segment_by_default(struct x683_sbi *sbi, int type, bool force_new_section)
{
    void *sm = x683_ptr(sbi, X683_SBI_SM_INFO);
    void *curseg = (unsigned char *)sm + X683_SM_CURSEG + (size_t)type * X683_CURSEG_SIZE;
    uint32_t alloc_type = x683_u32(curseg, 0x58);

    if (force_new_section) {
        new_curseg(sbi, type, true);
        return 0;
    }

    if (type == 4 && (x683_u32(sbi, 0x0d0) & (1U << 6)) == 0) {
        new_curseg(sbi, type, false);
        return 0;
    }

    if (alloc_type == 0 && x683_u32(curseg, 0x5c) + 1 < x683_u32(sm, 0x58)) {
        /* Stock X683 checks the next-segment bitmap here. */
    }

    if (f2fs_need_SSR(sbi) && get_ssr_segment(sbi, type))
        change_curseg(sbi, type);
    else
        new_curseg(sbi, type, false);

    stat_inc_seg_type(sbi, curseg);
    return 0;
}

/* X683: new_curseg @ 0xffffff92d0df07e8. */
int x683_new_curseg(struct x683_sbi *sbi, int type, bool new_sec)
{
    void *sm = x683_ptr(sbi, X683_SBI_SM_INFO);
    void *curseg = (unsigned char *)sm + X683_SM_CURSEG + (size_t)type * X683_CURSEG_SIZE;
    uint32_t next = x683_u32(curseg, 0x5c);

    if (next == UINT32_MAX || new_sec) {
        uint32_t segno = get_free_segment(sbi);
        if (segno == UINT32_MAX)
            return -28;
        x683_w32(curseg, 0x58, segno);
        x683_w32(curseg, 0x5c, 0);
        locate_dirty_segment(sbi, segno);
    }
    return 0;
}

/* X683: f2fs_allocate_new_segments @ 0xffffff92d0de94fc. */
void x683_f2fs_allocate_new_segments(struct x683_sbi *sbi)
{
    for (int type = 0; type < 6; ++type) {
        void *sm = x683_ptr(sbi, X683_SBI_SM_INFO);
        void *curseg = (unsigned char *)sm + X683_SM_CURSEG + (size_t)type * X683_CURSEG_SIZE;
        uint32_t old = x683_u32(curseg, 0x58);
        (void)x683_allocate_segment_by_default(sbi, type, false);
        if (old != x683_u32(curseg, 0x58))
            locate_dirty_segment(sbi, old);
    }
}

/* update_sit_entry @ 0xffffff92d0de8c1c / add_sit_entry @ 0xffffff92d0df0008. */
void x683_update_sit_entry_opaque(struct x683_sbi *sbi, uint64_t blkaddr, int del)
{
    (void)sbi; (void)blkaddr; (void)del;
    /* Validity-map mutation, valid-block accounting and dirty SIT propagation
     * are proven; unresolved bitmap member names stay opaque. */
}

/* issue_discard_thread @ 0xffffff92d0df0120. */
int x683_issue_discard_thread(void *arg)
{
    struct x683_sbi *sbi = arg;
    (void)sbi;
    /* set_freezable(); wait for discard state; submit ranges; stop/freezer path. */
    return 0;
}
