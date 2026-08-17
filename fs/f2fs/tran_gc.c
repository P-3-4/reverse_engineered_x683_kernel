/* SPDX-License-Identifier: GPL-2.0 */
/*
 * X683/H694 Transsion F2FS GC reconstruction.
 *
 * This is reconstructed behavior based on the stock 4.14.141+ binary.
 * It is not claimed to be verbatim proprietary source.
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include "f2fs.h"
#include "x683_layout.h"

/*
 * Exact X683 call ABI recovered from tran_do_f2fs_gc():
 *
 *     f2fs_gc(sbi, sync, background, segno)
 *
 * The stock wrapper passes:
 *     sync       = F2FS_MOUNT_FORCE_FG_GC (mount_opt bit 14)
 *     background = true
 *     segno      = NULL_SEGNO (-1)
 *
 * The fourth argument is therefore part of the X683/vendor F2FS ABI.
 */
extern int f2fs_gc(struct f2fs_sb_info *sbi, bool sync,
                   bool background, unsigned int segno);

/* Reconstructed Transsion GC mode selector. */
static int gc_type;

/*
 * Stock tran_do_f2fs_gc() behavior recovered from AArch64:
 *
 *   save sbi->gc_mode
 *
 *   if (gc_type == 0)
 *       f2fs_gc(sbi, mount_opt bit 14, true, NULL_SEGNO)
 *
 *   else if (gc_type == 2)
 *       sbi->gc_mode = 3;
 *       f2fs_gc(sbi, mount_opt bit 14, true, NULL_SEGNO)
 *       restore gc_mode
 *
 *   else
 *       sbi->gc_mode = 2;
 *       f2fs_gc(sbi, mount_opt bit 14, true, NULL_SEGNO)
 *       restore gc_mode
 *
 * The stock binary passes the saved gc_mode back after the GC call.
 */
int x683_tran_do_f2fs_gc(struct f2fs_sb_info *sbi)
{
        u32 old_state = x683_gc_mode(sbi);
        int ret;

        switch (gc_type) {
        case 0:
                return f2fs_gc(sbi, x683_gc_sync(sbi), true, NULL_SEGNO);
        case 2:
                x683_set_gc_mode(sbi, 3);
                break;
        default:
                x683_set_gc_mode(sbi, 2);
                break;
        }

        ret = f2fs_gc(sbi, x683_gc_sync(sbi), true, NULL_SEGNO);
        x683_set_gc_mode(sbi, old_state);
        return ret;
}

int x683_tran_gc_get_type(void)
{
        return gc_type;
}

void x683_tran_gc_set_type(int type)
{
        gc_type = type;
}
