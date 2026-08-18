/*
 * X683/H694 Transsion GC policy/orchestration reconstruction.
 * Binary-derived from stock Image 0x366cd4..0x366f2c.
 * Not proprietary source recovery. Not build-proven.
 *
 * This is intentionally a CFG/data-flow scaffold. Opaque helper identities
 * and image-absolute globals are kept as callbacks instead of fabricated
 * source-level symbols.
 */
#include "f2fs.h"

#define X683_SBI_MODE_OFF      0x534
#define X683_SBI_MOUNTBYTE_OFF 0x4b9
#define X683_SBI_STAT_OFF      0x568

struct x683_policy_ops {
	bool (*policy_test)(struct f2fs_sb_info *sbi, unsigned int selector);
	bool (*gate_0x80)(struct f2fs_sb_info *sbi);
	int (*helper_455)(struct f2fs_sb_info *sbi, unsigned int arg);
	int (*segment_manager)(struct f2fs_sb_info *sbi,
		unsigned int a, unsigned int b);
	int (*list_drain)(struct f2fs_sb_info *sbi, unsigned int arg);

	/* 0x366da4 active-path predicate. */
	bool (*active_capacity_guard)(struct f2fs_sb_info *sbi);

	/* 0x366ee0 clean-path predicate. Returns true only when control should
	 * continue to the shared 0x366de4 stage. */
	bool (*clean_path_continue)(struct f2fs_sb_info *sbi);

	/* Shared-stage time/current predicate. */
	bool (*shared_time_guard)(struct f2fs_sb_info *sbi);

	/* Terminal helpers. Exact source identities remain unresolved. */
	void (*terminal_tls_init)(void *stack_object);
	int (*terminal_global_dispatch)(struct f2fs_sb_info *sbi,
		unsigned int arg);
	void (*terminal_tls_reset)(void *stack_object);
	int (*terminal_fs_helper)(struct super_block *sb, unsigned int arg);
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
 * Fresh-byte result for the seven SBI fields:
 *
 *   0x44c, 0x450, 0x454, 0x448, 0x444, 0x45c, 0x458
 *
 * Any nonzero field selects 0x366da4. Only the all-zero case selects
 * 0x366ee0. This is a discriminator, not an all-zero prerequisite.
 */
static bool x683_any_guard_set(const struct f2fs_sb_info *sbi)
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

int x683_tran_gc_policy_step(struct f2fs_sb_info *sbi,
		const struct x683_policy_ops *ops,
		void *stack_object)
{
	bool result;

	if (!sbi || !ops)
		return -EINVAL;

	/* 0x366cf0: sbi + 0x48 bit 3 -> return. */
	if (x683_sbi_u32(sbi, 0x48) & BIT(3))
		return 0;

	/* 0x366cf8..0x366d5c: selector/helper ladder. */
	if (ops->policy_test && !ops->policy_test(sbi, 4)) {
		if (ops->gate_0x80)
			(void)ops->gate_0x80(sbi);
	}

	if (ops->policy_test && !ops->policy_test(sbi, 1)) {
		if (ops->helper_455)
			(void)ops->helper_455(sbi, 455);
	}

	result = ops->policy_test ? ops->policy_test(sbi, 0) : false;
	if (result) {
		if (ops->segment_manager)
			(void)ops->segment_manager(sbi, 0, 0);
	} else {
		if (ops->list_drain)
			(void)ops->list_drain(sbi, 0xe38);
	}

	/* 0x366d60: mode 3 skips the seven-field discriminator. */
	if (x683_sbi_u32(sbi, X683_SBI_MODE_OFF) != 3) {
		if (x683_any_guard_set(sbi)) {
			/* 0x366da4 active path. */
			if (ops->active_capacity_guard &&
				!ops->active_capacity_guard(sbi))
				return 0;
		} else {
			/* 0x366ee0 clean/alternate path. */
			if (ops->clean_path_continue &&
				!ops->clean_path_continue(sbi)) {
				/* The binary escalates back to 0x366da4. */
				if (ops->active_capacity_guard &&
					!ops->active_capacity_guard(sbi))
					return 0;
			}
		}
	}

	/* 0x366de4 shared secondary stage. */
	result = ops->policy_test ? ops->policy_test(sbi, 1) : false;
	if (!result)
		goto terminal;

	result = ops->policy_test ? ops->policy_test(sbi, 3) : false;
	if (!result)
		goto terminal;

	/* Nested reservation/dirty comparison at 0x366e04..0x366e18 is kept as
	 * part of the opaque active/shared predicate because its exact source
	 * structure names are unresolved. */
	if (ops->active_capacity_guard && !ops->active_capacity_guard(sbi))
		goto terminal;

	/* The final 250*x+y versus Image+0x16c6980 comparison is a direct-return
	 * gate. Keep it external until the image-absolute global is integrated
	 * into a real X683 source mapping. */
	if (ops->shared_time_guard && !ops->shared_time_guard(sbi))
		return 0;

terminal:
	/* 0x366e7c: bit7 controls only the TLS/list helper trio. */
	if (x683_sbi_u8(sbi, X683_SBI_MOUNTBYTE_OFF) & BIT(7)) {
		if (ops->terminal_tls_init)
			ops->terminal_tls_init(stack_object);
		if (ops->terminal_global_dispatch)
			(void)ops->terminal_global_dispatch(sbi, 1);
		if (ops->terminal_tls_reset)
			ops->terminal_tls_reset(stack_object);
	}

	/* 0x366ea0: always executed after reaching the terminal path. */
	if (ops->terminal_fs_helper)
		(void)ops->terminal_fs_helper(sbi->sb, 1);
	if (ops->terminal_stat_increment)
		ops->terminal_stat_increment(sbi);

	(void)X683_SBI_STAT_OFF;
	return 0;
}
