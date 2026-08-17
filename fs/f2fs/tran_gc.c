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

/* Stock F2FS entry point observed in the X683 binary. */
extern int f2fs_gc(struct f2fs_sb_info *sbi, bool sync, bool force);

/* Reconstructed Transsion GC mode selector. */
static int gc_type;

/*
 * The X683 binary temporarily changes the stock F2FS GC state word:
 *   gc_type == 0 : no override
 *   gc_type == 2 : force state 3 during GC
 *   otherwise    : force state 2 during GC
 */
int x683_tran_do_f2fs_gc(struct f2fs_sb_info *sbi)
{
        u32 old_state = x683_gc_state(sbi);
        int ret;

        switch (gc_type) {
        case 0:
                return f2fs_gc(sbi, x683_gc_sync(sbi), true);
        case 2:
                x683_sbi_write_u32(sbi, X683_SBI_OFF_GC_STATE, 3);
                break;
        default:
                x683_sbi_write_u32(sbi, X683_SBI_OFF_GC_STATE, 2);
                break;
        }

        ret = f2fs_gc(sbi, x683_gc_sync(sbi), true);
        x683_sbi_write_u32(sbi, X683_SBI_OFF_GC_STATE, old_state);
        return ret;
}

/*
 * These symbols are intentionally kept separate from the final vendor
 * implementation until the exact MT6768/X683 F2FS source revision is matched.
 */
int x683_tran_gc_get_type(void)
{
        return gc_type;
}

void x683_tran_gc_set_type(int type)
{
        gc_type = type;
}
