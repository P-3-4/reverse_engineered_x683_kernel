/*
 * X683/H694 Transsion GC thread reconstruction.
 *
 * Reconstructed/inferred directly from stock boot.img AArch64 evidence.
 * NOT recovered proprietary Transsion source.
 *
 * This revision pins the register producers used by the Stop-1..5 detector.
 */

#include "f2fs.h"

#define TRAN_GC_CTRL_NORMAL 0
#define TRAN_GC_CTRL_GREEDY 1
#define TRAN_GC_CTRL_URGENT 2

struct x683_tran_gc_state {
	u64 cycle;                    /* +0x990 */
	u32 controller;               /* +0x998 */
	u8 controller_write_blocked;  /* +0x9c0 */
	u32 loop_state;               /* +0x9d0 */
	u32 detector_state;           /* +0x9d4 */
	u64 repeated_detector_count; /* +0x9d8 */
	u64 detector_cycles;          /* +0x9e0 */
	u32 running_max;              /* +0x9f0 */
	u32 saved_baseline;           /* +0x9f4 */
	u8 stop_result;               /* +0x9f8 */
	u32 stop_condition;           /* +0x9fc */
	u8 cadence_selector;          /* +0xa04 */
	u8 loop_active;               /* +0xa05 */
	u8 detector_enabled;          /* +0xa06 */
	s32 baseline_seg;              /* +0xa08 */
	u32 baseline_written_seg;      /* +0xa0c */
};

struct x683_tran_gc_inputs {
	u32 user_block_count;       /* sbi + 0x408 */
	u32 log_blocks_per_seg;     /* sbi + 0x3d8 */
	u32 main_segments;          /* sm_info + 0x5c */
	u32 reserved_segments;      /* sm_info + 0x60 */
	u32 sit_blocks;              /* SIT_I(sbi) + 0x10 */
	u32 free_segments;           /* FREE_I(sbi) + 0x04 */
	u32 prefree_segments;        /* DIRTY_I(sbi) + 0x84 */
	u32 dirty_type_sum;          /* DIRTY_I(sbi) + 0x68..0x7c */
	u32 user_segments;           /* user_block_count >> log_blocks_per_seg */
	u32 sit_segments;            /* sit_blocks >> log_blocks_per_seg */
	u32 capacity_bucket;         /* (user_segments >> 13) & 0x7ffff */
};

static void x683_collect_inputs(struct f2fs_sb_info *sbi,
		struct x683_tran_gc_inputs *in)
{
	struct f2fs_sm_info *sm = SM_I(sbi);
	struct sit_info *sit = sm->sit_info;
	struct free_segmap_info *free_i = sm->free_info;
	struct dirty_seglist_info *dirty_i = sm->dirty_info;
	unsigned int i;

	in->user_block_count = sbi->user_block_count;
	in->log_blocks_per_seg = sbi->log_blocks_per_seg;
	in->main_segments = sm->main_segments;
	in->reserved_segments = sm->reserved_segments;
	in->sit_blocks = *(u32 *)((char *)sit + 0x10);
	in->free_segments = free_i->free_segments;

	in->dirty_type_sum = 0;
	for (i = 0; i < 6; i++)
		in->dirty_type_sum += *(u32 *)((char *)dirty_i + 0x68 + i * 4);

	in->prefree_segments = *(u32 *)((char *)dirty_i + 0x84);
	in->user_segments = in->user_block_count >> in->log_blocks_per_seg;
	in->sit_segments = in->sit_blocks >> in->log_blocks_per_seg;
	in->capacity_bucket = (in->user_segments >> 13) & 0x7ffff;
}

/*
 * Direct stock mapping around 0x3775d4:
 *
 *   x25 = user_segments
 *   x20 = sit_segments
 *   x21 = capacity_bucket initially, then a selected threshold base
 *   w23 = free_segments + prefree_segments
 */
static void x683_prepare_detector_values(const struct x683_tran_gc_inputs *in,
		u32 *threshold_base, u32 *threshold_scale,
		u32 *recoverable_segments)
{
	u32 base;
	u32 scale;

	switch (in->capacity_bucket) {
	case 0:
		base = 0x800;
		scale = 2 * 512;
		break;
	case 1:
		base = 0xc00;
		scale = 3 * 512;
		break;
	case 2:
	case 3:
		base = 0x1000;
		scale = 2 * 512;
		break;
	default:
		base = 0x1800;
		scale = 4 * 512;
		break;
	}

	*threshold_base = base;
	*threshold_scale = scale;
	*recoverable_segments = in->free_segments + in->prefree_segments;
}

/*
 * Stop 1, exact register-level predicate at 0x3776a4..0x377724:
 *
 *   delta1 = recoverable_segments - controller->saved_baseline (+0x9f4)
 *   factor = table[capacity_bucket], table @ image +0x10a64e4
 *   threshold1 = factor * selected_scale * 0x51EB851F / 2^37
 *
 * table[0..7] = {100,100,100,80,80,80,60,60}
 */
static bool x683_stop1(struct x683_tran_gc_state *st,
			const struct x683_tran_gc_inputs *in,
			u32 selected_scale)
{
	static const u32 factor[8] = {100, 100, 100, 80, 80, 80, 60, 60};
	u32 idx = in->capacity_bucket;
	u32 delta;
	u64 threshold;

	if (idx > 7)
		return false;

	delta = in->free_segments + in->prefree_segments - st->saved_baseline;
	threshold = (u64)factor[idx] * selected_scale * 0x51EB851FULL;
	threshold >>= 37;

	if ((s32)delta > (s32)threshold) {
		st->stop_condition = 1;
		return true;
	}
	return false;
}

/*
 * Stop 2, exact predicate at 0x377728..0x377770:
 *
 *   delta2 = recoverable_segments - sbi->sm_info->reserved_segments
 *   threshold2 = factor[bucket] * threshold_base * 0x51EB851F / 2^37
 *
 * The multiply is unsigned in stock code.
 */
static bool x683_stop2(struct x683_tran_gc_state *st,
			const struct x683_tran_gc_inputs *in,
			u32 threshold_base)
{
	static const u32 factor[8] = {100, 100, 100, 80, 80, 80, 60, 60};
	u32 idx = in->capacity_bucket;
	u32 delta;
	u64 threshold;

	if (idx > 7)
		return false;

	delta = in->free_segments + in->prefree_segments - in->reserved_segments;
	threshold = (u64)factor[idx] * threshold_base * 0x51EB851FULL;
	threshold >>= 37;

	if ((s32)delta > (s32)threshold) {
		st->stop_condition = 2;
		return true;
	}
	return false;
}

/*
 * Stop 3, exact arithmetic shape at 0x37777c..0x3777d0.
 *
 *   x = table64[bucket] * (user_segments - sit_segments)
 *   scaled = signed_fixed_point(x, 0xA3D70A3D70A3D70B, >>6)
 *   compare scaled < (recoverable_segments - reserved_segments)
 *
 * table64[0..7] = {80,80,80,70,70,70,60,60}
 */
static bool x683_stop3(struct x683_tran_gc_state *st,
			const struct x683_tran_gc_inputs *in)
{
	static const u64 factor[8] = {80, 80, 80, 70, 70, 70, 60, 60};
	static const s64 magic = (s64)0xA3D70A3D70A3D70BULL;
	u32 idx = in->capacity_bucket;
	s64 span;
	s64 prod;
	s64 high;
	s64 scaled;
	s64 reference;

	if (idx > 7)
		return false;

	span = (s64)(u64)(in->user_segments - in->sit_segments);
	prod = (s64)factor[idx] * span;
	high = (s64)((__int128)prod * magic >> 64);
	scaled = (high + prod) >> 6;
	scaled += (prod < 0);
	reference = (s64)(s32)((in->free_segments + in->prefree_segments) -
			in->reserved_segments);

	if (scaled < reference) {
		st->stop_condition = 3;
		return true;
	}
	return false;
}

/* Stop 4 remains the proven SSR trigger. */
static bool x683_stop4(struct x683_tran_gc_state *st,
			s64 delta, s64 threshold)
{
	if (delta > threshold) {
		if (!st->controller_write_blocked)
			st->controller = TRAN_GC_CTRL_URGENT;
		st->stop_result = 1;
		return true;
	}
	return false;
}

/*
 * Stop 5 exact predicate from 0x377830..0x377878.
 * The current component x20 is the SIT-segment baseline; w23 is the
 * recoverable-segment count (free + prefree).
 */
static bool x683_stop5(struct x683_tran_gc_state *st,
			s64 current_sit_segments,
			u32 current_recoverable)
{
	u64 interval = st->cadence_selector ? 500 : 50;
	s64 progress;
	s64 baseline = st->baseline_seg;

	if ((st->cycle % interval) != 0)
		return false;

	progress = current_sit_segments +
		(s64)(s32)(current_recoverable - st->baseline_written_seg);

	if (progress > baseline)
		return false;

	if (!st->controller_write_blocked)
		st->controller = TRAN_GC_CTRL_URGENT;
	st->stop_result = 2;
	return true;
}
