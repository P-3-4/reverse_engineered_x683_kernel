/*
 * X683/H694 F2FS GC reconstruction.
 *
 * Reconstructed from stock call/field evidence and historical 4.14-era F2FS
 * control flow. This is not claimed to be original Transsion source.
 *
 * Target X683 ABI:
 *     f2fs_gc(sbi, bool sync, bool background, unsigned int segno)
 *
 * The stock Transsion caller supplies:
 *     sync       = (sbi->mount_opt.opt >> 14) & 1
 *     background = true
 *     segno      = NULL_SEGNO
 *
 * Do not replace this prototype with the older three-argument form.
 */

#include "f2fs.h"
#include "segment.h"
#include "gc.h"

/*
 * The helper implementations below are expected from the matching 4.14 F2FS
 * tree: __get_victim(), do_garbage_collect(), put_gc_inode(),
 * has_not_enough_free_secs(), prefree_segments(), free_segments(),
 * reserved_segments(), free_sections(), f2fs_write_checkpoint(), and the
 * segment-victim bookkeeping helpers.
 *
 * The victim-selection and migration phase is documented separately in:
 *
 * docs/reverse-engineering/
 *   x683-f2fs-victim-selection-migration-delta-2026-08-19.md
 */

int x683_reconstructed_f2fs_gc(struct f2fs_sb_info *sbi, bool sync,
                               bool background, unsigned int segno_hint)
{
    unsigned int segno = segno_hint;
    int gc_type = sync ? FG_GC : BG_GC;
    int sec_freed = 0;
    int seg_freed = 0;
    int total_freed = 0;
    int ret = -EINVAL;
    struct cp_control cpc;
    struct gc_inode_list gc_list = {
        .ilist = LIST_HEAD_INIT(gc_list.ilist),
        .iroot = RADIX_TREE_INIT(GFP_NOFS),
    };

    cpc.reason = __get_cp_reason(sbi);

    /*
     * This is a historical-X683 reconstruction of the stock GC state
     * machine. Transsion policy enters this core through tran_do_f2fs_gc(),
     * which may temporarily change sbi->gc_mode before this function runs.
     */
    for (;;) {
        if (unlikely(!(sbi->sb->s_flags & SB_ACTIVE))) {
            ret = -EINVAL;
            break;
        }

        if (unlikely(f2fs_cp_error(sbi))) {
            ret = -EIO;
            break;
        }

        if (gc_type == BG_GC && has_not_enough_free_secs(sbi, 0, 0)) {
            if (prefree_segments(sbi)) {
                ret = f2fs_write_checkpoint(sbi, &cpc);
                if (ret)
                    break;
            }

            if (has_not_enough_free_secs(sbi, 0, 0))
                gc_type = FG_GC;
        }

        /* f2fs_balance_fs() must not perform BG_GC in its critical path. */
        if (gc_type == BG_GC && !background) {
            ret = -EINVAL;
            break;
        }

        if (!__get_victim(sbi, &segno, gc_type)) {
            ret = -ENODATA;
            break;
        }

        seg_freed = do_garbage_collect(sbi, segno, &gc_list, gc_type);

        if (gc_type == FG_GC && seg_freed == sbi->segs_per_sec)
            sec_freed++;

        total_freed += seg_freed;

        if (gc_type == FG_GC)
            sbi->cur_victim_sec = NULL_SEGNO;

        if (!sync) {
            if (has_not_enough_free_secs(sbi, sec_freed, 0)) {
                segno = NULL_SEGNO;
                continue;
            }

            if (gc_type == FG_GC)
                ret = f2fs_write_checkpoint(sbi, &cpc);
        }

        break;
    }

    SIT_I(sbi)->last_victim[ALLOC_NEXT] = 0;

    put_gc_inode(&gc_list);

    /* Foreground GC succeeds only when a complete section was freed. */
    if (sync)
        ret = sec_freed ? 0 : -EAGAIN;

    (void)total_freed;
    return ret;
}

/*
 * Stock-facing four-argument X683 adapter.
 * Keep the reconstructed symbol separate until the exact 4.14 source tree
 * is integrated, but preserve the ABI proven by the stock X683 binary.
 */
int x683_f2fs_gc(struct f2fs_sb_info *sbi, bool sync, bool background,
                 unsigned int segno)
{
    return x683_reconstructed_f2fs_gc(sbi, sync, background, segno);
}
