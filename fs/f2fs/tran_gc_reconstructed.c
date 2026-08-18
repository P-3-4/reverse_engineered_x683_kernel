/*
 * X683/H694 Transsion GC wrapper reconstruction.
 * Inferred directly from stock boot.img AArch64 machine code.
 * Not proprietary source recovery.
 */

#include "f2fs.h"

/*
 * Direct stock evidence:
 *
 *   sbi + 0x534 = gc_mode
 *   sbi + 0x4b8  = mount_opt.opt
 *   f2fs_gc() target = stock function at the recovered GC entry point
 *
 * A separate Transsion controller value selects a temporary policy:
 *
 *   0 -> leave gc_mode unchanged (COST/default path)
 *   1 -> temporarily force GC_GREEDY (2)
 *   2 -> temporarily force GC_URGENT (3)
 *
 * The stock code saves the old gc_mode and restores it after f2fs_gc().
 */

#define X683_GC_NORMAL 0
#define X683_GC_GREEDY 2
#define X683_GC_URGENT 3

static int x683_tran_f2fs_gc(struct f2fs_sb_info *sbi,
		unsigned int tran_gc_mode)
{
	int old_gc_mode = sbi->gc_mode;
	int ret;

	switch (tran_gc_mode) {
	case 2:
		sbi->gc_mode = X683_GC_URGENT;
		ret = f2fs_gc(sbi,
				!!(sbi->mount_opt.opt & (1U << 14)), true);
		sbi->gc_mode = old_gc_mode;
		break;
	case 1:
		sbi->gc_mode = X683_GC_GREEDY;
		ret = f2fs_gc(sbi,
				!!(sbi->mount_opt.opt & (1U << 14)), true);
		sbi->gc_mode = old_gc_mode;
		break;
	default:
		ret = f2fs_gc(sbi,
				!!(sbi->mount_opt.opt & (1U << 14)), true);
		break;
	}

	return ret;
}

/*
 * Directly observed stock behavior at image offsets:
 *
 *   0x37add8: load [sbi + 0x534]
 *   0x37adfc: store 3 to [sbi + 0x534]
 *   0x37ae00: call f2fs_gc
 *   0x37ae04: restore saved gc_mode
 *
 * and:
 *
 *   0x37ae50: select alternate policy
 *   0x37ae68: store 2 to [sbi + 0x534]
 *   0x37ae6c: call f2fs_gc
 *   0x37ae7c: restore saved gc_mode
 *
 * The exact address of the Transsion controller field is intentionally not
 * exposed as a C structure member until its complete structure layout is
 * recovered. The pseudocode above captures the proven policy behavior only.
 */
