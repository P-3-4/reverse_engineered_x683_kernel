/*
 * X683/H694 F2FS GC reconstruction.
 *
 * This is reconstructed/inferred code, not recovered proprietary source.
 * Target ABI: 4.14-era f2fs_gc(sbi, sync, background).
 *
 * The surrounding tree must supply the normal F2FS types/macros/helpers.
 */

#include "f2fs.h"
#include "segment.h"

/*
 * Reconstructed from the 4.15-era F2FS implementation and matched against
 * the X683 binary's recovered manager relationships.
 */
static int x683_get_victim(struct f2fs_sb_info *sbi,
		unsigned int *victim, int gc_type)
{
	struct sit_info *sit_i = SIT_I(sbi);
	int ret;

	/* Exact primitive (mutex vs rwsem) remains revision-sensitive. */
	mutex_lock(&sit_i->sentry_lock);
	ret = DIRTY_I(sbi)->v_ops->get_victim(sbi, victim,
			gc_type, NO_CHECK_TYPE, LFS);
	mutex_unlock(&sit_i->sentry_lock);

	return ret;
}

/*
 * Summary/migration dispatcher.
 *
 * The return value is deliberately a section-freed count rather than a
 * guessed vendor-specific status. The caller can convert it to sec_freed.
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
	int seg_freed = 0;
	unsigned char type;

	type = IS_DATASEG(get_seg_entry(sbi, segno)->type) ?
		SUM_TYPE_DATA : SUM_TYPE_NODE;

	/* X683 is reconstructed against the ordinary 4.14/4.15 section path. */
	if (sbi->segs_per_sec > 1)
		f2fs_ra_meta_pages(sbi, GET_SUM_BLOCK(sbi, segno),
				sbi->segs_per_sec, META_SSA, true);

	/* Reference summary pages before migration. */
	while (segno < end_segno) {
		sum_page = f2fs_get_sum_page(sbi, segno++);
		if (IS_ERR(sum_page))
			continue;
		unlock_page(sum_page);
	}

	blk_start_plug(&plug);

	for (segno = start_segno; segno < end_segno; segno++) {
		sum_page = f2fs_get_sum_page(sbi, segno);
		if (IS_ERR(sum_page))
			continue;

		/* Avoid the summary-page/sentry-lock deadlock documented by F2FS. */
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

	stat_inc_call_count(sbi->stat_info);
	return seg_freed;
}

/*
 * X683 three-argument GC core reconstruction.
 *
 * This intentionally does not import the later force/victim-segment API.
 * Vendor trigger predicates remain outside this core until stock call-site
 * evidence identifies them.
 */
int x683_f2fs_gc(struct f2fs_sb_info *sbi, bool sync, bool background)
{
	unsigned int segno;
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

	/* f2fs_balance_fs() does not perform BG_GC on its critical path. */
	if (gc_type == BG_GC && !background)
		goto stop;

	if (!x683_get_victim(sbi, &segno, gc_type))
		goto stop;

	if (x683_do_garbage_collect(sbi, segno, &gc_list, gc_type) &&
			gc_type == FG_GC)
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
	mutex_unlock(&sbi->gc_mutex);
	put_gc_inode(&gc_list);

	if (sync)
		ret = sec_freed ? 0 : -EAGAIN;
	return ret;
}

/*
 * Unresolved stock-specific items intentionally left visible:
 *
 * 1. Exact X683 dirty-manager cost function / candidate scoring.
 * 2. Exact sentry_lock primitive in the vendor revision.
 * 3. Exact gc_node_segment()/gc_data_segment() revision.
 * 4. Transsion GC trigger predicates.
 * 5. Exact structure packing around sbi->gc_mode at 0x534.
 */
