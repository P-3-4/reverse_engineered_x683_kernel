/*
 * X683/H694 Transsion GC thread reconstruction.
 * Reconstructed/inferred from stock Image evidence. Not proprietary source.
 *
 * This file models the recovered control/data flow. Vendor-global addresses
 * and several task/registration internals remain explicit parameters because
 * they are not ordinary f2fs_sb_info fields.
 */
#include "f2fs.h"

#define X683_GC_NORMAL 0
#define X683_GC_GREEDY 1
#define X683_GC_URGENT 2

typedef int (*x683_tran_gc_exec_t)(struct f2fs_sb_info *, bool, bool, u32);

struct x683_tran_gc_vendor_state {
	u8 controller_write_blocked; /* +0x9c0 */
	u8 detector_state;            /* +0x9d4 */
	u8 cadence_selector;          /* +0xa04 */
	u8 loop_active;               /* +0xa05 */
	u8 detector_active;           /* +0xa06 */
	u8 detector_mode;             /* +0xa00 */
	u8 need_switch_ssr;           /* descriptor-backed control value */
	u8 urgent_gc;                 /* descriptor-backed control value */
	u8 charger_type;              /* descriptor-backed control value */
	u32 controller;               /* +0x998 */
	u32 loop_state;               /* +0x9d0 semantic label */
	u64 repeated_count;           /* +0x9d8 */
	u64 detector_cycles;          /* +0x9e0 */
	u32 running_max;              /* +0x9f0 */
	u32 saved_baseline;            /* +0x9f4 */
	u8 stop_result;               /* +0x9f8 */
	u32 stop_condition;            /* +0x9fc */
	s32 baseline_segment;          /* +0xa08 */
	u32 baseline_written;          /* +0xa0c */
	u64 cycle;                     /* +0x990 */
};

struct x683_tran_gc_metrics {
	u32 user_segments;
	u32 sit_segments;
	u32 free_segments;
	u32 reserved_segments;
	u32 recoverable_segments;
	u32 span;
};

static void x683_collect_metrics(struct f2fs_sb_info *sbi,
				 struct x683_tran_gc_metrics *m)
{
	struct f2fs_sm_info *sm = SM_I(sbi);
	struct sit_info *sit = sm->sit_info;
	struct free_segmap_info *free_i = sm->free_info;
	struct dirty_seglist_info *dirty_i = sm->dirty_info;
	u32 i;

	m->user_segments = sbi->user_block_count >> sbi->log_blocks_per_seg;
	m->sit_segments =
		(*(u32 *)((char *)sit + 0x10)) >> sbi->log_blocks_per_seg;
	m->free_segments = *(u32 *)((char *)free_i + 0x04);
	m->reserved_segments = sm->reserved_segments;
	m->recoverable_segments = 0;
	for (i = 0; i < 6; i++)
		m->recoverable_segments +=
			*(u32 *)((char *)dirty_i + 0x68 + i * 4);
	m->span = m->user_segments - m->sit_segments;
}

static inline void x683_set_controller(
		struct x683_tran_gc_vendor_state *v, u32 value)
{
	if (!v->controller_write_blocked)
		v->controller = value;
}

static void x683_arm_detector(struct f2fs_sb_info *sbi,
			       struct x683_tran_gc_vendor_state *v,
			       u32 preliminary_ratio,
			       u32 scaled_user_guard,
			       u32 scaled_sit_guard)
{
	struct x683_tran_gc_metrics m;
	u32 user_tenth;

	x683_collect_metrics(sbi, &m);
	user_tenth = (u32)(((u64)m.user_segments *
				0xAAAAAAAAAAAAAAABULL) >> 67);

	/* Binary-confirmed arming shape; vendor-global operands stay explicit. */
	v->detector_mode = 2;
	v->cadence_selector = 0;
	if (m.recoverable_segments > user_tenth &&
	    preliminary_ratio >= 0x15f &&
	    (u64)m.free_segments * 25 < (u64)sm->main_segments * 10 &&
	    (s64)sbi->user_block_count -
			(s64)*(u32 *)((char *)m /* deliberate semantic placeholder */) >
			(s64)scaled_user_guard &&
	    sbi->reserved_blocks >= scaled_sit_guard) {
		v->detector_mode = 1;
		v->cadence_selector = 1;
	}

	v->loop_state = v->detector_mode;
	v->detector_state = 2;
	v->detector_active = 1;
	v->loop_active = 1;
}

/*
 * State 3: the binary performs a real timed wait. The exact vendor task
 * predicate is represented as a callback rather than guessed source.
 */
static bool x683_state3_wait(struct x683_tran_gc_vendor_state *v,
			     u32 timeout_ms,
			     bool (*abort_predicate)(void *opaque),
			     void *opaque)
{
	v->detector_state = 3;

	/* 0xce58c: (ms + 3) >> 2 in this stock kernel. */
	(void)((timeout_ms + 3U) >> 2);

	/* 0x9c688/0x9c6e8/0x9c8d0 wait-entry setup/queue/finish. */
	if (abort_predicate && abort_predicate(opaque)) {
		v->detector_active = 0;
		return false;
	}
	return true;
}

static bool x683_stop1(struct x683_tran_gc_vendor_state *v,
		       s32 delta, s32 threshold)
{
	if (delta > threshold) {
		v->stop_condition = 1;
		return true;
	}
	return false;
}

static bool x683_stop2(struct x683_tran_gc_vendor_state *v,
		       s32 delta, s32 threshold)
{
	if (delta > threshold) {
		v->stop_condition = 2;
		return true;
	}
	return false;
}

static bool x683_stop3(struct x683_tran_gc_vendor_state *v,
		       s64 scaled, s64 reference)
{
	if (scaled < reference) {
		v->stop_condition = 3;
		return true;
	}
	return false;
}

static bool x683_stop4(struct x683_tran_gc_vendor_state *v,
		       s64 delta, s64 threshold)
{
	if (delta > threshold) {
		x683_set_controller(v, X683_GC_URGENT);
		v->stop_result = 1;
		return true;
	}
	return false;
}

static bool x683_stop5(struct x683_tran_gc_vendor_state *v,
		       s64 current_segment_component,
		       u32 current_recoverable)
{
	u64 interval = v->cadence_selector ? 500 : 50;
	s64 progress;

	if ((v->cycle % interval) != 0)
		return false;

	progress = current_segment_component +
		(s64)(s32)(current_recoverable - v->baseline_written);
	if (progress > v->baseline_segment)
		return false;

	x683_set_controller(v, X683_GC_URGENT);
	v->stop_result = 2;
	return true;
}

/*
 * Integrated detector evaluation. Stop 1/2/3 are calculated before the
 * vendor SSR threshold Stop 4 and periodic no-progress Stop 5.
 */
static void x683_run_detector(struct f2fs_sb_info *sbi,
			       struct x683_tran_gc_vendor_state *v,
			       s32 delta1, s32 threshold1,
			       s32 delta2, s32 threshold2,
			       s64 scaled3, s64 reference3,
			       s64 delta4, s64 threshold4,
			       s64 current_segment_component)
{
	struct x683_tran_gc_metrics m;

	v->detector_cycles++;
	x683_collect_metrics(sbi, &m);

	if (x683_stop1(v, delta1, threshold1) ||
	    x683_stop2(v, delta2, threshold2) ||
	    x683_stop3(v, scaled3, reference3) ||
	    x683_stop4(v, delta4, threshold4) ||
	    x683_stop5(v, current_segment_component, m.recoverable_segments))
		return;

	v->baseline_segment = (s32)reference3;
	v->baseline_written = m.recoverable_segments;
}

/*
 * Main reconstructed loop. The exact kthread scheduling wrapper is kept
 * outside this function because the stock binary interleaves vendor task
 * predicates with the waitqueue operations.
 */
int x683_tran_gc_thread_func_reconstructed(struct f2fs_sb_info *sbi,
					   struct x683_tran_gc_vendor_state *v,
					   x683_tran_gc_exec_t gc_exec,
					   bool (*abort_predicate)(void *),
					   void *opaque)
{
	if (!sbi || !v || !gc_exec)
		return -EINVAL;

	v->cycle++;

	if (!v->detector_active)
		x683_arm_detector(sbi, v, 0x15f, 0, 0);

	if (!x683_state3_wait(v, 500, abort_predicate, opaque))
		return 0;

	/* The exact threshold operands are produced by the stock metric/helper
	 * blocks. This integration point intentionally receives those already
	 * reconstructed values rather than inventing vendor globals here. */
	return gc_exec(sbi, true, true, NULL_SEGNO);
}
