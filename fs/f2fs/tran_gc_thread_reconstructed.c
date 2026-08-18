/*
 * X683/H694 Transsion GC thread reconstruction.
 *
 * Reconstructed/inferred directly from stock boot.img AArch64 evidence.
 * NOT recovered proprietary Transsion source.
 */

#include "f2fs.h"

#define TRAN_GC_CTRL_NORMAL 0
#define TRAN_GC_CTRL_GREEDY 1
#define TRAN_GC_CTRL_URGENT 2

struct x683_tran_gc_state {
	u64 cycle;
	u32 controller;
	u8  controller_write_blocked;
	u32 loop_state;
	u32 detector_state;
	u64 repeated_detector_count;
	u64 detector_cycles;
	u32 running_max;
	u32 saved_baseline;
	u8  stop_result;
	u32 stop_condition;
	u8  cadence_selector;
	u8  loop_active;
	u8  detector_enabled;
	s32 baseline_seg;
	u32 baseline_written_seg;
};

static void x683_set_controller(struct x683_tran_gc_state *st, u32 value)
{
	if (!st->controller_write_blocked)
		st->controller = value;
}

static bool x683_stop1(struct x683_tran_gc_state *st,
		       s32 delta_seg, s32 threshold)
{
	if (delta_seg > threshold) {
		st->stop_condition = 1;
		return true;
	}
	return false;
}

static bool x683_stop2(struct x683_tran_gc_state *st,
		       s32 delta, s32 threshold)
{
	if (delta > threshold) {
		st->stop_condition = 2;
		return true;
	}
	return false;
}

static bool x683_stop3(struct x683_tran_gc_state *st,
		       s64 scaled_movement, s64 segment_reference)
{
	if (scaled_movement < segment_reference) {
		st->stop_condition = 3;
		return true;
	}
	return false;
}

static bool x683_stop4(struct x683_tran_gc_state *st,
		       s64 delta, s64 threshold)
{
	if (delta > threshold) {
		x683_set_controller(st, TRAN_GC_CTRL_URGENT);
		st->stop_result = 1;
		return true;
	}
	return false;
}

/* Exact Stop-5 register relation from 0x377858..0x377874. */
static bool x683_stop5(struct x683_tran_gc_state *st,
		       s64 current_sit_component, u32 current_recoverable)
{
	u64 interval = st->cadence_selector ? 500 : 50;
	s64 progress;

	if ((st->cycle % interval) != 0)
		return false;

	progress = current_sit_component +
		(s64)(s32)(current_recoverable - st->baseline_written_seg);

	if (progress > st->baseline_seg)
		return false;

	x683_set_controller(st, TRAN_GC_CTRL_URGENT);
	st->stop_result = 2;
	return true;
}

struct x683_tran_gc_inputs {
	u32 user_block_count;
	u32 log_blocks_per_seg;
	u32 main_segments;
	u32 reserved_segments;
	u32 sit_blocks;
	u32 free_segments;
	u32 recoverable_segments;
	u32 user_segments;
	u32 sit_segments;
	u32 capacity_bucket;
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

	in->recoverable_segments = 0;
	for (i = 0; i < 6; i++)
		in->recoverable_segments += *(u32 *)((char *)dirty_i + 0x68 + i * 4);

	in->user_segments = in->user_block_count >> in->log_blocks_per_seg;
	in->sit_segments = in->sit_blocks >> in->log_blocks_per_seg;
	in->capacity_bucket = (in->user_segments >> 13) & 0x7ffff;
}

/*
 * Stock 0x377120..0x3772dc arming gate.
 * This function preserves the proven arithmetic/state transitions while
 * leaving anonymous helper return values as explicit inputs.
 */
static void x683_detector_arm(struct f2fs_sb_info *sbi,
		struct x683_tran_gc_state *st,
		u32 preliminary_ratio,
		u32 scaled_user_guard,
		u32 scaled_sit_guard)
{
	struct x683_tran_gc_inputs in;
	u64 user_tenth;
	u32 mode;

	x683_collect_inputs(sbi, &in);
	user_tenth = ((u64)in.user_segments * 0xAAAAAAAAAAAAAAABULL) >> 67;

	mode = 2;

	if (in.recoverable_segments > user_tenth &&
	    preliminary_ratio >= 0x15f &&
	    in.free_segments * 25 < in.main_segments * 10 &&
	    (in.user_block_count - in.sit_blocks) > scaled_user_guard &&
	    sbi->reserved_blocks >= scaled_sit_guard) {
		mode = 1;
		st->cadence_selector = 1;
	}

	st->loop_state = mode;
	st->detector_state = 2;
	st->detector_enabled = 1;
}

/*
 * Stock 0x377494..0x377570: timed detector wait/re-entry gate.
 *
 * 0x377494: detector state = 3.
 * 0x3774a8: 0xce58c converts the vendor millisecond timeout to jiffies.
 * 0x3774ac: converted timeout retained in x21.
 * 0x3774b0: 0x57554 tests a current-task flag bit.
 * 0x3774d0: timeout is converted again from global +0xd94 when the
 *            vendor gate permits the timed-wait path.
 * 0x3774dc: stack waitqueue entry is initialized by 0x9c688.
 * 0x3774f4: entry is linked into the queue at x27 by 0x9c6e8.
 * 0x377508: current-task flag is tested again before sleeping.
 * 0x37753c: remaining timeout is passed to 0xcc774, the generic scheduler
 *            timeout path; the return value controls re-entry.
 * 0x377548: waitqueue entry is removed by 0x9c8d0.
 * 0x377554: task-state flag is retested.
 * 0x377570: execution falls into metric collection/detector evaluation.
 *
 * The exact vendor semantic names of +0x974/+0xd84/+0xd94 remain unresolved.
 */
static void x683_detector_state3(struct x683_tran_gc_state *st,
					u32 timeout_ms)
{
	st->detector_state = 3;
	(void)timeout_ms;
	/* Actual waitqueue/schedule_timeout operations are kernel primitives. */
}

static void x683_tran_gc_detect(struct x683_tran_gc_state *st,
				s32 delta1, s32 threshold1,
				s32 delta2, s32 threshold2,
				s64 movement, s64 reference,
				s64 ssr_delta, s64 ssr_threshold,
				s64 current_sit_component,
				u32 current_recoverable)
{
	st->detector_cycles++;

	if (x683_stop1(st, delta1, threshold1) ||
	    x683_stop2(st, delta2, threshold2) ||
	    x683_stop3(st, movement, reference) ||
	    x683_stop4(st, ssr_delta, ssr_threshold) ||
	    x683_stop5(st, current_sit_component, current_recoverable))
		return;

	st->baseline_seg = reference;
	st->baseline_written_seg = current_recoverable;
}
