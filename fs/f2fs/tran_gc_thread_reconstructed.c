/*
 * X683/H694 Transsion GC thread reconstruction.
 *
 * Reconstructed/inferred from stock boot.img AArch64 evidence.
 * NOT recovered proprietary Transsion source.
 *
 * This file deliberately separates proven behavior from unresolved helper
 * semantics. Names are semantic labels assigned by the reverse-engineering
 * project.
 */

#include "f2fs.h"

#define TRAN_GC_CTRL_NORMAL 0
#define TRAN_GC_CTRL_GREEDY 1
#define TRAN_GC_CTRL_URGENT 2

struct x683_tran_gc_state {
	/* +0x990 */
	u64 cycle;
	/* +0x998 */
	u32 controller;
	/* +0x9c0 */
	u8 controller_write_blocked;
	/* +0x9d0 */
	u32 loop_state;
	/* +0x9d4 */
	u32 detector_state;
	/* +0x9d8 */
	u64 repeated_detector_count;
	/* +0x9e0 */
	u64 detector_cycles;
	/* +0x9f0 */
	u32 running_max;
	/* +0x9f4 */
	u32 saved_baseline;
	/* +0x9f8 */
	u8 stop_result;
	/* +0x9fc */
	u32 stop_condition;
	/* +0xa04 */
	u8 cadence_selector;
	/* +0xa05 */
	u8 loop_active;
	/* +0xa06 */
	u8 detector_enabled;
	/* +0xa08 */
	s32 baseline_seg;
	/* +0xa0c */
	u32 baseline_written_seg;
};

static void x683_set_controller(struct x683_tran_gc_state *st, u32 value)
{
	/* Directly proven guard around the Stop-4/5 controller store. */
	if (!st->controller_write_blocked)
		st->controller = value;
}

/*
 * Stop 1:
 *   delta_seg > table[index] * base * ~0.025
 *   -> stop_condition = 1
 *
 * The table index/base production is not yet reconstructed exactly.
 */
static bool x683_stop1(struct x683_tran_gc_state *st,
		       s32 delta_seg, s32 threshold)
{
	if (delta_seg > threshold) {
		st->stop_condition = 1;
		return true;
	}
	return false;
}

/* Stop 2: second delta > second table-derived threshold. */
static bool x683_stop2(struct x683_tran_gc_state *st,
		       s32 delta, s32 threshold)
{
	if (delta > threshold) {
		st->stop_condition = 2;
		return true;
	}
	return false;
}

/* Stop 3: scaled movement < signed segment/reference quantity. */
static bool x683_stop3(struct x683_tran_gc_state *st,
		       s64 scaled_movement, s64 segment_reference)
{
	if (scaled_movement < segment_reference) {
		st->stop_condition = 3;
		return true;
	}
	return false;
}

/*
 * Stop 4 is proven to be the SSR-switch path.
 * x9 is the calculated dec/inc segment delta and threshold is loaded from
 * the separate vendor-state object (+0xd90 there).
 */
static bool x683_stop4(struct x683_tran_gc_state *st,
		       s64 delta, s64 threshold)
{
	if (delta > threshold) {
		x683_set_controller(st, TRAN_GC_CTRL_URGENT);
		st->stop_result = 1;
		st->stop_condition = 4;
		return true;
	}
	return false;
}

/*
 * Stop 5 is evaluated only at the selected periodic cadence.
 * The stock code computes cycle % (50 or 500), then compares current
 * segment/write progress against +0xa08/+0xa0c baselines.
 */
static bool x683_stop5(struct x683_tran_gc_state *st,
		       s64 current_progress)
{
	u64 interval = st->cadence_selector ? 500 : 50;

	if (interval == 0 || (st->cycle % interval) != 0)
		return false;

	/* Exact stock comparison is retained as a caller-supplied predicate. */
	if (current_progress <= 0) {
		x683_set_controller(st, TRAN_GC_CTRL_URGENT);
		st->stop_result = 2;
		st->stop_condition = 5;
		return true;
	}
	return false;
}

/*
 * One detector iteration. The actual production of delta/threshold operands
 * is intentionally outside this reconstruction until the helper at
 * 0x37b5d4..0x37b8c0 is fully recovered.
 */
static void x683_tran_gc_detect(struct x683_tran_gc_state *st,
				s32 delta1, s32 threshold1,
				s32 delta2, s32 threshold2,
				s64 movement, s64 reference,
				s64 ssr_delta, s64 ssr_threshold,
				s64 progress)
{
	st->detector_cycles++;

	if (x683_stop1(st, delta1, threshold1) ||
	    x683_stop2(st, delta2, threshold2) ||
	    x683_stop3(st, movement, reference) ||
	    x683_stop4(st, ssr_delta, ssr_threshold) ||
	    x683_stop5(st, progress))
		return;

	st->baseline_seg = reference;
	st->baseline_written_seg = (u32)movement;
}

/*
 * Reconstructed outer thread loop.
 *
 * Proven ordering:
 *   cycle/invocation counter is maintained at +0x990;
 *   detector state/counters live at +0x9d0..+0x9e0;
 *   detector enable/loop bytes gate continued detection;
 *   Stop 4/5 can set controller=2;
 *   controller=2 is consumed later by tran_f2fs_gc as URGENT GC.
 *
 * Scheduling, wakelock ownership, charger detection and the exact calls to
 * need_switch_ssr()/tran_urgent_gc() are intentionally not guessed here.
 */
static void x683_tran_gc_thread_step(struct x683_tran_gc_state *st)
{
	if (!st->loop_active || !st->detector_enabled)
		return;

	st->cycle++;
	st->detector_state = 1;

	/* Operand production and vendor event helpers remain unresolved. */
}
