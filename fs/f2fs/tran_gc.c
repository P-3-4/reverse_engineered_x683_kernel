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
 * Stock X683 call record: three arguments are observed at the f2fs_gc call:
 *   x0 = sbi, x1 = sync, x2 = true
 *
 * The semantic identity of the third parameter is vendor-specific and must
 * not be inferred from a newer public F2FS tree. Current reconstruction
 * treats it as the stock wrapper's force/override argument.
 */
extern int f2fs_gc(struct f2fs_sb_info *sbi, bool sync, bool force);

/* Reconstructed Transsion GC mode selector. */
static int gc_type;

/*
 * The X683 binary temporarily changes the stock F2FS GC state word:
 *   gc_type == 0 : no override
 *   gc_type == 2 : force state 3 during GC
 *   otherwise    : force state 2 during GC
 *
 * The state word is mapped to f2fs_sb_info.gc_mode by the X683-era layout
 * reconstruction, but remains offset-backed until the complete source tree
 * is matched and compiled against the same structure packing.
 */
int x683_tran_do_f2fs_gc(struct f2fs_sb_info *sbi)
{
        u32 old_state = x683_gc_mode(sbi);
        int ret;

        switch (gc_type) {
        case 0:
                return f2fs_gc(sbi, x683_gc_sync(sbi), true);
        case 2:
                x683_set_gc_mode(sbi, 3);
                break;
        default:
                x683_set_gc_mode(sbi, 2);
                break;
        }

        ret = f2fs_gc(sbi, x683_gc_sync(sbi), true);
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
