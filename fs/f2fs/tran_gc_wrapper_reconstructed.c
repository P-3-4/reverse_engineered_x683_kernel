/*
 * X683/H694 Transsion GC wrapper.
 * Reconstructed from stock 0x37ada8..0x37af00.
 */
#include "f2fs.h"

#define X683_GC_NORMAL 0
#define X683_GC_GREEDY 1
#define X683_GC_URGENT 2

extern int x683_f2fs_gc(struct f2fs_sb_info *sbi, bool sync,
			bool background, unsigned int segno);

/*
 * controller is the recovered +0x998 state:
 *   0 = normal
 *   1 = temporary gc_mode 2 / GREEDY
 *   2 = temporary gc_mode 3 / URGENT
 *
 * The stock wrapper restores sbi->gc_mode after the forced call.
 */
int tran_f2fs_gc_reconstructed(struct f2fs_sb_info *sbi, bool sync,
				       bool background, u32 controller)
{
	u32 old_gc_mode;
	int ret;

	if (controller == X683_GC_NORMAL)
		return x683_f2fs_gc(sbi, sync, background, NULL_SEGNO);

	old_gc_mode = sbi->gc_mode;

	if (controller == X683_GC_GREEDY)
		sbi->gc_mode = GC_GREEDY;
	else if (controller == X683_GC_URGENT)
		sbi->gc_mode = GC_URGENT;
	else
		return -EINVAL;

	ret = x683_f2fs_gc(sbi, sync, background, NULL_SEGNO);
	sbi->gc_mode = old_gc_mode;
	return ret;
}
