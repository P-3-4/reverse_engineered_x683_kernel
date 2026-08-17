# X683/H694 F2FS layout reconstruction

Target stock kernel: Linux 4.14.141+ on X683/H694.

This document records only field mappings supported by binary evidence and a structurally matching 4.14 F2FS reference. Unresolved fields remain unresolved.

## `struct f2fs_sb_info`

| X683 offset | Candidate field | Confidence |
|---:|---|---|
| `0x80` | `sm_info` | confirmed by binary access chain |
| `0x3d8` | `log_blocks_per_seg` | high |
| `0x3dc` | `blocks_per_seg` | high |
| `0x3e0` | `segs_per_sec` | high |
| `0x408` | `user_block_count` | high |
| `0x410` | `total_valid_block_count` | high structural match |
| `0x418` | `discard_blks` | high structural match |
| `0x420` | `last_valid_block_count` | high structural match |
| `0x428` | `reserved_blocks` | high structural match |
| `0x430` | `current_reserved_blocks` | high structural match |
| `0x438` | `unusable_block_count` | high structural match |
| `0x440` | `nquota_files` | high structural match |
| `0x4b8` | `mount_opt.opt` | high; bit 14 correlates with `FORCE_FG_GC` |
| `0x534` | `gc_mode` | strong candidate; final confirmation pending |

## `struct f2fs_sm_info`

| Offset | Candidate |
|---:|---|
| `0x00` | `sit_info` |
| `0x08` | `free_info` |
| `0x10` | `dirty_info` |
| `0x18` | `curseg_array` |
| `0x20` | `curseg_lock` |
| `0x40` | `seg0_blkaddr` |
| `0x48` | `main_blkaddr` |
| `0x50` | `ssa_blkaddr` |
| `0x58` | `segment_count` |
| `0x5c` | `main_segments` |
| `0x60` | `reserved_segments` |
| `0x64` | `additional_reserved_segments` |
| `0x68` | `ovp_segments` |

## Remaining work

1. Confirm the exact X683-era `gc_mode` layout around `0x534` from stock `f2fs_gc()`.
2. Resolve `dirty_info` members accessed by `is_f2fs_fragmentation()`.
3. Match the exact 4.14 F2FS API/signatures used by the X683 binary (`f2fs_gc`, `start/stop_gc_thread`, balancing path).
4. Replace offset helpers with normal structure members only after the offsets are proven against the selected source revision.
