/*
 * X683/H694 Transsion GC thread reconstruction.
 *
 * Reconstructed/inferred from stock boot.img AArch64 evidence.
 * NOT recovered proprietary Transsion source.
 */

#include "f2fs.h"

#define TRAN_GC_CTRL_NORMAL 0
#define TRAN_GC_CTRL_GREEDY 1
#define TRAN_GC_CTRL_URGENT 2

struct x683_tran_gc_state {
	u64 cycle;                    /* +0x990 */
	u32 controller;               /* +0x998 */
	u8  controller_write_blocked; /* +0x9c0 */
	u32 loop_state;               /* +0x9d0 */
	u32 detector_state;           /* +0x9d4 */
	u64 repeated_detector_count;  /* +0x9d8 */
	u64 detector_cycles;          /* +0x9e0 */
	u32 running_max;              /* +0x9f0 */
	u32 saved_baseline;           /* +0x9f4 */
	u8  stop_result;              /* +0x9f8: 1=Stop4, 2=Stop5 */
	u32 stop_condition;           /* +0x9fc: 1=Stop1, 2=Stop2, 3=Stop3 */
	u8  cadence_selector;         /* +0xa04 */
	u8  loop_active;               /* +0xa05 */
	u8  detector_enabled;         /* +0xa06 */
	s32 baseline_seg;              /* +0xa08 */
	u32 baseline_written_seg;      /* +0xa0c */
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

/* Stop 4: x9 > x8 -> SSR switch. Stock does NOT write +0x9fc=4. */
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

/*
 * Stop 5 exact comparison recovered from 0x377858..0x377874:
 *
 *   interval = cadence_selector ? 500 : 50
 *   if (cycle % interval != 0) -> no Stop 5
 *
 *   x8 = x20 + (w23 - baseline_written_seg)
 *   x9 = sign_extend(baseline_seg)
 *   if (x8 > x9) -> normal progress path
 *   otherwise -> Stop 5
 *
 * Thus Stop 5 is NOT simply "progress <= 0". It compares the current
 * segment/write movement combination against the two saved baselines.
 * x20 and w23 retain raw-register semantics until their producer block is
 * reconstructed.
 */
static bool x683_stop5(struct x683_tran_gc_state *st,
		       s64 x20_current_component, u32 w23_current_written)
{
	u64 interval = st->cadence_selector ? 500 : 50;
	s64 x8, x9;

	if ((st->cycle % interval) != 0)
		return false;

	x8 = x20_current_component +
		(s64)(s32)(w23_current_written - st->baseline_written_seg);
	x9 = (s64)st->baseline_seg;

	if (x8 > x9)
		return false;

	x683_set_controller(st, TRAN_GC_CTRL_URGENT);
	st->stop_result = 2;
	return true;
}

/*
 * One detector iteration. Stop ordering follows the stock basic blocks.
 * Stop 1/2/3 terminate their respective paths by recording +0x9fc.
 * Stop 4/5 record +0x9f8 and may drive controller=2.
 */
static void x683_tran_gc_detect(struct x683_tran_gc_state *st,
				s32 delta1, s32 threshold1,
				s32 delta2, s32 threshold2,
				s64 movement, s64 reference,
				s64 ssr_delta, s64 ssr_threshold,
				s64 x20_current_component,
				u32 w23_current_written)
{
	st->detector_cycles++;

	if (x683_stop1(st, delta1, threshold1) ||
	    x683_stop2(st, delta2, threshold2) ||
	    x683_stop3(st, movement, reference) ||
	    x683_stop4(st, ssr_delta, ssr_threshold) ||
	    x683_stop5(st, x20_current_component, w23_current_written))
		return;

	st->baseline_seg = reference;
	st->baseline_written_seg = w23_current_written;
}

/*
 * Outer thread step. Exact sleep/wakelock/charger helper ordering remains
 * unresolved; do not substitute guessed calls here.
 */
static void x683_tran_gc_thread_step(struct x683_tran_gc_state *st)
{
	if (!st->loop_active || !st->detector_enabled)
		return;

	st->cycle++;
	st->detector_state = 1;
}
