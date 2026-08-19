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

## Evidence

The stock binary's Transsion GC code obtains `free_segments` through `sbi -> sm_info -> free_info`, and obtains segment arithmetic from the `0x3d8` / `0x408` region. The Transsion wrapper extracts bit 14 of `sbi + 0x4b8` and passes it as the `sync` argument to `f2fs_gc()`. The known 4.14 F2FS layout identifies that bit as `F2FS_MOUNT_FORCE_FG_GC`, matching the observed stock behavior.

Historical 4.14-era `f2fs_sb_info` places `gc_mode` immediately after `cur_victim_sec`. The stock GC-state access pattern at `0x534` matches that field ordering and semantics.

The source-path fingerprint in the stock kernel identifies `fs/f2fs/f2fs.h`, `segment.h`, `segment.c`, and `super.c` as part of the original `kernel-4.14` build.

## Transsion GC global state context

The formal vendor structure name/size is not proven. The binary nevertheless establishes a live state context containing at least:

| Global offset | Proven role | Confidence |
|---:|---|---|
| `0x898` | worker-active flag | High |
| `0x8a0` | `f2fs_sb_info *` | High |
| `0x8a8` | worker `task_struct *` | High |
| `0x8b8` | wakeup source | High |
| `0x970` | charger-detection state/control | High |
| `0x974` | framebuffer/blank state | High |
| `0x978` | waitqueue | High |
| `0x990` | GC invocation counter | High |
| `0x998` | persistent `gc_type` 0..2 | High |
| `0x9a0` | post-GC counter | High |
| `0x9c0` | inverse `need_switch_ssr` flag | High |
| `0x9d0` | urgent-GC flag | High |
| `0x9d4` | worker phase | High |
| `0x9e0` | worker-create count | High |
| `0x9e8` | worker-destroy count | High |
| `0x9f0` | free-segment metric | High |
| `0x9f4` | startup segment/dirty metric | Medium |
| `0x9f8` | telemetry/type state | Medium |
| `0x9fc` | status state | Medium |
| `0xa00` | capacity/fragmentation state | Medium |
| `0xa04` | positive threshold-trigger byte | High |
| `0xa05` | wakelock/detect gate | Medium |
| `0xa06` | continuation flag | Medium |
| `0xa08` | remembered metric | Medium |
| `0xa0c` | remembered metric | Medium |
| `0xa10` | last segment metric | High |
| `0xa18` | remembered GC/delta value | Medium |
| `0xa20` | proc directory pointer | High |

This offset table is a field map, not a guessed formal C declaration.

## GC state-machine correlation

The reconstructed stock-compatible core uses:

`sync -> FG_GC/BG_GC -> free-section check -> optional checkpoint -> BG-to-FG promotion -> victim selection through dirty_info -> segment migration -> freed-section accounting -> retry/checkpoint -> victim-state reset`.

The Transsion controller sits outside that core and supplies the invocation/admission policy plus a temporary `gc_mode` override.

## Current remaining layout work

- complete exact X683 source/member naming for the still-opaque vendor global state;
- resolve remaining indirect callback/proc-op relationships;
- integrate the reconstructed Transsion policy with the stock-compatible F2FS source model without promoting unresolved offsets to guessed members.
