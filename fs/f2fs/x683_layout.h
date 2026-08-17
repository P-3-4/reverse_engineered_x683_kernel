/* SPDX-License-Identifier: GPL-2.0 */
/*
 * X683/H694 F2FS binary layout reconstruction.
 *
 * These helpers intentionally use recovered offsets until the exact vendor
 * f2fs_sb_info definition is matched. They are not an assertion that the
 * original proprietary source used offset casts.
 */
#ifndef __X683_F2FS_LAYOUT_H__
#define __X683_F2FS_LAYOUT_H__

#include <linux/types.h>

struct f2fs_sb_info;

#define X683_SBI_OFF_LOG_BLOCKS_PER_SEG   0x3d8
#define X683_SBI_OFF_BLOCKS_PER_SEG       0x3dc
#define X683_SBI_OFF_SEGS_PER_SEC         0x3e0
#define X683_SBI_OFF_USER_BLOCK_COUNT     0x408
#define X683_SBI_OFF_TOTAL_VALID_BLOCKS   0x410
#define X683_SBI_OFF_DISCARD_BLKS         0x418
#define X683_SBI_OFF_LAST_VALID_BLOCKS    0x420
#define X683_SBI_OFF_RESERVED_BLOCKS      0x428
#define X683_SBI_OFF_CURRENT_RESERVED     0x430
#define X683_SBI_OFF_UNUSABLE_BLOCKS      0x438
#define X683_SBI_OFF_NQUOTA_FILES         0x440
#define X683_SBI_OFF_MOUNT_OPT            0x4b8
#define X683_SBI_OFF_GC_MODE              0x534

static inline u32 x683_sbi_read_u32(const struct f2fs_sb_info *sbi,
                                     unsigned int off)
{
        return *(const u32 *)((const char *)sbi + off);
}

static inline void x683_sbi_write_u32(struct f2fs_sb_info *sbi,
                                      unsigned int off, u32 value)
{
        *(u32 *)((char *)sbi + off) = value;
}

static inline u32 x683_log_blocks_per_seg(const struct f2fs_sb_info *sbi)
{
        return x683_sbi_read_u32(sbi, X683_SBI_OFF_LOG_BLOCKS_PER_SEG);
}

static inline u32 x683_blocks_per_seg(const struct f2fs_sb_info *sbi)
{
        return x683_sbi_read_u32(sbi, X683_SBI_OFF_BLOCKS_PER_SEG);
}

static inline u32 x683_segs_per_sec(const struct f2fs_sb_info *sbi)
{
        return x683_sbi_read_u32(sbi, X683_SBI_OFF_SEGS_PER_SEC);
}

static inline u32 x683_user_block_count(const struct f2fs_sb_info *sbi)
{
        return x683_sbi_read_u32(sbi, X683_SBI_OFF_USER_BLOCK_COUNT);
}

static inline u32 x683_mount_opt(const struct f2fs_sb_info *sbi)
{
        return x683_sbi_read_u32(sbi, X683_SBI_OFF_MOUNT_OPT);
}

/* Bit 14 of mount_opt.opt correlates with the stock FORCE_FG_GC option. */
static inline u32 x683_gc_sync(const struct f2fs_sb_info *sbi)
{
        return (x683_mount_opt(sbi) >> 14) & 1U;
}

static inline u32 x683_gc_mode(const struct f2fs_sb_info *sbi)
{
        return x683_sbi_read_u32(sbi, X683_SBI_OFF_GC_MODE);
}

static inline void x683_set_gc_mode(struct f2fs_sb_info *sbi, u32 mode)
{
        x683_sbi_write_u32(sbi, X683_SBI_OFF_GC_MODE, mode);
}

#endif
