/*
 * X683/H694 Transsion GC threshold reconstruction.
 *
 * Reconstructed from the verified stock Image. Not proprietary source.
 *
 * The two selector bytes are passed in by the caller because their absolute
 * image addresses are vendor-global storage, not fields in f2fs_sb_info.
 */

#include "f2fs.h"

#define X683_FP1PCT_NUM 0x51EB851FULL
#define X683_SIGNED_SCALE_NUM 0xA3D70A3D70A3D70BULL

static const u32 x683_factor_table[8] = {
	100, 100, 100, 80, 80, 80, 60, 60
};

static const u32 x683_small_scale[4] = {
	0x0800, 0x0c00, 0x1000, 0x1000
};

static const u64 x683_left_space_table[8] = {
	80, 80, 80, 70, 70, 70, 60, 60
};

static inline u32 x683_select_scale(u32 user_segments)
{
	if ((user_segments >> 15) == 0)
		return x683_small_scale[(user_segments >> 13) & 3U];
	return 0x1800;
}

static inline u32 x683_select_factor(u32 selector)
{
	return selector <= 7 ? x683_factor_table[selector] : 0;
}

/* Exact unsigned fixed-point threshold sequence used by Stop 2. */
static inline u32 x683_threshold(u32 factor, u32 scale)
{
	u64 product = (u64)factor * scale;
	product *= X683_FP1PCT_NUM;
	return (u32)(product >> 37);
}

/* Exact signed multiply/high-word sequence used by Stop 3. */
static inline s64 x683_scale_left_space(s64 span, u64 factor)
{
	__int128 product = (__int128)(s64)factor * span;
	s64 high = (s64)(product >> 64);
	s64 value = (high + (s64)product) >> 6;
	value += ((s64)product < 0);
	return value;
}

struct x683_gc_threshold_result {
	u32 scale;
	u32 factor;
	s32 free_minus_reserved;
	u32 threshold2;
	s64 span;
	s64 scaled_left_space;
	bool stop2;
	bool stop3;
};

/*
 * Reconstructs 0x37b5d4..0x37b748.
 *
 * selector = max(global_selector_a, global_selector_b)
 * selector > 7 disables the policy factor.
 */
bool x683_gc_threshold_eval(struct f2fs_sb_info *sbi,
				u8 global_selector_a,
				u8 global_selector_b,
				struct x683_gc_threshold_result *r)
{
	struct f2fs_sm_info *sm = SM_I(sbi);
	struct sit_info *sit = sm->sit_info;
	struct free_segmap_info *free_i = sm->free_info;
	u32 user_segments;
	u32 sit_blocks;
	u32 sit_segments;
	u32 free_segments;
	u32 selector;
	u32 factor;

	if (!r)
		return false;

	selector = max_t(u32, global_selector_a, global_selector_b);
	factor = x683_select_factor(selector);
	user_segments = sbi->user_block_count >> sbi->log_blocks_per_seg;
	sit_blocks = *(u32 *)((char *)sit + 0x10);
	sit_segments = sit_blocks >> sbi->log_blocks_per_seg;
	free_segments = *(u32 *)((char *)free_i + 0x04);

	r->scale = x683_select_scale(user_segments);
	r->factor = factor;
	r->free_minus_reserved =
		(s32)(free_segments - sm->reserved_segments);
	r->threshold2 = factor ?
		x683_threshold(factor, r->scale) : 0;
	r->span = (s64)user_segments - (s64)sit_segments;
	r->scaled_left_space = factor && selector <= 7 ?
		x683_scale_left_space(r->span,
				x683_left_space_table[selector]) : 0;

	r->stop2 = factor &&
		(r->free_minus_reserved > (s32)r->threshold2);
	r->stop3 = factor &&
		(r->scaled_left_space < (s64)r->free_minus_reserved);

	return r->stop2 || r->stop3;
}

/* 0x37b580 fragmentation percentage/log calculation. */
unsigned long x683_calc_fragmentation_percent(struct f2fs_sb_info *sbi)
{
	struct f2fs_sm_info *sm = SM_I(sbi);
	struct sit_info *sit = sm->sit_info;
	struct free_segmap_info *free_i = sm->free_info;
	u32 user_segments = sbi->user_block_count >> sbi->log_blocks_per_seg;
	u32 sit_blocks = *(u32 *)((char *)sit + 0x10);
	u32 sit_segments = sit_blocks >> sbi->log_blocks_per_seg;
	u32 free_segments = *(u32 *)((char *)free_i + 0x04);
	u32 span = user_segments - sit_segments;
	u32 free_percent;

	if (!span)
		return 0;

	free_percent = (u32)(((u64)free_segments * 100U) / span);
	return 100U - free_percent;
}
