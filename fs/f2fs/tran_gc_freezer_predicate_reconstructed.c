/*
 * X683/H694 reconstruction of the freezer-aware task predicate used by
 * stock helper 0xcc774. Binary-derived; not proprietary source recovery.
 */
#include "f2fs.h"
#include <linux/freezer.h>
#include <linux/sched.h>

/*
 * 0x1051a8 is cgroup_freezing(task).
 * The binary accesses the task freezer cgroup state under the kernel's
 * read-side/preemption protection and tests CGROUP_FREEZING/CGROUP_FROZEN.
 */
static bool x683_cgroup_freezing(const struct task_struct *task)
{
	return cgroup_freezing((struct task_struct *)task);
}

/*
 * Recovered boolean predicate at Image+0xcc774.
 *
 * It is an inverse/eligibility form of the Android 4.14 freezer slow-path:
 * true means the task may remain eligible for this detector path while
 * system PM freezing is active.
 *
 * X683-specific deviation from stock Android/common 4.14:
 *   task->flags bit 18 (PF_KSWAPD) is rejected directly instead of the
 *   usual TIF_MEMDIE test.
 */
bool x683_cc774_recovered(const struct task_struct *task)
{
	if (!task)
		return false;

	if (task->flags & (PF_NOFREEZE | PF_SUSPEND_TASK))
		return false;

	if (task->flags & PF_KSWAPD)
		return false;

	if (pm_nosig_freezing)
		return false;

	if (x683_cgroup_freezing(task))
		return false;

	if (!pm_freezing)
		return false;

	if (task->flags & PF_KTHREAD)
		return false;

	return true;
}

/*
 * Recovered Image+0x19f0020 state-3 gate.
 * +0x20 is system_freezing_cnt, not a vendor/controller field.
 */
bool x683_state3_freezer_gate(const struct task_struct *task)
{
	if (atomic_read(&system_freezing_cnt))
		return false;

	return x683_cc774_recovered(task);
}
