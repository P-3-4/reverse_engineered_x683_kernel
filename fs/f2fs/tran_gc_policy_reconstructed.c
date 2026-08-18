/*
 * X683/H694 Transsion GC policy/orchestration reconstruction.
 * Binary-derived from stock Image 0x366cd4..0x366f2c.
 * Not proprietary source recovery. Not build-proven.
 *
 * This version follows the fresh-byte CFG from the supplied stock boot.img.
 * Opaque helper identities remain deliberately unnamed.
 */
#include "f2fs.h"

#define X683_SBI_MODE_OFF       0x534
#define X683_SBI_MOUNTBYTE_OFF  0x4b9
#define X683_SBI_STAT_OFF       0x568

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

struct x683_policy_ops {
	bool (*policy_test)(struct f2fs_sb_info *sbi, unsigned int selector);
	bool (*gate_0x80)(struct f2fs_sb_info *sbi);
	int (*helper_455)(struct f2fs_sb_info *sbi, unsigned int arg);
	int (*segment_manager)(struct f2fs_sb_info *sbi,
		unsigned int a, unsigned int b);
	int (*list_drain)(struct f2fs_sb_info *sbi, unsigned int arg);

	/* 0x366da4..0x366de0 guarded-path predicate. */
	bool (*active_capacity_guard)(struct f2fs_sb_info *sbi);

	/* 0x366ee0..0x366f28 clean-path predicate. */
	bool (*clean_path_guard)(struct f2fs_sb_info *sbi);

	/* Terminal helpers. Exact source identities remain unresolved. */
	void (*terminal_tls_init)(void *stack_object);
	int (*terminal_global_dispatch)(struct f2fs_sb_info *sbi,
		unsigned int arg);
	void (*terminal_tls_reset)(void *stack_object);
	int (*terminal_fs_helper)(struct super_block *sb, unsigned int arg);
	void (*terminal_stat_increment)(struct f2fs_sb_info *sbi);
};

/*
 * Exact seven-field branch discriminator.
 *
 * The binary does NOT require all seven fields to be zero for normal
 * execution. Any nonzero field in the first six, or the final +0x458 field,
 * selects the active guarded path at 0x366da4. Only the all-zero case takes
 * the alternate clean-path tail at 0x366ee0.
 */
static bool x683_any_policy_guard_set(const struct f2fs_sb_info *sbi)
{
	static const unsigned int offs[] = {
		0x44c, 0x450, 0x454, 0x448,
		0x444, 0x45c, 0x458,
	};
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(offs); ++i) {
		if (x683_sbi_u32(sbi, offs[i]))
			return true;
	}
	return false;
}

/*
 * 0x366cd4 entry-to-active-path predicate.
 *
 * Binary arithmetic:
 *   scaled = obj[0x4] * obj[0x18] * 0x51EB851F >> 37
 *   if obj[0x80] >= scaled -> continue secondary path
 *   else require sbi[0x434] >= 8 * sbi[0x3dc]
 *   otherwise return.
 */
static bool x683_active_guard_binary_semantics(struct f2fs_sb_info *sbi)
{
	const u8 *base = (const u8 *)sbi;
	void *obj = *(void **)(base + 0x70);
	u32 a, b, current, scaled;
	s64 lhs;

	if (!obj)
		return true;

	a = *(u32 *)((u8 *)obj + 0x04);
	b = *(u32 *)((u8 *)obj + 0x18);
	current = *(u32 *)((u8 *)obj + 0x80);
	scaled = (u32)(((u64)a * (u64)b * 0x51EB851FULL) >> 37);

	if (current >= scaled)
		return true;

	lhs = *(s32 *)(base + 0x434);
	return lhs >= ((s64)(u32)*(u32 *)(base + 0x3dc) << 3);
}

/*
 * 0x366ee0..0x366f28 clean-path semantics.
 *
 * If the auxiliary manager/list state requests escalation, branch back to
 * 0x366da4. Otherwise compare 250*sbi[0x1d0] + sbi[0x1a0] against the same
 * vendor global used by the active path. A signed value below the global
 * proceeds to the secondary policy stage at 0x366de4; a nonnegative result
 * re-enters the active path.
 */
static bool x683_clean_path_to_secondary(struct f2fs_sb_info *sbi)
{
	const u8 *base = (const u8 *)sbi;
	void *obj = *(void **)(base + 0x80);

	if (obj) {
		void *child = *(void **)((u8 *)obj + 0xa0);
		if (child && *(u32 *)((u8 *)child + 0x2090))
			return false;

		void *list = *(void **)((u8 *)obj + 0x98);
		if (list && *(u32 *)((u8 *)list + 0x24))
			return false;
	}

	/* Image +0x16c6980: recovered from ADRP 0x16c6000 + LDR #0x980. */
	const u8 *image = (const u8 *)(uintptr_t)0;
	const u64 policy_deadline = *(const u64 *)(image + 0x16c6980);
	s64 value = (s64)*(u64 *)(base + 0x1d0) * 250 +
		(s64)*(u64 *)(base + 0x1a0);

	return value < (s64)policy_deadline;
}

/*
 * Exact 0x366cd4..0x366f2c control-flow reconstruction.
 *
 * Return value here is only a reconstruction status value; the stock binary's
 * caller-visible semantics are not promoted to a proprietary source API.
 */
int x683_tran_gc_policy_step(struct f2fs_sb_info *sbi,
		const struct x683_policy_ops *ops,
		void *stack_object)
{
	bool result;

	if (!sbi || !ops)
		return -EINVAL;

	/* 0x366cf0: sbi + 0x48 bit 3 -> clean return. */
	if (x683_sbi_u32(sbi, 0x48) & BIT(3))
		return 0;

	/* First selector ladder. */
	if (ops->policy_test && !ops->policy_test(sbi, 4)) {
		if (ops->gate_0x80)
			(void)ops->gate_0x80(sbi);
	}

	if (ops->policy_test && !ops->policy_test(sbi, 1)) {
		if (ops->helper_455)
			(void)ops->helper_455(sbi, 455);
	}

	/* Selector 0 chooses exactly one helper. */
	result = ops->policy_test ? ops->policy_test(sbi, 0) : false;
	if (result) {
		if (ops->segment_manager)
			(void)ops->segment_manager(sbi, 0, 0);
	} else {
		if (ops->list_drain)
			(void)ops->list_drain(sbi, 0xe38);
	}

	/* 0x366d60: mode 3 bypasses the seven-field discriminator. */
	if (x683_sbi_u32(sbi, X683_SBI_MODE_OFF) != 3) {
		if (x683_any_policy_guard_set(sbi)) {
			/* 0x366da4 active path. */
			if (ops->active_capacity_guard &&
				!ops->active_capacity_guard(sbi))
				return 0;
		} else {
			/* 0x366ee0 clean path. */
			if (ops->clean_path_guard &&
				!ops->clean_path_guard(sbi)) {
				/* State requests escalation back into 0x366da4. */
				if (ops->active_capacity_guard &&
					!ops->active_capacity_guard(sbi))
					return 0;
			}
		}
	}

	/* Shared secondary policy stage at 0x366de4. */
	result = ops->policy_test ? ops->policy_test(sbi, 1) : false;
	if (!result)
		goto terminal;

	result = ops->policy_test ? ops->policy_test(sbi, 3) : false;
	if (!result)
		goto terminal;

	/* Nested reservation/dirty comparison. */
	{
		const u8 *base = (const u8 *)sbi;
		void *obj = *(void **)(base + 0x80);
		if (obj) {
			void *nested = *(void **)((u8 *)obj + 0x10);
			u32 lhs = *(u32 *)((u8 *)obj + 0x64);
			if (nested && *(u32 *)((u8 *)nested + 0x84) > lhs)
				goto terminal;
		}
	}

	if (ops->active_capacity_guard &&
		!ops->active_capacity_guard(sbi))
		goto terminal;

	/* Second 0x434 vs 8*0x3dc gate is terminal-bound, not an early return. */
	if (*(s32 *)((u8 *)sbi + 0x434) <
			((s64)(u32)*(u32 *)((u8 *)sbi + 0x3dc) << 3))
		goto terminal;

	/* 250*sbi[0x1c8] + sbi[0x198] >= global -> direct clean return. */
	{
		const u8 *base = (const u8 *)sbi;
		const u64 policy_deadline = *(const u64 *)(uintptr_t)0x16c6980;
		s64 value = (s64)*(u64 *)(base + 0x1c8) * 250 +
			(s64)*(u64 *)(base + 0x198);
		if (value >= (s64)policy_deadline)
			return 0;
	}

terminal:
	/* 0x366e7c terminal path. +0x4b9 bit7 gates only the TLS helpers. */
	if (x683_sbi_u8(sbi, X683_SBI_MOUNTBYTE_OFF) & BIT(7)) {
		if (ops->terminal_tls_init)
			ops->terminal_tls_init(stack_object);
		if (ops->terminal_global_dispatch)
			(void)ops->terminal_global_dispatch(sbi, 1);
		if (ops->terminal_tls_reset)
			ops->terminal_tls_reset(stack_object);
	}

	/* 0x366ea0: always executed on the terminal path. */
	if (ops->terminal_fs_helper)
		(void)ops->terminal_fs_helper(sbi->sb, 1);
	if (ops->terminal_stat_increment)
		ops->terminal_stat_increment(sbi);

	return 0;
}

/*
 * The two helpers below expose the binary semantics without pretending to
 * resolve the original vendor symbol names.
 */
bool x683_tran_gc_active_guard(struct f2fs_sb_info *sbi)
{
	return x683_active_guard_binary_semantics(sbi);
}

bool x683_tran_gc_clean_path(struct f2fs_sb_info *sbi)
{
	return x683_clean_path_to_secondary(sbi);
}
