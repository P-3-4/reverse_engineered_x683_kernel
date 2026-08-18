/*
 * X683/H694 F2FS GC reconstruction.
 *
 * Reconstructed from stock call/field evidence and historical 4.14-era F2FS
 * control flow. This is not claimed to be original Transsion source.
 *
 * Target ABI:
 *     f2fs_gc(sbi, bool sync, bool background)
 *
 * The stock X683 call site supplies the third argument as true. Do not replace
 * this prototype with the later segno/force variants without matching a new
 * binary revision first.
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
 */

int x683_reconstructed_f2fs_gc(struct f2fs_sb_info *sbi, bool sync,
                               bool background)
{
    unsigned int segno = NULL_SEGNO;
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
     * Stock evidence and historical F2FS agree on this state machine:
     * validate the superblock, optionally checkpoint prefree segments,
     * promote BG_GC to FG_GC when free sections are insufficient, select a
     * victim, migrate it, and repeat BG_GC until enough space is available.
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

    return ret;
}

/*
 * Stock-facing ABI adapter.
 *
 * Keep the public name separate until the exact reconstructed 4.14 tree is
 * integrated. The adapter makes the ABI boundary explicit and prevents an
 * accidental call to the later four/five-argument f2fs_gc().
 */
int x683_f2fs_gc(struct f2fs_sb_info *sbi, bool sync, bool background)
{
    return x683_reconstructed_f2fs_gc(sbi, sync, background);
}
