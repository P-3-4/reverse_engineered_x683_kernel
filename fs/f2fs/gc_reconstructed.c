/*
 * X683/H694 F2FS GC reconstruction.
 *
 * This is reconstructed/inferred code, not recovered proprietary source.
 * Target ABI: 4.14-era f2fs_gc(sbi, sync, background).
 */

#include "f2fs.h"
#include "segment.h"

/*
 * Historical 4.15 evidence pins the GC wrapper to a write acquisition of
 * SIT_I(sbi)->sentry_lock before dispatching into DIRTY_I()->v_ops.
 * The vendor binary must still be checked for any local locking change.
 */
static int x683_get_victim(struct f2fs_sb_info *sbi,
		unsigned int *victim, int gc_type)
{
	struct sit_info *sit_i = SIT_I(sbi);
	int ret;

	down_write(&sit_i->sentry_lock);
	ret = DIRTY_I(sbi)->v_ops->get_victim(sbi, victim,
			gc_type, NO_CHECK_TYPE, LFS);
	up_write(&sit_i->sentry_lock);

	return ret;
}

static int x683_do_garbage_collect(struct f2fs_sb_info *sbi,
		unsigned int start_segno, struct gc_inode_list *gc_list,
		int gc_type)
{
	struct page *sum_page;
	struct f2fs_summary_block *sum;
	struct blk_plug plug;
	unsigned int segno = start_segno;
	unsigned int end_segno = start_segno + sbi->segs_per_sec;
	int seg_freed = 0;
	unsigned char type;

	type = IS_DATASEG(get_seg_entry(sbi, segno)->type) ?
		SUM_TYPE_DATA : SUM_TYPE_NODE;

	if (sbi->segs_per_sec > 1)
		f2fs_ra_meta_pages(sbi, GET_SUM_BLOCK(sbi, segno),
				sbi->segs_per_sec, META_SSA, true);

	blk_start_plug(&plug);

	for (segno = start_segno; segno < end_segno; segno++) {
		sum_page = f2fs_get_sum_page(sbi, segno);
		if (IS_ERR(sum_page))
			continue;

		unlock_page(sum_page);
		sum = page_address(sum_page);

		switch (GET_SUM_TYPE(&sum->footer)) {
		case SUM_TYPE_NODE:
			gc_node_segment(sbi, sum->entries, segno, gc_type);
			break;
		case SUM_TYPE_DATA:
			gc_data_segment(sbi, sum->entries, gc_list, segno, gc_type);
			break;
		default:
			break;
		}

		stat_inc_seg_count(sbi, GET_SUM_TYPE(&sum->footer));
		f2fs_put_page(sum_page, 0);
	}

	if (gc_type == FG_GC)
		f2fs_submit_merged_write(sbi,
				(type == SUM_TYPE_NODE) ? NODE : DATA);

	blk_finish_plug(&plug);

	if (gc_type == FG_GC &&
			get_valid_blocks(sbi, start_segno, sbi->segs_per_sec) == 0)
		seg_freed = sbi->segs_per_sec;

	return seg_freed;
}

int x683_f2fs_gc(struct f2fs_sb_info *sbi, bool sync, bool background)
{
	unsigned int segno;
	int gc_type = sync ? FG_GC : BG_GC;
	int sec_freed = 0;
	int ret = -EINVAL;
	int seg_freed;
	struct cp_control cpc;
	struct gc_inode_list gc_list = {
		.ilist = LIST_HEAD_INIT(gc_list.ilist),
		.iroot = RADIX_TREE_INIT(gc_list.iroot, GFP_NOFS),
	};

	cpc.reason = __get_cp_reason(sbi);

 gc_more:
	if (unlikely(!(sbi->sb->s_flags & MS_ACTIVE)))
		goto stop;
	if (unlikely(f2fs_cp_error(sbi))) {
		ret = -EIO;
		goto stop;
	}

	if (gc_type == BG_GC && has_not_enough_free_secs(sbi, 0, 0)) {
		if (prefree_segments(sbi)) {
			ret = write_checkpoint(sbi, &cpc);
			if (ret)
				goto stop;
		}
		if (has_not_enough_free_secs(sbi, 0, 0))
			gc_type = FG_GC;
	}

	if (gc_type == BG_GC && !background)
		goto stop;

	if (!x683_get_victim(sbi, &segno, gc_type))
		goto stop;

	seg_freed = x683_do_garbage_collect(sbi, segno, &gc_list, gc_type);
	if (gc_type == FG_GC && seg_freed == sbi->segs_per_sec)
		sec_freed++;

	if (gc_type == FG_GC)
		sbi->cur_victim_sec = NULL_SEGNO;

	if (!sync) {
		if (has_not_enough_free_secs(sbi, sec_freed, 0))
			goto gc_more;
		if (gc_type == FG_GC)
			ret = write_checkpoint(sbi, &cpc);
	}

stop:
	/* Historical 4.15-era GC resets these search cursors on exit. */
	SIT_I(sbi)->last_victim[ALLOC_NEXT] = 0;
	SIT_I(sbi)->last_victim[FLUSH_DEVICE] = 0;
	mutex_unlock(&sbi->gc_mutex);
	put_gc_inode(&gc_list);

	if (sync)
		ret = sec_freed ? 0 : -EAGAIN;
	return ret;
}

/*
 * Stock-specific items still requiring binary confirmation:
 *   - Transsion trigger predicates around f2fs_gc().
 *   - Exact X683 gc_node_segment()/gc_data_segment() revision.
 *   - Exact structure packing surrounding sbi->gc_mode at 0x534.
 */
