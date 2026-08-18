/*
 * X683/H694 Transsion GC layer reconstruction.
 *
 * Reconstructed/inferred from the verified stock X683/H694 Image.
 * Not recovered proprietary source.
 *
 * The file intentionally keeps unresolved vendor-global/task predicates as
 * callbacks/inputs rather than inventing source-level implementations.
 */
#include "f2fs.h"

#define X683_CTRL_NORMAL 0
#define X683_CTRL_GREEDY 1
#define X683_CTRL_URGENT 2

#define X683_FORCE_GREEDY 2
#define X683_FORCE_URGENT 3

struct x683_tran_gc_state {
	u64 cycle;                 /* +0x990 */
	u32 controller;            /* +0x998 */
	u8 controller_guard;       /* +0x9c0 */
	u32 loop_state;            /* +0x9d0 */
	u32 detector_state;        /* +0x9d4 */
	u64 repeated_count;        /* +0x9d8 */
	u64 detector_cycles;       /* +0x9e0 */
	u32 running_max;           /* +0x9f0 */
	u32 saved_baseline;        /* +0x9f4 */
	u8 stop_result;             /* +0x9f8 */
	u32 stop_condition;        /* +0x9fc */
	u8 detector_mode;           /* +0xa00 */
	u8 cadence_selector;        /* +0xa04 */
	u8 loop_active;             /* +0xa05 */
	u8 detector_active;         /* +0xa06 */
	s32 baseline_segment;       /* +0xa08 */
	u32 baseline_written;       /* +0xa0c */
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
	m->sit_segments = *(u32 *)((char *)sit + 0x10) >> sbi->log_blocks_per_seg;
	m->free_segments = *(u32 *)((char *)free_i + 0x04);
	m->reserved_segments = sm->reserved_segments;
	m->recoverable_segments = 0;
	for (i = 0; i < 6; i++)
		m->recoverable_segments +=
			*(u32 *)((char *)dirty_i + 0x68 + i * 4);
	m->span = m->user_segments - m->sit_segments;
}

static inline void x683_set_controller(struct x683_tran_gc_state *st,
		u32 value)
{
	if (!st->controller_guard)
		st->controller = value;
}

static bool x683_stop1(struct x683_tran_gc_state *st, s32 delta, s32 threshold)
{
	if (delta <= threshold)
		return false;
	st->stop_condition = 1;
	return true;
}

static bool x683_stop2(struct x683_tran_gc_state *st, s32 delta, s32 threshold)
{
	if (delta <= threshold)
		return false;
	st->stop_condition = 2;
	return true;
}

static bool x683_stop3(struct x683_tran_gc_state *st, s64 scaled, s64 ref)
{
	if (scaled >= ref)
		return false;
	st->stop_condition = 3;
	return true;
}

static bool x683_stop4(struct x683_tran_gc_state *st, s64 delta, s64 threshold)
{
	if (delta <= threshold)
		return false;
	x683_set_controller(st, X683_CTRL_URGENT);
	st->stop_result = 1;
	return true;
}

static bool x683_stop5(struct x683_tran_gc_state *st,
		s64 current_component, u32 current_recoverable)
{
	u64 interval = st->cadence_selector ? 500 : 50;
	s64 progress;

	if (st->cycle % interval)
		return false;

	progress = current_component +
		(s64)(s32)(current_recoverable - st->baseline_written);
	if (progress > st->baseline_segment)
		return false;

	x683_set_controller(st, X683_CTRL_URGENT);
	st->stop_result = 2;
	return true;
}

/*
 * State-3 wait/recheck. The stock image uses waitqueue/scheduler primitives;
 * unresolved vendor/task predicates are supplied by the integration layer.
 */
static bool x683_state3_recheck(struct x683_tran_gc_state *st,
		bool (*abort_predicate)(void *), void *opaque)
{
	st->detector_state = 3;
	if (abort_predicate && abort_predicate(opaque)) {
		st->detector_active = 0;
		return false;
	}
	return true;
}

/*
 * Recovered wrapper semantics at 0x37ada8..0x37af00.
 * The stock wrapper forces gc_mode only for controller 1/2 and restores it.
 */
int x683_tran_gc_execute(struct f2fs_sb_info *sbi, bool sync,
		bool background, u32 controller)
{
	int old_gc_mode;
	int ret;

	if (controller == X683_CTRL_NORMAL)
		return f2fs_gc(sbi, sync, background, NULL_SEGNO);

	old_gc_mode = sbi->gc_mode;
	if (controller == X683_CTRL_GREEDY)
		sbi->gc_mode = X683_FORCE_GREEDY;
	else if (controller == X683_CTRL_URGENT)
		sbi->gc_mode = X683_FORCE_URGENT;
	else
		return -EINVAL;

	ret = f2fs_gc(sbi, sync, background, NULL_SEGNO);
	sbi->gc_mode = old_gc_mode;
	return ret;
}

/*
 * One integrated detector step. Threshold inputs are produced by the
 * separately reconstructed binary-derived helper. This avoids guessing the
 * remaining vendor-global selector/state producers.
 */
int x683_tran_gc_step(struct f2fs_sb_info *sbi,
		struct x683_tran_gc_state *st,
		s32 delta1, s32 threshold1,
		s32 delta2, s32 threshold2,
		s64 scaled3, s64 ref3,
		s64 delta4, s64 threshold4,
		s64 current_component,
		bool (*abort_predicate)(void *), void *opaque)
{
	struct x683_tran_gc_metrics m;

	if (!sbi || !st)
		return -EINVAL;

	st->cycle++;
	if (!st->detector_active) {
		st->detector_state = 2;
		st->detector_active = 1;
		st->loop_active = 1;
	}

	if (!x683_state3_recheck(st, abort_predicate, opaque))
		return 0;

	st->detector_cycles++;
	x683_collect_metrics(sbi, &m);

	if (x683_stop1(st, delta1, threshold1) ||
	    x683_stop2(st, delta2, threshold2) ||
	    x683_stop3(st, scaled3, ref3) ||
	    x683_stop4(st, delta4, threshold4) ||
	    x683_stop5(st, current_component, m.recoverable_segments))
		return 1;

	st->baseline_segment = (s32)ref3;
	st->baseline_written = m.recoverable_segments;
	return 0;
}
