/*
 * X683/H694 semantic GC policy reconstruction.
 * Binary-derived from stock 0x366cd4..0x366f2c.
 * Not proprietary source recovery; not build-proven.
 */
#include "f2fs.h"

#define X683_STAT_OFF 0x568
#define X683_MODE_OFF 0x534
#define X683_MOUNT_OPT_BYTE_OFF 0x4b9

/* X683 f2fs_sb_info fields mirrored to f2fs_stat_info by 0x375ed8..0x375f0c. */
#define X683_NR_WB_CP_DATA 0x444
#define X683_NR_WB_DATA    0x448
#define X683_NR_RD_DATA    0x44c
#define X683_NR_RD_NODE    0x450
#define X683_NR_RD_META    0x454
#define X683_NR_DIO_WRITE  0x458
#define X683_NR_DIO_READ   0x45c

static inline u32 sbi_u32(const struct f2fs_sb_info *sbi, unsigned int off)
{
	return *(const u32 *)((const char *)sbi + off);
}

static inline bool x683_io_active(const struct f2fs_sb_info *sbi)
{
	return sbi_u32(sbi, X683_NR_WB_CP_DATA) ||
		sbi_u32(sbi, X683_NR_WB_DATA) ||
		sbi_u32(sbi, X683_NR_RD_DATA) ||
		sbi_u32(sbi, X683_NR_RD_NODE) ||
		sbi_u32(sbi, X683_NR_RD_META) ||
		sbi_u32(sbi, X683_NR_DIO_WRITE) ||
		sbi_u32(sbi, X683_NR_DIO_READ);
}

static inline void x683_bg_cp_count_inc(struct f2fs_sb_info *sbi)
{
	struct f2fs_stat_info *stat =
		*(struct f2fs_stat_info **)((char *)sbi + X683_STAT_OFF);

	if (stat)
		*(u32 *)((char *)stat + 0x16c) += 1; /* bg_cp_count */
}

/*
 * Semantic CFG scaffold for 0x366cd4.
 * Opaque vendor helper names remain callbacks until exact source attribution.
 * The shared time reference is jiffies_64 at Image+0x16c6980.
 */
struct x683_gc_policy_ops {
	bool (*policy_test)(struct f2fs_sb_info *, unsigned int selector);
	void (*gate_0x80)(struct f2fs_sb_info *, unsigned int arg);
	void (*helper_455)(struct f2fs_sb_info *, unsigned int arg);
	int (*heavy_gc_path)(struct f2fs_sb_info *, unsigned int, unsigned int);
	int (*list_drain)(struct f2fs_sb_info *, unsigned int);
	bool (*active_fixed_point_guard)(struct f2fs_sb_info *);
	bool (*clean_branch_guard)(struct f2fs_sb_info *);
	bool (*jiffies_guard)(struct f2fs_sb_info *);
	void (*gc_tls_init)(void *gc_list_stack_object);
	void (*gc_tls_reset)(void *gc_list_stack_object);
	int (*balance_fs)(struct f2fs_sb_info *, bool); /* strong f2fs_balance_fs candidate */
	int (*super_sync)(struct super_block *, int);   /* medium f2fs_sync_fs candidate */
};

int x683_gc_policy_semantic(struct f2fs_sb_info *sbi,
		const struct x683_gc_policy_ops *ops, void *gc_list_stack_object)
{
	if (!sbi || !ops)
		return -EINVAL;

	/* 0x366cf0 */
	if (sbi_u32(sbi, 0x48) & BIT(3))
		return 0;

	/* Selector/helper ladder. */
	if (ops->policy_test && !ops->policy_test(sbi, 4) && ops->gate_0x80)
		ops->gate_0x80(sbi, 0x80);
	if (ops->policy_test && !ops->policy_test(sbi, 1) && ops->helper_455)
		ops->helper_455(sbi, 455);

	if (ops->policy_test && ops->policy_test(sbi, 0)) {
		if (ops->heavy_gc_path)
			ops->heavy_gc_path(sbi, 0, 0);
	} else if (ops->list_drain) {
		ops->list_drain(sbi, 0xe38);
	}

	/* gc_mode 3 bypasses the non-urgent I/O discriminator. */
	if (sbi_u32(sbi, X683_MODE_OFF) != 3 &&
	    (x683_io_active(sbi) && ops->active_fixed_point_guard)) {
		if (!ops->active_fixed_point_guard(sbi))
			return 0;
	}

	/* all-seven-zero clean branch. */
	if (sbi_u32(sbi, X683_MODE_OFF) != 3 && !x683_io_active(sbi) &&
	    ops->clean_branch_guard && !ops->clean_branch_guard(sbi)) {
		if (ops->active_fixed_point_guard &&
		    !ops->active_fixed_point_guard(sbi))
			return 0;
	}

	/* Shared secondary stage. */
	if (ops->policy_test && !ops->policy_test(sbi, 1))
		goto terminal;
	if (ops->policy_test && !ops->policy_test(sbi, 3))
		goto terminal;
	if (ops->active_fixed_point_guard && !ops->active_fixed_point_guard(sbi))
		goto terminal;

	/* 0x366e64: derived value compared directly against jiffies. */
	if (ops->jiffies_guard && !ops->jiffies_guard(sbi))
		return 0;

terminal:
	/* 0x366e7c..0x366e9c */
	if ((*(const u8 *)((const char *)sbi + X683_MOUNT_OPT_BYTE_OFF)) & BIT(7)) {
		if (ops->gc_tls_init)
			ops->gc_tls_init(gc_list_stack_object);
		if (ops->balance_fs)
			ops->balance_fs(sbi, true);
		if (ops->gc_tls_reset)
			ops->gc_tls_reset(gc_list_stack_object);
	}

	/* 0x366ea0: unconditional after reaching terminal path. */
	if (ops->super_sync)
		ops->super_sync(sbi->sb, 1);

	/* 0x366eac..0x366eb8 */
	x683_bg_cp_count_inc(sbi);
	return 0;
}
