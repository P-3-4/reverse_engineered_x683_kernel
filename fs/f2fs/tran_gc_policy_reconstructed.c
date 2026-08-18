/*
 * X683/H694 Transsion GC policy/orchestration reconstruction.
 * Binary-derived from stock Image at 0x366cd4.
 * Not proprietary-source recovery and not build-proven.
 *
 * IMPORTANT: this file records control-flow/data-flow only. It does not
 * fabricate return semantics for opaque vendor helpers.
 */
#include "f2fs.h"

extern bool x683_policy_test(struct f2fs_sb_info *sbi, unsigned int selector);
extern bool x683_policy_gate_0x80(struct f2fs_sb_info *sbi);
extern int x683_policy_helper_455(struct f2fs_sb_info *sbi, unsigned int arg);
extern int x683_segment_manager_helper(struct f2fs_sb_info *sbi,
					unsigned int a, unsigned int b);
extern int x683_list_drain_helper(struct f2fs_sb_info *sbi,
					unsigned int arg);
extern int x683_fs_balance_helper(struct super_block *sb, unsigned int arg);

/*
 * Binary-backed control-flow skeleton. The exact helper return values and
 * several arithmetic predicates remain intentionally external.
 */
void x683_tran_gc_policy_step(struct f2fs_sb_info *sbi)
{
	if (!sbi)
		return;

	if (*(u32 *)((char *)sbi + 0x48) & (1U << 3))
		return;

	/* 0x366d00: selector 4 */
	(void)x683_policy_test(sbi, 4);

	/* 0x366d10: selector 0x80 gate is reached on selector-4 false. */
	(void)x683_policy_gate_0x80(sbi);

	/* 0x366d1c / 0x366d2c: selector 1 and helper argument 455. */
	(void)x683_policy_test(sbi, 1);
	(void)x683_policy_helper_455(sbi, 455);

	/* 0x366d38 / 0x366d4c / 0x366d5c. The branch decision is helper-0's
	 * actual bit-0 result; the result itself is not modeled here. */
	(void)x683_policy_test(sbi, 0);
	(void)x683_segment_manager_helper(sbi, 0, 0);
	(void)x683_list_drain_helper(sbi, 0xE38);

	/* mode == 3 selects a separate urgent branch before the seven-field
	 * non-urgent guard at +0x444..+0x45c. */
	if (sbi->gc_mode != 3) {
		static const unsigned int guard_offs[] = {
			0x44c, 0x450, 0x454, 0x448,
			0x444, 0x45c, 0x458,
		};
		unsigned int i;
		for (i = 0; i < ARRAY_SIZE(guard_offs); ++i)
			(void)*(u32 *)((char *)sbi + guard_offs[i]);
	}

	/* Secondary vendor gates: selectors 1 and 3. */
	(void)x683_policy_test(sbi, 1);
	(void)x683_policy_test(sbi, 3);

	/* Final terminal chain is only entered when the stock bit-7 gate at
	 * sbi+0x4b9 is set. */
	if (*(u8 *)((char *)sbi + 0x4b9) & (1U << 7))
		(void)x683_fs_balance_helper(sbi->sb, 1);
}
