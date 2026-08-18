/*
 * X683/H694 Transsion GC thread reconstruction.
 * Reconstructed/inferred from stock Image evidence. Not proprietary source.
 *
 * IMPORTANT: this is a binary-derived scaffold, not yet a buildable drop-in
 * replacement for the proprietary thread. Unresolved vendor/task producers
 * remain explicit inputs.
 */
#include "f2fs.h"

#define X683_GC_NORMAL  0
#define X683_GC_GREEDY  1
#define X683_GC_URGENT  2

typedef int (*x683_tran_gc_exec_t)(struct f2fs_sb_info *, bool, bool, u32);

struct x683_tran_gc_vendor_state {
	u8 controller_write_blocked; /* +0x9c0 */
	u8 detector_state;            /* +0x9d4 */
	u8 cadence_selector;          /* +0xa04 */
	u8 loop_active;               /* +0xa05 */
	u8 detector_active;           /* +0xa06 */
	u8 detector_mode;             /* +0xa00 */
	u32 controller;               /* +0x998 */
	u32 loop_state;               /* +0x9d0 */
	u64 repeated_count;           /* +0x9d8 */
	u64 detector_cycles;          /* +0x9e0 */
	u32 running_max;              /* +0x9f0 */
	u32 saved_baseline;           /* +0x9f4 */
	u8 stop_result;               /* +0x9f8 */
	u32 stop_condition;           /* +0x9fc */
	s32 baseline_segment;         /* +0xa08 */
	u32 baseline_written;         /* +0xa0c */
	u64 cycle;                    /* +0x990 */
};

/*
 * The exact meaning of the six values around dirty_info +0x68 was not
 * sufficiently established to justify summing them here. Keep the detector's
 * recoverable-segment input explicit until the raw structure mapping is
 * resolved. This avoids turning an uncertain offset interpretation into
 * source-level fact.
 */
struct x683_tran_gc_metrics {
	u32 user_block_count;
	u32 sit_blocks;
	u32 user_segments;
	u32 sit_segments;
	u32 main_segments;
	u32 free_segments;
	u32 reserved_segments;
	u32 reserved_blocks;
	u32 recoverable_segments;
	u32 span;
};

static void x683_collect_metrics(struct f2fs_sb_info *sbi,
				 struct x683_tran_gc_metrics *m,
				 u32 recoverable_segments)
{
	struct f2fs_sm_info *sm = SM_I(sbi);
	struct sit_info *sit = sm->sit_info;
	struct free_segmap_info *free_i = sm->free_info;

	m->user_block_count = sbi->user_block_count;
	m->sit_blocks = *(u32 *)((char *)sit + 0x10);
	m->user_segments = m->user_block_count >> sbi->log_blocks_per_seg;
	m->sit_segments = m->sit_blocks >> sbi->log_blocks_per_seg;
	m->main_segments = sm->main_segments;
	m->free_segments = *(u32 *)((char *)free_i + 0x04);
	m->reserved_segments = sm->reserved_segments;
	m->reserved_blocks = sbi->reserved_blocks;
	m->recoverable_segments = recoverable_segments;
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
			       u32 scaled_sit_guard,
			       u32 recoverable_segments)
{
	struct x683_tran_gc_metrics m;
	u32 user_tenth;

	x683_collect_metrics(sbi, &m, recoverable_segments);
	user_tenth = (u32)(((u64)m.user_segments *
				0xAAAAAAAAAAAAAAABULL) >> 67);

	/* Binary-confirmed arming shape; unresolved guard producers stay inputs. */
	v->detector_mode = 2;
	v->cadence_selector = 0;
	if (m.recoverable_segments > user_tenth &&
	    preliminary_ratio >= 0x15f &&
	    (u64)m.free_segments * 25 < (u64)m.main_segments * 10 &&
	    (u64)(m.user_block_count - m.sit_blocks) > scaled_user_guard &&
	    m.reserved_blocks >= scaled_sit_guard) {
		v->detector_mode = 1;
		v->cadence_selector = 1;
	}

	v->loop_state = v->detector_mode;
	v->detector_state = 2;
	v->detector_active = 1;
	v->loop_active = 1;
}

/*
 * State 3 uses the kernel waitqueue/scheduler machinery. Do not emulate
 * msecs_to_jiffies() with arithmetic: the exact conversion is configuration
 * dependent. The real integration should call msecs_to_jiffies(timeout_ms)
 * and the recovered waitqueue sequence.
 */
static bool x683_state3_wait(struct x683_tran_gc_vendor_state *v,
			     u32 timeout_ms,
			     bool (*abort_predicate)(void *opaque),
			     void *opaque)
{
	v->detector_state = 3;
	(void)timeout_ms;

	/* 0x9c688 / 0x9c6e8 / 0x9c8d0 are intentionally not faked here. */
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

static void x683_run_detector(struct f2fs_sb_info *sbi,
			       struct x683_tran_gc_vendor_state *v,
			       u32 recoverable_segments,
			       s32 delta1, s32 threshold1,
			       s32 delta2, s32 threshold2,
			       s64 scaled3, s64 reference3,
			       s64 delta4, s64 threshold4,
			       s64 current_segment_component)
{
	struct x683_tran_gc_metrics m;

	v->detector_cycles++;
	x683_collect_metrics(sbi, &m, recoverable_segments);

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
 * One reconstruction step. This deliberately does not claim to be the full
 * scheduler/thread implementation: the exact vendor/task predicates and
 * surrounding wait/re-entry branches remain unresolved.
 */
int x683_tran_gc_thread_step(struct f2fs_sb_info *sbi,
				     struct x683_tran_gc_vendor_state *v,
				     u32 recoverable_segments,
				     bool (*abort_predicate)(void *),
				     void *opaque)
{
	if (!sbi || !v)
		return -EINVAL;

	v->cycle++;
	if (!v->detector_active)
		x683_arm_detector(sbi, v, 0x15f, 0, 0,
				recoverable_segments);

	return x683_state3_wait(v, 500, abort_predicate, opaque) ? 1 : 0;
}
