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
| `0x534` | `gc_mode` | High |

## Segment manager

| Offset from `sm_info` | Identification | Confidence |
|---:|---|---|
| `0x00` | `sit_info` | High |
| `0x08` | `free_info` | High |
| `0x10` | `dirty_info` | High |
| `0x60` | `reserved_segments` | High |

## Evidence

The stock binary's Transsion GC code obtains `free_segments` through `sbi -> sm_info -> free_info`, and obtains segment arithmetic from the `0x3d8` / `0x408` region. The Transsion wrapper extracts bit 14 of `sbi + 0x4b8` and passes it as the `sync` argument to `f2fs_gc()`. The known 4.14 F2FS layout identifies bit 14 as `F2FS_MOUNT_FORCE_FG_GC`, matching the stock behavior.

Historical 4.14-era `f2fs_sb_info` places `gc_mode` immediately after `cur_victim_sec`. The stock GC-state access pattern at `0x534` matches that field ordering and semantics. The field is therefore resolved as `gc_mode` rather than left as an unnamed GC-state field.

The source-path fingerprint in the stock kernel identifies `fs/f2fs/f2fs.h`, `segment.h`, `segment.c`, and `super.c` as part of the original `kernel-4.14` build.

## GC state-machine correlation

The reconstructed core uses the following stock-compatible flow:

`sync -> FG_GC/BG_GC` -> free-section check -> optional checkpoint -> BG-to-FG promotion -> victim selection through `dirty_info` -> segment migration -> freed-section accounting -> repeat/checkpoint -> victim-state reset.

`gc_mode` is a policy/state field and is not substituted for the mount option word at `0x4b8`.

## Remaining work

- Match the exact X683-era helper implementations (`__get_victim`, `do_garbage_collect`, dirty-segment operations) against the stock binary.
- Resolve the Transsion-specific GC trigger predicates around charging, USB, framebuffer, wakelock and fragmentation.
- Match the final vendor 4.14 source revision before replacing all remaining offset-based reconstruction with normal C member accesses.
