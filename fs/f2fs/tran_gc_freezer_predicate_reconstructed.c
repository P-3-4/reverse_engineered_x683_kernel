/*
 * X683/H694 reconstruction of the freezer-aware task predicate used by
 * stock helper 0xcc774. Binary-derived; not proprietary source recovery.
 */
#include "f2fs.h"

/*
 * Exact storage roles recovered from kernel/power/process.c call/data flow:
 *
 *   global +0x24 = pm_freezing
 *   global +0x28 = pm_nosig_freezing
 *
 * These are represented as explicit inputs here so this file remains usable
 * until the exact vendor/global symbol integration is performed.
 */
struct x683_cc774_state {
	bool pm_freezing;
	bool pm_nosig_freezing;
};

/*
 * Exact helper shape at 0x1051a8:
 *   task +0x950 -> object
 *   object +0x30 -> state
 *   state +0xb8 -> flags
 *   return (flags & 0x6) != 0
 *
 * The stock helper brackets the read with preempt disable/enable. The exact
 * object/type names are intentionally not invented.
 */
static bool x683_task_freezer_state(const struct task_struct *task)
{
	const u64 *obj;
	const u64 *state;
	u32 flags;
	unsigned long irqflags;

	(void)irqflags;
	preempt_disable();
	obj = *(const u64 **)((const char *)task + 0x950);
	if (!obj) {
		preempt_enable();
		return false;
	}
	state = *(const u64 **)((const char *)obj + 0x30);
	if (!state) {
		preempt_enable();
		return false;
	}
	flags = *(const u32 *)((const char *)state + 0xb8);
	preempt_enable();
	return !!(flags & 0x6);
}

/*
 * Recovered boolean predicate at 0xcc774.
 */
bool x683_cc774_recovered(const struct task_struct *task,
			  const struct x683_cc774_state *st)
{
	u32 task_flags44;
	unsigned long task_flags0;
	u8 task_byte46;

	if (!task || !st)
		return false;

	task_flags44 = *(const u32 *)((const char *)task + 0x44);
	if (task_flags44 & 0x80008000U)
		return false;

	task_flags0 = *(const unsigned long *)task;
	if (task_flags0 & (1UL << 18))
		return false;

	if (st->pm_nosig_freezing)
		return false;

	if (x683_task_freezer_state(task))
		return false;

	if (!st->pm_freezing)
		return false;

	task_byte46 = *(const u8 *)((const char *)task + 0x46);
	if (task_byte46 & (1U << 5))
		return false;

	return true;
}
