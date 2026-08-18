/*
 * X683/H694 Transsion GC policy/orchestration reconstruction.
 * Binary-derived from stock Image at 0x366cd4.
 * Not proprietary-source recovery and not yet build-proven.
 */
#include "f2fs.h"

/* Opaque vendor helpers: symbolic names intentionally not invented. */
extern bool x683_policy_test(struct f2fs_sb_info *sbi, unsigned int selector);
extern bool x683_policy_gate_0x80(struct f2fs_sb_info *sbi);
extern int x683_policy_helper_455(struct f2fs_sb_info *sbi, unsigned int arg);
extern int x683_segment_manager_helper(struct f2fs_sb_info *sbi,
					unsigned int a, unsigned int b);
extern int x683_list_drain_helper(struct f2fs_sb_info *sbi,
					unsigned int arg);
extern int x683_fs_balance_helper(struct super_block *sb, unsigned int arg);

/*
 * This is deliberately a control-flow reconstruction. The individual
 * arithmetic gates below retain raw SBI offsets until their exact source
 * member names are proven from the vendor tree.
 */
bool x683_tran_gc_policy(struct f2fs_sb_info *sbi)
{
	if (!sbi)
		return false;

	if (*(u32 *)((char *)sbi + 0x48) & (1U << 3))
		return false;

	if (!x683_policy_test(sbi, 4))
		(void)x683_policy_gate_0x80(sbi);

	if (!x683_policy_test(sbi, 1))
		(void)x683_policy_helper_455(sbi, 455);

	if (x683_policy_test(sbi, 0))
		(void)x683_segment_manager_helper(sbi, 0, 0);
	else
		(void)x683_list_drain_helper(sbi, 0xE38);

	/* mode == 3 selects the urgent branch before the seven-field guard. */
	if (sbi->gc_mode != 3) {
		const unsigned int guard_offs[] = {
			0x44c, 0x450, 0x454, 0x448,
			0x444, 0x45c, 0x458,
		};
		bool active = false;
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(guard_offs); ++i) {
			if (*(u32 *)((char *)sbi + guard_offs[i])) {
				active = true;
				break;
			}
		}

		/* Exact fixed-point guard is intentionally kept in the binary notes. */
		if (!active) {
			/* unresolved vendor arithmetic/threshold path */
		}
	}

	if (!x683_policy_test(sbi, 1))
		return false;
	if (!x683_policy_test(sbi, 3))
		return false;

	/* Remaining dirty/reservation and fixed-point guards are binary-backed
	 * but their vendor symbolic fields are still unresolved. */

	if (!(*(u8 *)((char *)sbi + 0x4b9) & (1U << 7)))
		return true;

	/* Exact terminal helper chain: 0x3e1014 -> 0x34e224 -> 0x3e1558
	 * followed by 0x341250(sbi->sb, 1). */
	return x683_fs_balance_helper(sbi->sb, 1) == 0;
}
