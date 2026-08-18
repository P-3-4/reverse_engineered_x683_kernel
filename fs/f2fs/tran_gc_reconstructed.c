/*
 * X683/H694 Transsion GC wrapper reconstruction.
 * Inferred directly from stock boot.img AArch64 machine code.
 * Not proprietary source recovery.
 *
 * Stock ABI: f2fs_gc(sbi, sync, background, segno).
 */

#include "f2fs.h"

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
				!!(sbi->mount_opt.opt & (1U << 14)), true,
				NULL_SEGNO);
		sbi->gc_mode = old_gc_mode;
		break;
	case 1:
		sbi->gc_mode = X683_GC_GREEDY;
		ret = f2fs_gc(sbi,
				!!(sbi->mount_opt.opt & (1U << 14)), true,
				NULL_SEGNO);
		sbi->gc_mode = old_gc_mode;
		break;
	default:
		ret = f2fs_gc(sbi,
				!!(sbi->mount_opt.opt & (1U << 14)), true,
				NULL_SEGNO);
		break;
	}

	return ret;
}

/*
 * Direct stock behavior:
 *
 *   controller 0 -> normal f2fs_gc()
 *   controller 1 -> temporary gc_mode 2 (GREEDY)
 *   controller 2 -> temporary gc_mode 3 (URGENT)
 *
 * The previous gc_mode is restored after the vendor-forced call.
 */
