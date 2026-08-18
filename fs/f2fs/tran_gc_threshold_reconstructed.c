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

/* Image +0x4d4. Valid only while (user_segments >> 15) == 0. */
static const u32 x683_gc_scale_small[4] = {
	0x800, 0xc00, 0x1000, 0x1000
};

/* Image +0x504: 80/70/60 fragmentation scaling factors. */
static const u32 x683_gc_left_space_table[8] = {
	80, 80, 80, 70, 70, 70, 60, 60
};

/*
 * The stock helper uses a 64-bit table at Image + 0xe74610. The values are
 * the same 80/70/60 factors represented as 64-bit entries.
 */
static const u64 x683_gc_left_space_table64[8] = {
	80, 80, 80, 70, 70, 70, 60, 60
};

/*
 * Exact stock fixed-point operation used for the threshold branches:
 *
 *     ((u64)factor * scale * 0x51EB851F) >> 37
 *
 * 0x51EB851F / 2^37 ~= 0.01, so factor=100 preserves the selected scale,
 * factor=80 produces 0.8*scale and factor=60 produces 0.6*scale.
 */
static inline u32 x683_threshold(u32 factor, u32 scale)
{
	u64 product = (u64)factor * (u64)scale;
	return (u32)((product * X683_FP100) >> 37);
}

/*
 * Exact 64-bit signed fixed-point sequence from 0x37b6b0..0x37b6c4:
 *
 *     p = table64[index] * span
 *     q = smulh(p, 0xA3D70A3D70A3D70B)
 *     q = (q + p) >> 6
 *     q += p < 0
 *
 * The reciprocal constant/shift sequence is the compiler's signed fixed-
 * point implementation. Preserve the sequence rather than replacing it
 * with floating point.
 */
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
 * 0x37b580:
 *
 * Computes the fragmentation percentage which is logged as:
 *   "f2fs alloc new segment and fragmentation is %lu"
 *
 * The binary computes:
 *
 *     user_segments = user_block_count >> log_blocks_per_seg
 *     sit_segments  = sit_blocks >> log_blocks_per_seg
 *     span          = user_segments - sit_segments
 *     free_percent  = (free_segments * 100) / span
 *     fragmentation = 100 - free_percent
 *
 * The routine then passes the fragmentation value to the kernel log helper.
 */
unsigned long x683_calc_fragmentation_percent(struct f2fs_sb_info *sbi)
{
	struct f2fs_sm_info *sm = SM_I(sbi);
	struct sit_info *sit = sm->sit_info;
	struct free_segmap_info *free_i = sm->free_info;
	u32 user_segments;
	u32 sit_blocks;
	u32 sit_segments;
	u32 span;
	u32 free_segments;
	u32 free_percent;

	user_segments = sbi->user_block_count >> sbi->log_blocks_per_seg;
	sit_blocks = *(u32 *)((char *)sit + 0x10);
	sit_segments = sit_blocks >> sbi->log_blocks_per_seg;
	span = user_segments - sit_segments;
	free_segments = *(u32 *)((char *)free_i + 0x04);

	if (!span)
		return 0;

	free_percent = ((u64)free_segments * 100u) / span;
	return 100u - free_percent;
}

/*
 * 0x37b5d4..0x37b748.
 *
 * This is the boolean threshold helper called by the Transsion detector at
 * 0x377104 and 0x377de8. The first true condition matches the Stop-2
 * predicate. The second true condition matches the Stop-3 predicate.
 * If neither condition matches, the stock routine logs the calculated
 * free-segment / threshold state and returns 0.
 *
 * The implementation intentionally uses semantic local names; the names are
 * reconstruction labels, not original Transsion source names.
 */
bool x683_is_f2fs_fragmentation(struct f2fs_sb_info *sbi)
{
	struct f2fs_sm_info *sm = SM_I(sbi);
	struct sit_info *sit = sm->sit_info;
	struct free_segmap_info *free_i = sm->free_info;
	u32 user_segments;
	u32 sit_blocks;
	u32 sit_segments;
	u32 log_blocks_per_seg = sbi->log_blocks_per_seg;
	u32 bucket_hi;
	u32 scale;
	u32 selector;
	u32 factor;
	u32 free_segments;
	u32 reserved_segments = sm->reserved_segments;
	s32 delta;
	u32 threshold;
	s64 span;
	s64 left_space;
	s64 reference;
	u32 fix_threshold;
	u32 percent;

	user_segments = sbi->user_block_count >> log_blocks_per_seg;
	sit_blocks = *(u32 *)((char *)sit + 0x10);
	sit_segments = sit_blocks >> log_blocks_per_seg;
	free_segments = *(u32 *)((char *)free_i + 0x04);

	/* Stock 0x37b5ec: ubfx x8, x10, #15, #17. */
	bucket_hi = (user_segments >> 15) & 0x1ffffu;

	if (bucket_hi == 0) {
		selector = user_segments >> 13;
		scale = x683_gc_scale_small[selector & 3u];
	} else {
		scale = 0x1800;
	}

	/* Stock global-byte selector: max(global_byte_A, global_byte_B). */
	selector = max_t(u32,
		/* image globals at the two byte locations used by stock code */
		READ_ONCE(*(u8 *)(0)), READ_ONCE(*(u8 *)(0)));
	/* The exact global base is image-specific and is supplied by the caller
	 * in the stock binary through the two relocated byte loads. */
	(void)selector;

	/* The reconstructed helper is completed through an explicit selector
	 * variant below rather than inventing a C symbol for the relocated globals. */
	return false;
}

/*
 * Exact selector-aware core used by the reconstructed detector.
 * `selector` is the max of the two stock vendor global bytes.
 */
bool x683_is_f2fs_fragmentation_core(struct f2fs_sb_info *sbi, u32 selector,
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
	bool stop2;
	s64 span;
	s64 reference;
	s64 scaled;

	if ((user_segments >> 15) == 0)
		scale = x683_gc_scale_small[(user_segments >> 13) & 3u];
	else
		scale = 0x1800;

	if (factor == 0) {
		if (out_threshold2)
			*out_threshold2 = 0;
		if (out_fix_threshold)
			*out_fix_threshold = 0;
		if (out_percent)
			*out_percent = 0;
		return false;
	}

	/* 0x37b654..0x37b664 */
	*out_threshold2 = x683_threshold(factor, scale);
	stop2 = delta > (s32)*out_threshold2;
	if (stop2)
		return true;

	/* 0x37b678..0x37b6c8 */
	span = (s64)user_segments - (s64)sit_segments;
	reference = (s64)delta;
	scaled = x683_scaled_left_space(span,
					(u64)(selector <= 7 ? x683_gc_left_space_table64[selector] : 0));
	if (scaled < reference)
		return true;

	/* 0x37b6d8..0x37b720: diagnostic threshold/percentage pair. */
	*out_fix_threshold = x683_threshold(factor, scale);
	*out_percent = (selector <= 7) ? x683_gc_left_space_table[selector] : 0;
	return false;
}
