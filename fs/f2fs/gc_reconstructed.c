/*
 * X683/H694 F2FS GC reconstruction.
 * Reconstructed/inferred code, not recovered proprietary source.
 * Target ABI: stock X683 f2fs_gc(sbi, sync, background, segno).
 *
 * The control-flow skeleton follows the X683 binary and the matching
 * four-argument Android/common 4.14-era lineage. This is not a claim of
 * proprietary-source recovery or byte equivalence.
 */
#include "f2fs.h"
#include "segment.h"

static int x683_get_victim(struct f2fs_sb_info *sbi,
		unsigned int *victim, int gc_type)
{
	struct sit_info *sit_i = SIT_I(sbi);
	int ret;

	mutex_lock(&sit_i->sentry_lock);
	ret = DIRTY_I(sbi)->v_ops->get_victim(sbi, victim, gc_type,
			NO_CHECK_TYPE, LFS);
	mutex_unlock(&sit_i->sentry_lock);
	return ret;
}

/*
 * Returns the number of foreground-freed segments in the victim section.
 * A complete section is therefore represented by
 *
 *     seg_freed == sbi->segs_per_sec
 *
 * and the caller converts that to sec_freed.
 */
static int x683_do_garbage_collect(struct f2fs_sb_info *sbi,
		unsigned int start_segno, struct gc_inode_list *gc_list,
		int gc_type)
{
	struct page *sum_page;
	struct f2fs_summary_block *sum;
	struct blk_plug plug;
	unsigned int segno = start_segno;
	unsigned int end_segno = start_segno + sbi->segs_per_sec;
	unsigned char type = IS_DATASEG(get_seg_entry(sbi, segno)->type) ?
		SUM_TYPE_DATA : SUM_TYPE_NODE;
	int seg_freed = 0;

	if (sbi->segs_per_sec > 1)
		f2fs_ra_meta_pages(sbi, GET_SUM_BLOCK(sbi, segno),
				sbi->segs_per_sec, META_SSA, true);

	blk_start_plug(&plug);

	for (segno = start_segno; segno < end_segno; segno++) {
		sum_page = f2fs_get_sum_page(sbi, segno);
		if (IS_ERR(sum_page))
			continue;

		if (get_valid_blocks(sbi, segno, false) == 0 ||
				!PageUptodate(sum_page) || f2fs_cp_error(sbi)) {
			f2fs_put_page(sum_page, 0);
			continue;
		}

		sum = page_address(sum_page);
		if (type == SUM_TYPE_NODE)
			gc_node_segment(sbi, sum->entries, segno, gc_type);
		else
			gc_data_segment(sbi, sum->entries, gc_list, segno, gc_type);

		stat_inc_seg_count(sbi, type, gc_type);
		if (gc_type == FG_GC &&
				get_valid_blocks(sbi, segno, false) == 0)
			seg_freed++;

		f2fs_put_page(sum_page, 0);
	}

	if (gc_type == FG_GC)
		f2fs_submit_merged_write(sbi,
				(type == SUM_TYPE_NODE) ? NODE : DATA);

	blk_finish_plug(&plug);
	stat_inc_call_count(sbi->stat_info);

	return seg_freed;
}

int x683_f2fs_gc(struct f2fs_sb_info *sbi, bool sync, bool background,
		unsigned int requested_segno)
{
	unsigned int segno = requested_segno;
	unsigned int init_segno = requested_segno;
	int gc_type = sync ? FG_GC : BG_GC;
	int sec_freed = 0;
	int ret = -EINVAL;
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

	/* Critical-path balance calls must not perform BG GC. */
	if (gc_type == BG_GC && !background)
		goto stop;

	if (segno == NULL_SEGNO) {
		if (!x683_get_victim(sbi, &segno, gc_type)) {
			ret = -ENODATA;
			goto stop;
		}
	}

	{
		int seg_freed = x683_do_garbage_collect(sbi, segno,
				&gc_list, gc_type);
		if (gc_type == FG_GC && seg_freed == sbi->segs_per_sec)
			sec_freed++;
	}

	if (gc_type == FG_GC)
		sbi->cur_victim_sec = NULL_SEGNO;

	if (!sync) {
		if (has_not_enough_free_secs(sbi, sec_freed, 0)) {
			segno = NULL_SEGNO;
			goto gc_more;
		}

		if (gc_type == FG_GC)
			ret = write_checkpoint(sbi, &cpc);
	}

stop:
	/* The explicit-segment ABI keeps the initial segment for cursor cleanup. */
	if (SIT_I(sbi)) {
		SIT_I(sbi)->last_victim[ALLOC_NEXT] = 0;
		SIT_I(sbi)->last_victim[FLUSH_DEVICE] = init_segno;
	}

	put_gc_inode(&gc_list);

	if (sync)
		ret = sec_freed ? 0 : -EAGAIN;
	return ret;
}
