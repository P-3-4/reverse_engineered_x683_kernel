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
| `0x534` | F2FS GC state/mode field; exact member name unresolved | High for semantics, unresolved for name |

## Segment manager

| Offset from `sm_info` | Identification | Confidence |
|---:|---|---|
| `0x00` | `sit_info` | High |
| `0x08` | `free_info` | High |
| `0x10` | `dirty_info` | High |
| `0x60` | `reserved_segments` | High |

## Evidence

The stock binary's Transsion GC code obtains `free_segments` through `sbi -> sm_info -> free_info`, and obtains segment arithmetic from the `0x3d8` / `0x408` region. The Transsion wrapper extracts bit 14 of `sbi + 0x4b8` and passes it as the `sync` argument to `f2fs_gc()`. The known 4.14 F2FS layout identifies bit 14 as `F2FS_MOUNT_FORCE_FG_GC`, matching the stock behavior.

The source-path fingerprint in the stock kernel identifies `fs/f2fs/f2fs.h`, `segment.h`, `segment.c`, and `super.c` as part of the original `kernel-4.14` build.

## Remaining work

- Resolve the exact `0x534` member name from stock `f2fs_gc()` access patterns.
- Correlate all remaining accesses around the GC state machine and dirty-segment counters.
- Match the exact X683-era 4.14 F2FS revision before replacing offset-based reconstruction with normal C member accesses.
