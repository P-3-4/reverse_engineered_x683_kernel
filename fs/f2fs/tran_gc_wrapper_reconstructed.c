/*
 * X683/H694 Transsion GC wrapper.
 * Reconstructed directly from stock 0x37ada8..0x37ae94.
 * Binary-derived; not proprietary source recovery.
 */
#include "f2fs.h"

#define X683_GC_CONTROLLER_NORMAL 0
#define X683_GC_CONTROLLER_URGENT 1
#define X683_GC_CONTROLLER_GREEDY 2

#define X683_VENDOR_GC_MODE_COST   1
#define X683_VENDOR_GC_MODE_URGENT 2
#define X683_VENDOR_GC_MODE_GREEDY 3

extern int x683_f2fs_gc(struct f2fs_sb_info *sbi, bool sync,
			bool background, unsigned int segno);

/*
 * Exact wrapper behavior at 0x37ada8:
 *
 *   controller 0: call f2fs_gc() with the current sbi->gc_mode unchanged.
 *   controller 1: save gc_mode, force vendor mode 2, call f2fs_gc(), restore.
 *   controller 2: save gc_mode, force vendor mode 3, call f2fs_gc(), restore.
 *
 * The first f2fs_gc() argument is the FORCE_FG_GC mount option (mount_opt
 * bit 14 in the matching 4.14 source); background is always true; segno is
 * NULL_SEGNO (-1).
 */
int tran_f2fs_gc_reconstructed(struct f2fs_sb_info *sbi)
{
	u32 controller;
	u32 old_gc_mode;
	u32 force_fg_gc;
	int ret;

	controller = x683_vendor_gc_controller; /* image +0x1a13998 */
	force_fg_gc = (sbi->mount_opt.opt >> 14) & 1U;

	if (controller == X683_GC_CONTROLLER_NORMAL)
		return x683_f2fs_gc(sbi, force_fg_gc, true, NULL_SEGNO);

	old_gc_mode = sbi->gc_mode;

	if (controller == X683_GC_CONTROLLER_GREEDY)
		sbi->gc_mode = X683_VENDOR_GC_MODE_GREEDY;
	else if (controller == X683_GC_CONTROLLER_URGENT)
		sbi->gc_mode = X683_VENDOR_GC_MODE_URGENT;
	else
		return -EINVAL;

	ret = x683_f2fs_gc(sbi, force_fg_gc, true, NULL_SEGNO);
	sbi->gc_mode = old_gc_mode;
	return ret;
}
