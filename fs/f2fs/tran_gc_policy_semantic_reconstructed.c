/*
 * X683/H694 semantic reconstruction of f2fs_balance_fs_bg().
 *
 * Binary entry: Image+0x366cd4
 * Exact kallsyms symbol: f2fs_balance_fs_bg
 * Binary-derived; not proprietary source recovery and not build-proven.
 */
#include "f2fs.h"

#define X683_STAT_OFF 0x568
#define X683_MODE_OFF 0x534
#define X683_MOUNT_OPT_BYTE_OFF 0x4b9

/* X683 f2fs_sb_info fields mirrored into the statistics snapshot. */
#define X683_NR_WB_CP_DATA 0x444
#define X683_NR_WB_DATA    0x448
#define X683_NR_RD_DATA    0x44c
#define X683_NR_RD_NODE   0x450
#define X683_NR_RD_META   0x454
#define X683_NR_DIO_WRITE 0x458
#define X683_NR_DIO_READ  0x45c

static inline u32 x683_sbi_u32(const struct f2fs_sb_info *sbi, unsigned int off)
{
	return *(const u32 *)((const char *)sbi + off);
}

static inline bool x683_io_active(const struct f2fs_sb_info *sbi)
{
	return x683_sbi_u32(sbi, X683_NR_WB_CP_DATA) ||
		x683_sbi_u32(sbi, X683_NR_WB_DATA) ||
		x683_sbi_u32(sbi, X683_NR_RD_DATA) ||
		x683_sbi_u32(sbi, X683_NR_RD_NODE) ||
		x683_sbi_u32(sbi, X683_NR_RD_META) ||
		x683_sbi_u32(sbi, X683_NR_DIO_WRITE) ||
		x683_sbi_u32(sbi, X683_NR_DIO_READ);
}

static inline void x683_bg_cp_count_inc(struct f2fs_sb_info *sbi)
{
	struct f2fs_stat_info *stat =
		*(struct f2fs_stat_info **)((char *)sbi + X683_STAT_OFF);

	if (stat)
		*(u32 *)((char *)stat + 0x16c) += 1; /* bg_cp_count */
}

/*
 * X683-specific predicates whose source-level names are not yet recovered.
 * The surrounding helper identities are exact kallsyms matches.
 */
struct x683_balance_bg_ops {
	bool (*vendor_post_mem_policy)(struct f2fs_sb_info *, unsigned int);
	bool (*vendor_dirty_policy)(struct f2fs_sb_info *);
	bool (*vendor_secondary_policy)(struct f2fs_sb_info *);
	bool (*vendor_jiffies_policy)(struct f2fs_sb_info *);
};

/*
 * Semantic CFG for 0x366cd4..0x366f2c.
 * Exact helper identities now come from x683_kallsyms.txt:
 *
 *   0x35cc18 = f2fs_available_free_memory
 *   0x35d22c = f2fs_try_to_free_nats
 *   0x362c40 = f2fs_build_free_nids
 *   0x363288 = f2fs_try_to_free_nids
 *   0x373108 = f2fs_shrink_extent_tree
 *   0x34e224 = f2fs_sync_dirty_inodes
 *   0x341250 = f2fs_sync_fs
 *   0x3e1014 = blk_start_plug
 *   0x3e1558 = blk_finish_plug
 */
int x683_f2fs_balance_fs_bg_semantic(struct f2fs_sb_info *sbi,
					 bool from_bg,
					 const struct x683_balance_bg_ops *ops)
{
	if (!sbi)
		return -EINVAL;

	/* Image+0x366cf0: SBI/POR-style early guard. */
	if (x683_sbi_u32(sbi, 0x48) & BIT(3))
		return 0;

	/* Stock 4.14 structure, with X683 selector values not yet promoted. */
	if (ops && ops->vendor_post_mem_policy &&
	    !ops->vendor_post_mem_policy(sbi, 4))
		f2fs_shrink_extent_tree(sbi, 0);

	if (ops && ops->vendor_post_mem_policy &&
	    !ops->vendor_post_mem_policy(sbi, 1))
		f2fs_try_to_free_nats(sbi, 455);

	if (ops && ops->vendor_post_mem_policy &&
	    !ops->vendor_post_mem_policy(sbi, 0))
		f2fs_try_to_free_nids(sbi, 0);
	else
		f2fs_build_free_nids(sbi, false, false);

	/*
	 * The X683 binary inserts additional dirty-I/O/fixed-point policy here.
	 * Exact vendor predicates remain intentionally opaque until their field
	 * and selector semantics are recovered from the binary.
	 */
	if (ops && ops->vendor_dirty_policy &&
	    !ops->vendor_dirty_policy(sbi))
		return 0;

	if (sbi->gc_mode != 3 && x683_io_active(sbi)) {
		if (ops && ops->vendor_secondary_policy &&
		    !ops->vendor_secondary_policy(sbi))
			return 0;
	}

	if (sbi->gc_mode != 3 && !x683_io_active(sbi)) {
		if (ops && ops->vendor_secondary_policy &&
		    !ops->vendor_secondary_policy(sbi))
			return 0;
	}

	/* X683 time/policy gate, shared with the original balance_fs_bg flow. */
	if (ops && ops->vendor_jiffies_policy &&
	    !ops->vendor_jiffies_policy(sbi))
		return 0;

	/* DATA_FLUSH path: the mount-option byte maps to the X683 DATA_FLUSH bit. */
	if ((*(const u8 *)((const char *)sbi + X683_MOUNT_OPT_BYTE_OFF)) & BIT(7)) {
		struct blk_plug plug;

		mutex_lock(&sbi->flush_lock);
		blk_start_plug(&plug);
		f2fs_sync_dirty_inodes(sbi, FILE_INODE);
		blk_finish_plug(&plug);
		mutex_unlock(&sbi->flush_lock);
	}

	f2fs_sync_fs(sbi->sb, true);
	x683_bg_cp_count_inc(sbi);
	(void)from_bg;
	return 0;
}
