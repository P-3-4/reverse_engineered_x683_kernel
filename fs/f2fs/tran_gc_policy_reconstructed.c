/*
 * X683/H694 Transsion GC policy/orchestration reconstruction.
 * Binary-derived from stock Image 0x366cd4..0x366edc.
 * Not proprietary source recovery. Not build-proven.
 *
 * This file intentionally models control/data flow only where the stock
 * binary establishes it. Opaque helper return semantics remain callbacks.
 */
#include "f2fs.h"

#define X683_SBI_MODE_OFF       0x534
#define X683_SBI_GUARD6_OFF     0x444
#define X683_SBI_MOUNTOPT_OFF   0x4b8
#define X683_SBI_MOUNTBYTE_OFF  0x4b9
#define X683_SBI_STAT_OFF       0x568

/* Seven exact guard loads, in stock order. */
static const unsigned int x683_guard_offsets[] = {
	0x44c, 0x450, 0x454, 0x448,
	0x444, 0x45c, 0x458,
};

struct x683_policy_ops {
	bool (*policy_test)(struct f2fs_sb_info *sbi, unsigned int selector);
	bool (*gate_0x80)(struct f2fs_sb_info *sbi);
	int (*helper_455)(struct f2fs_sb_info *sbi, unsigned int arg);
	int (*segment_manager)(struct f2fs_sb_info *sbi,
		unsigned int a, unsigned int b);
	int (*list_drain)(struct f2fs_sb_info *sbi, unsigned int arg);
	bool (*fixed_point_guard)(struct f2fs_sb_info *sbi);
	bool (*secondary_dirty_guard)(struct f2fs_sb_info *sbi);
	bool (*time_current_guard)(struct f2fs_sb_info *sbi);

	/* Terminal path: identities remain unresolved at source level. */
	void (*terminal_0x3e1014)(void *stack_object);
	int (*terminal_0x34e224)(void *stack_object, unsigned int arg);
	void (*terminal_0x3e1558)(void *stack_object);
	int (*terminal_0x341250)(struct super_block *sb, unsigned int arg);
	void (*terminal_stat_increment)(struct f2fs_sb_info *sbi);
};

static inline u32 x683_sbi_u32(const struct f2fs_sb_info *sbi,
		unsigned int off)
{
	return *(const u32 *)((const char *)sbi + off);
}

static inline u8 x683_sbi_u8(const struct f2fs_sb_info *sbi,
		unsigned int off)
{
	return *(const u8 *)((const char *)sbi + off);
}

/*
 * Exact stock seven-field gate.
 *
 * First six fields are checked one-by-one with cbnz to the common guarded
 * path. The seventh (+0x458) is only tested after all six earlier fields are
 * zero. Thus the normal path requires ALL SEVEN fields to be zero.
 */
static bool x683_seven_guard_all_zero(const struct f2fs_sb_info *sbi)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(x683_guard_offsets); ++i) {
		if (x683_sbi_u32(sbi, x683_guard_offsets[i]))
			return false;
	}
	return true;
}

/*
 * 0x366cd4 control-flow reconstruction.
 *
 * The caller supplies opaque helper results so we do not invent source-level
 * meanings for selector values or vendor helpers.
 */
int x683_tran_gc_policy_step(struct f2fs_sb_info *sbi,
		const struct x683_policy_ops *ops,
		void *stack_object)
{
	bool pass;

	if (!sbi || !ops)
		return -EINVAL;

	/* Entry guard: sbi + 0x48, bit 3. */
	if (x683_sbi_u32(sbi, 0x48) & BIT(3))
		return 0;

	/* selector 4: false -> gate(0x80) */
	if (ops->policy_test && !ops->policy_test(sbi, 4)) {
		if (ops->gate_0x80)
			(void)ops->gate_0x80(sbi);
	}

	/* selector 1: false -> helper(455) */
	if (ops->policy_test && !ops->policy_test(sbi, 1)) {
		if (ops->helper_455)
			(void)ops->helper_455(sbi, 455);
	}

	/* selector 0 chooses exactly one of the two vendor helpers. */
	pass = ops->policy_test ? ops->policy_test(sbi, 0) : false;
	if (pass) {
		if (ops->segment_manager)
			(void)ops->segment_manager(sbi, 0, 0);
	} else {
		if (ops->list_drain)
			(void)ops->list_drain(sbi, 0xE38);
	}

	/* gc_mode == 3 bypasses the seven-field non-mode-3 ladder. */
	if (x683_sbi_u32(sbi, X683_SBI_MODE_OFF) != 3) {
		if (!x683_seven_guard_all_zero(sbi))
			return 0;

		if (ops->fixed_point_guard && !ops->fixed_point_guard(sbi))
			return 0;
	}

	/* Secondary policy ladder: selector 1 then selector 3. */
	if (ops->policy_test && !ops->policy_test(sbi, 1))
		return 0;
	if (ops->policy_test && !ops->policy_test(sbi, 3))
		return 0;

	/* Dirty/reservation + second fixed-point guard are opaque predicates. */
	if (ops->secondary_dirty_guard && !ops->secondary_dirty_guard(sbi))
		return 0;
	if (ops->time_current_guard && !ops->time_current_guard(sbi))
		return 0;

	/* Terminal/post-GC preparation is gated by mount option byte bit 7. */
	if (!(x683_sbi_u8(sbi, X683_SBI_MOUNTBYTE_OFF) & BIT(7)))
		return 0;

	if (ops->terminal_0x3e1014)
		ops->terminal_0x3e1014(stack_object);
	if (ops->terminal_0x34e224)
		(void)ops->terminal_0x34e224(stack_object, 1);
	if (ops->terminal_0x3e1558)
		ops->terminal_0x3e1558(stack_object);
	if (ops->terminal_0x341250)
		(void)ops->terminal_0x341250(sbi->sb, 1);
	if (ops->terminal_stat_increment)
		ops->terminal_stat_increment(sbi);

	(void)X683_SBI_GUARD6_OFF;
	(void)X683_SBI_MOUNTOPT_OFF;
	(void)X683_SBI_STAT_OFF;
	return 0;
}
