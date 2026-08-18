/*
 * X683/H694 Transsion GC threshold/fragmentation reconstruction.
 *
 * Reconstructed directly from the stock X683/H694 Image at:
 *   0x37b580 .. 0x37b748
 *
 * This is independent reconstructed source; it is not claimed to be the
 * original proprietary Transsion source.
 */

#include "f2fs.h"

#define X683_FP100 0x51EB851FULL
#define X683_INV_100 0xA3D70A3D70A3D70BULL

/* Image +0x4e4: 100/80/60 policy factors. */
static const u32 x683_gc_factor_table[8] = {
	100, 100, 100, 80, 80, 80, 60, 60
};

/* Image +0x4d4. Used while (user_segments >> 15) == 0. */
static const u32 x683_gc_scale_small[4] = {
	0x800, 0xc00, 0x1000, 0x1000
};

/* Image +0x504: 80/70/60 diagnostic scaling factors. */
static const u32 x683_gc_left_space_table[8] = {
	80, 80, 80, 70, 70, 70, 60, 60
};

static const u64 x683_gc_left_space_table64[8] = {
	80, 80, 80, 70, 70, 70, 60, 60
};

/*
 * Exact stock fixed-point threshold sequence:
 *     ((u64)factor * scale * 0x51EB851F) >> 37
 *
 * 0x51EB851F / 2^37 ~= 0.01.
 */
static inline u32 x683_threshold(u32 factor, u32 scale)
{
	u64 product = (u64)factor * (u64)scale;
	return (u32)((product * X683_FP100) >> 37);
}

/* Exact signed fixed-point sequence from 0x37b6b0..0x37b6c4. */
static inline s64 x683_scaled_left_space(s64 span, u64 factor)
{
	s64 p = (s64)factor * span;
	__int128 wide = (__int128)p * (__int128)X683_INV_100;
	s64 high = (s64)(wide >> 64);
	s64 q = (high + p) >> 6;
	q += (u64)p >> 63;
	return q;
}

/*
 * 0x37b580.
 *
 * The stock arithmetic is:
 *
 *   user_segments = user_block_count >> log_blocks_per_seg
 *   sit_segments  = sit_blocks >> log_blocks_per_seg
 *   span          = user_segments - sit_segments
 *   free_percent  = free_segments * 100 / span
 *   fragmentation = 100 - free_percent
 *
 * The result is logged with:
 *   "f2fs alloc new segment and fragmentation is %lu"
 *
 * The log helper's exact prototype is intentionally not reconstructed here.
 */
u32 x683_calc_fragmentation_percent(struct f2fs_sb_info *sbi)
{
	struct f2fs_sm_info *sm = SM_I(sbi);
	struct sit_info *sit = sm->sit_info;
	struct free_segmap_info *free_i = sm->free_info;
	u32 user_segments;
	u32 sit_blocks;
	u32 sit_segments;
	u32 span;
	u32 free_segments;

	user_segments = sbi->user_block_count >> sbi->log_blocks_per_seg;
	sit_blocks = *(u32 *)((char *)sit + 0x10);
	sit_segments = sit_blocks >> sbi->log_blocks_per_seg;
	span = user_segments - sit_segments;
	free_segments = *(u32 *)((char *)free_i + 0x04);

	if (!span)
		return 0;

	return 100u - (u32)(((u64)free_segments * 100u) / span);
}

/*
 * 0x37b5d4..0x37b748.
 *
 * The stock function returns 1 on either of the first two predicates below,
 * and otherwise continues into its diagnostic logging path before returning
 * 0. The function is called directly by the detector at 0x377104 and
 * 0x377de8.
 *
 * `selector` is the maximum of the two vendor byte globals loaded by stock at
 * Image + 0x1a13890 and +0x1a13894. The absolute globals are kept out of the
 * reconstructed C interface; callers pass the already-selected value.
 */
bool x683_gc_fragmentation_predicate(struct f2fs_sb_info *sbi, u32 selector,
					  u32 *out_threshold2,
					  u32 *out_fix_threshold,
					  u32 *out_percent)
{
	struct f2fs_sm_info *sm = SM_I(sbi);
	struct sit_info *sit = sm->sit_info;
	struct free_segmap_info *free_i = sm->free_info;
	u32 log_blocks_per_seg = sbi->log_blocks_per_seg;
	u32 user_segments = sbi->user_block_count >> log_blocks_per_seg;
	u32 sit_blocks = *(u32 *)((char *)sit + 0x10);
	u32 sit_segments = sit_blocks >> log_blocks_per_seg;
	u32 free_segments = *(u32 *)((char *)free_i + 0x04);
	u32 reserved_segments = sm->reserved_segments;
	u32 scale;
	u32 factor = (selector <= 7) ? x683_gc_factor_table[selector] : 0;
	s32 delta = (s32)(free_segments - reserved_segments);
	s64 span;
	s64 scaled;

	if ((user_segments >> 15) == 0)
		scale = x683_gc_scale_small[(user_segments >> 13) & 3u];
	else
		scale = 0x1800;

	if (out_threshold2)
		*out_threshold2 = x683_threshold(factor, scale);
	if (out_fix_threshold)
		*out_fix_threshold = x683_threshold(factor, scale);
	if (out_percent)
		*out_percent = (selector <= 7) ? x683_gc_left_space_table[selector] : 0;

	/* 0x37b668: Stop-2 predicate. */
	if (delta > (s32)x683_threshold(factor, scale))
		return true;

	/* 0x37b678..0x37b6c8: Stop-3 predicate. */
	span = (s64)user_segments - (s64)sit_segments;
	scaled = x683_scaled_left_space(
		span,
		(selector <= 7) ? x683_gc_left_space_table64[selector] : 0);
	if (scaled < (s64)delta)
		return true;

	/* 0x37b6d8..0x37b720 computes diagnostic threshold/percent values;
	 * the stock routine then logs them using the string at 0x10a5f47:
	 * "free_segment=%d, fix_size=%d, left_space=%d(precent:%d)".
	 */
	return false;
}
