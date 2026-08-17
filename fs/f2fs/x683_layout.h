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

#define X683_SBI_OFF_SM_INFO             0x080
#define X683_SBI_OFF_LOG_BLOCKS_PER_SEG  0x3d8
#define X683_SBI_OFF_BLOCKS_PER_SEG      0x3dc
#define X683_SBI_OFF_SEGS_PER_SEC        0x3e0
#define X683_SBI_OFF_USER_BLOCK_COUNT    0x408
#define X683_SBI_OFF_TOTAL_VALID_BLOCKS  0x410
#define X683_SBI_OFF_DISCARD_BLKS        0x418
#define X683_SBI_OFF_LAST_VALID_BLOCKS   0x420
#define X683_SBI_OFF_RESERVED_BLOCKS     0x428
#define X683_SBI_OFF_CURRENT_RESERVED    0x430
#define X683_SBI_OFF_UNUSABLE_BLOCKS     0x438
#define X683_SBI_OFF_NQUOTA_FILES        0x440
#define X683_SBI_OFF_MOUNT_OPT           0x4b8
#define X683_SBI_OFF_GC_MUTEX            0x508
#define X683_SBI_OFF_GC_THREAD           0x528
#define X683_SBI_OFF_CUR_VICTIM_SEC      0x530
#define X683_SBI_OFF_GC_MODE             0x534
#define X683_SBI_OFF_NEXT_VICTIM_SEG     0x538
#define X683_SBI_OFF_SKIPPED_ATOMIC      0x540
#define X683_SBI_OFF_SKIPPED_GC_RWSEM    0x550
#define X683_SBI_OFF_GC_PIN_THRESHOLD    0x558
#define X683_SBI_OFF_MAX_VICTIM_SEARCH   0x560
#define X683_SBI_OFF_MIGRATION_GRAN      0x564
#define X683_SBI_OFF_STAT_INFO           0x568

/*
 * Structural candidates from the historical CONFIG_F2FS_STAT_FS layout.
 *
 * These offsets line up exactly when META_MAX == 4 and normal 64-bit
 * alignment is respected. After stat_info, the sequence is:
 *   meta_count[4]            0x570..0x57f
 *   segment_count[2]         0x580..0x587
 *   block_count[2]           0x588..0x58f
 *   inplace_count            0x590..0x593
 *   padding                  0x594..0x597
 *   total_hit_ext            0x598..0x59f
 *   read_hit_rbtree          0x5a0..0x5a7
 *   read_hit_largest         0x5a8..0x5af
 *   read_hit_cached          0x5b0..0x5b7
 *   inline_xattr             0x5b8
 *   inline_inode             0x5bc
 *   inline_dir               0x5c0
 *   aw_cnt                   0x5c4
 *   vw_cnt                   0x5c8
 *   max_aw_cnt               0x5cc
 *   max_vw_cnt               0x5d0
 *   bg_gc                    0x5d4
 *   io_skip_bggc             0x5d8
 *   other_skip_bggc          0x5dc
 *
 * NOTE: the X683 offsets 0x5d4/0x5d8/0x5dc still require direct stock
 * call-site validation before their names are promoted to binary-confirmed.
 */
#define X683_SBI_CAND_STAT_BG_GC        0x5d4
#define X683_SBI_CAND_STAT_IO_SKIP_BGGC 0x5d8
#define X683_SBI_CAND_STAT_OTHER_SKIP   0x5dc

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
