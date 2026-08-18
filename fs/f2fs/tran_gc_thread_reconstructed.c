/*
 * X683/H694 Transsion GC thread reconstruction.
 * Reconstructed/inferred from stock Image evidence. Not proprietary source.
 *
 * IMPORTANT: this remains a binary-derived scaffold, not a byte-equivalent
 * replacement for the proprietary thread.
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
	s32 saved_sit_segments;       /* detector stack/sp+0x28 baseline */
	u64 cycle;                    /* +0x990 */
};

/*
 * The stock detector uses two distinct metrics. Do not conflate them:
 *
 * arming_dirty_segments = sum(nr_dirty[0..5])
 * recoverable_segments  = free_segments + nr_dirty[PRE]
 *
 * The former belongs to the static arming predicates; the latter is the
 * w23 metric consumed by Stop 1..5.
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
	u32 arming_dirty_segments;
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

	m->user_block_count = sbi->user_block_count;
	m->sit_blocks = *(u32 *)((char *)sit + 0x10);
	m->user_segments = m->user_block_count >> sbi->log_blocks_per_seg;
	m->sit_segments = m->sit_blocks >> sbi->log_blocks_per_seg;
	m->main_segments = sm->main_segments;
	m->free_segments = *(u32 *)((char *)free_i + 0x04);
	m->reserved_segments = sm->reserved_segments;
	m->reserved_blocks = sbi->reserved_blocks;

	m->arming_dirty_segments = 0;
	for (i = 0; i < 6; ++i)
		m->arming_dirty_segments += *(u32 *)((char *)dirty_i + 0x68 + i * 4);

	m->recoverable_segments = m->free_segments +
		*(u32 *)((char *)dirty_i + 0x84); /* nr_dirty[PRE] */
	m->span = m->user_segments - m->sit_segments;
}

static inline u32 x683_pct(u32 value)
{
	return (u32)(((u64)value * 0x51EB851FULL) >> 37);
}

static inline void x683_set_controller(
		struct x683_tran_gc_vendor_state *v, u32 value)
{
	if (!v->controller_write_blocked)
		v->controller = value;
}

/*
 * Exact +0x974 producer shape recovered at 0x37acf8.
 *
 * The callback receives an event number and event data. For event 9 it reads
 * *(event_data + 8): state 0 sets the vendor/global wait flag to 1, state 4
 * clears it. Other state values leave the flag unchanged. When auxiliary
 * state +0x898 exists, both branches signal the object at +0x978 with
 * operation parameters (3, 1, 0).
 *
 * The callback's public vendor/event name is not proven, so this is kept as a
 * primitive rather than assigned a fabricated name.
 */
static inline void x683_event9_update(u32 event, u32 event_state,
				       u32 *wait_flag)
{
	if (!wait_flag || event != 9)
		return;

	if (event_state == 0)
		*wait_flag = 1;
	else if (event_state == 4)
		*wait_flag = 0;
}

/*
 * Static detector arming, reconstructed from 0x377120..0x377494.
 * The exact vendor/global guard producers are still kept outside this
 * scaffold; the arithmetic and metric distinction are now explicit.
 */
static void x683_arm_detector(struct f2fs_sb_info *sbi,
			       struct x683_tran_gc_vendor_state *v)
{
	struct x683_tran_gc_metrics m;
	s64 ratio;
	u32 denominator;
	u32 ratio_value;
	u32 dirty_tenth;
	u32 user_guard;
	u32 sit_guard;

	x683_collect_metrics(sbi, &m);
	v->detector_mode = 2;
	v->cadence_selector = 0;

	denominator = m.arming_dirty_segments + m.free_segments;
	ratio = ((s64)(m.free_segments + m.arming_dirty_segments -
			m.main_segments) << sbi->log_blocks_per_seg) +
			((s64)m.sit_blocks - m.user_block_count);
	ratio_value = denominator ? (u32)(ratio / denominator) : 0;
	dirty_tenth = (u32)(((u64)m.user_segments *
				0xAAAAAAAAAAAAAAABULL) >> 67);

	user_guard = x683_pct(13U * m.user_block_count);
	sit_guard = x683_pct(27U * m.sit_blocks);

	if (m.arming_dirty_segments > dirty_tenth &&
	    ratio_value >= 0x15f &&
	    (u64)m.free_segments * 25 < (u64)m.main_segments * 10 &&
	    (u64)(m.user_block_count - m.sit_blocks) > user_guard &&
	    m.reserved_blocks >= sit_guard) {
		v->detector_mode = 1;
		v->cadence_selector = 1;
	}

	v->loop_state = v->detector_mode;
	v->detector_state = 2;
	v->detector_active = 1;
	v->loop_active = 1;
}

/*
 * State 3 wait/recheck semantics recovered from 0x377494..0x377570.
 *
 * The stock path performs the real kernel waitqueue/scheduler sequence, then
 * consults the producer-driven +0x974 flag before metric collection. The
 * concrete callback producer is now known, but its public event name remains
 * unresolved. +0x20 on the controller object and helper 0xcc774 are still
 * vendor/task predicates.
 */
static bool x683_state3_wait(struct x683_tran_gc_vendor_state *v,
			     u32 timeout_ms,
			     bool wait_flag_set,
			     bool (*abort_predicate)(void *opaque),
			     void *opaque)
{
	v->detector_state = 3;
	(void)timeout_ms;

	if (wait_flag_set)
		return true;

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
		       s32 running_max, s32 recoverable,
		       s32 saved_sit_segments, s32 sit_segments,
		       s32 threshold)
{
	s64 delta = (s64)(running_max - recoverable) +
		(s64)saved_sit_segments - sit_segments;

	if (delta > threshold) {
		x683_set_controller(v, X683_GC_URGENT);
		v->stop_result = 1;
		return true;
	}
	return false;
}

static bool x683_stop5(struct x683_tran_gc_vendor_state *v,
		       s32 sit_segments, u32 recoverable_segments)
{
	s64 progress;

	progress = (s64)sit_segments +
		(s64)(s32)(recoverable_segments - v->baseline_written);
	if (progress > v->baseline_segment)
		return false;

	x683_set_controller(v, X683_GC_URGENT);
	v->stop_result = 2;
	return true;
}

static void x683_run_detector(struct f2fs_sb_info *sbi,
			       struct x683_tran_gc_vendor_state *v,
			       s32 threshold1,
			       s32 delta2, s32 threshold2,
			       s64 scaled3, s64 reference3,
			       s32 threshold4)
{
	struct x683_tran_gc_metrics m;

	v->detector_cycles++;
	x683_collect_metrics(sbi, &m);

	if (x683_stop1(v,
			(s32)(m.recoverable_segments - v->saved_baseline),
			threshold1) ||
	    x683_stop2(v, delta2, threshold2) ||
	    x683_stop3(v, scaled3, reference3) ||
	    x683_stop4(v, (s32)v->running_max,
			(s32)m.recoverable_segments,
			v->saved_sit_segments,
			(s32)m.sit_segments,
			threshold4) ||
	    x683_stop5(v, (s32)m.sit_segments,
			m.recoverable_segments))
		return;

	v->baseline_segment = (s32)reference3;
	v->baseline_written = m.recoverable_segments;
}

/*
 * One reconstruction step. This deliberately does not claim to be the full
 * proprietary kthread implementation.
 */
int x683_tran_gc_thread_step(struct f2fs_sb_info *sbi,
				     struct x683_tran_gc_vendor_state *v,
				     bool wait_flag_set,
				     bool (*abort_predicate)(void *),
				     void *opaque)
{
	if (!sbi || !v)
		return -EINVAL;

	v->cycle++;
	if (!v->detector_active)
		x683_arm_detector(sbi, v);

	return x683_state3_wait(v, 500, wait_flag_set,
				abort_predicate, opaque) ? 1 : 0;
}