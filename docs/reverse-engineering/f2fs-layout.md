# X683/H694 F2FS layout reconstruction

Target kernel: Linux 4.14.141+ / X683-H694.

This document records binary-to-source correlations. Unknown fields remain unresolved until supported by the stock kernel binary.

## High-confidence `f2fs_sb_info` offsets

| Offset | Current identification | Confidence |
|---:|---|---|
| `0x3d8` | `log_blocks_per_seg` | High |
| `0x3dc` | `blocks_per_seg` | High |
| `0x3e0` | `segs_per_sec` | High |
| `0x408` | `user_block_count` | High |
| `0x428` | `reserved_blocks` | High |
| `0x430` | `current_reserved_blocks` | High |
| `0x438` | `unusable_block_count` | High |
| `0x440` | `nquota_files` | High |
| `0x4b8` | `mount_opt.opt` | High |
| `0x508` | `gc_mutex` | High |
| `0x534` | `gc_mode` | High |
| `0x538` | `next_victim_seg[0]` | High |
| `0x53c` | `next_victim_seg[1]` | High |
| `0x560` | `max_victim_search` | High |
| `0x564` | `migration_granularity` | High |
| `0x568` | `stat_info` pointer | High |

## Segment manager

| Offset from `sm_info` | Identification | Confidence |
|---:|---|---|
| `0x00` | `sit_info` | High |
| `0x08` | `free_info` | High |
| `0x10` | `dirty_info` | High |
| `0x60` | `reserved_segments` | High |
| `0x98` | `flush_cmd_control` | High |
| `0xa0` | `discard_cmd_control` | High |

Established sizes/relationships:

```text
sizeof(f2fs_sm_info)        = 0xA8
sizeof(sit_info)            = 0xA8
sizeof(free_segmap_info)    = 0x20
sizeof(dirty_seglist_info)  = 0x90
sizeof(curseg_info)         = 0x70
curseg_info count           = 6
curseg array size           = 0x2A0
sm_info + 0x98              = flush_cmd_control
sm_info + 0xa0              = discard_cmd_control
sizeof(discard_cmd_control) = 0x20B0
```

The dirty counters at `dirty_info + 0x68..0x7c` are the first six entries of `nr_dirty[8]`.

## Whole-image integration

The segment manager is connected to the stock X683 mount/checkpoint/allocation/recovery surface.

```text
f2fs_fill_super
  -> f2fs_build_segment_manager
  -> f2fs_build_node_manager
  -> f2fs_recover_fsync_data

f2fs_write_checkpoint
  -> f2fs_flush_nat_entries
  -> f2fs_flush_sit_entries
  -> discard/prefree handling

allocation
  -> allocate_segment_by_default
  -> change_curseg / new_curseg
  -> update_sit_entry / locate_dirty_segment

F2FS I/O
  -> f2fs_submit_page_bio / f2fs_submit_page_write
  -> block/MMC/MSDC/eMMC
```

Key addresses:

```text
f2fs_build_segment_manager = 0xffffff92d0ded138
f2fs_allocate_data_block   = 0xffffff92d0dea3c8
allocate_segment_by_default = 0xffffff92d0df0454
new_curseg                 = 0xffffff92d0df07e8
f2fs_write_checkpoint      = 0xffffff92d0dce5d0
f2fs_build_node_manager    = 0xffffff92d0de49a8
f2fs_recover_fsync_data    = 0xffffff92d0df0d08
issue_discard_thread       = 0xffffff92d0df0120
```

## 2026-08-20 executable revalidation

The recovered Image was independently decompressed and measured again. Image SHA-256 is `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`. The proven layout sizes remain unchanged. No new formal member names were added from the BLR recheck.

The vendor GC symbols are present in kallsyms, including `tran_do_f2fs_gc`, `tran_gc_thread_func`, `tran_gc_init`, `tran_has_enough_free_segment` and `is_f2fs_fragmentation`. The proven four-argument `f2fs_gc` ABI remains unchanged.

## Transsion GC context

```text
Transsion admission/policy -> tran_do_f2fs_gc -> stock f2fs_gc
-> stock victim selection -> stock migration/accounting
```

No downstream `tran_*` migration/scoring replacement was promoted.

## Remaining layout work

- exact formal vendor global-state declaration/size;
- adjacent `f2fs_sb_info` fields outside the proven region;
- exact historical member corresponding to the SIT quantity at `sit_info + 0x10`;
- indirect callback/proc-op container layouts.
