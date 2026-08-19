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

Established component sizes:

```text
sizeof(f2fs_sm_info)        = 0xA8
sizeof(sit_info)            = 0xA8
sizeof(free_segmap_info)    = 0x20
sizeof(dirty_seglist_info)  = 0x90
sizeof(curseg_info)         = 0x70
curseg_info count           = 6
curseg array size           = 0x2A0
sizeof(discard_cmd_control) = 0x20B0
```

## Evidence

The stock binary's Transsion GC code obtains `free_segments` through `sbi -> sm_info -> free_info`, and obtains segment arithmetic from the `0x3d8` / `0x408` region. The Transsion wrapper extracts bit 14 of `sbi + 0x4b8` and passes it as the `sync` argument to the proven four-argument `f2fs_gc()` ABI. The `gc_mode` access at `0x534` matches the stock 4.14-era GC mode/state region.

The worker directly consumes `dirty_info + 0x68..0x7c` as the first six entries of `nr_dirty[8]`, and consumes `sm_info + 0x60` as reserved segments. This proves the runtime pointer graph rather than merely matching a historical structure definition.

## Transsion GC global state context

The formal vendor structure name/size is not proven. The binary nevertheless establishes a live state context containing at least:

| Global offset | Proven role | Confidence |
|---:|---|---|
| `0x890` | free-segment policy selector | High |
| `0x894` | free-segment policy selector | High |
| `0x898` | worker-active flag | High |
| `0x8a0` | `f2fs_sb_info *` | High |
| `0x8a8` | worker `task_struct *` | High |
| `0x8b0` | wakeup-source control context | Medium |
| `0x8b8` | wakeup source object | High |
| `0x968` | wakelock/detection control byte | High |
| `0x970` | charger/USB control state | High |
| `0x974` | framebuffer/blank state | High |
| `0x978` | waitqueue | High |
| `0x990` | GC invocation counter | High |
| `0x998` | persistent `gc_type` 0..2 | High |
| `0x9a0` | post-GC counter | High |
| `0x9b0` | vendor retry/static-detect counter | Medium |
| `0x9b8` | vendor special-path counter | Medium |
| `0x9c0` | inverse `need_switch_ssr` flag | High |
| `0x9c8` | remembered retry/state value | Medium |
| `0x9d0` | urgent-GC flag | High |
| `0x9d4` | worker phase | High |
| `0x9d8` | remembered worker state | Medium |
| `0x9e0` | worker-create count | High |
| `0x9e8` | worker-destroy count | High |
| `0x9f0` | free-segment metric | High |
| `0x9f4` | startup segment/dirty metric | Medium |
| `0x9f8` | telemetry/type state | Medium |
| `0x9fc` | status state | Medium |
| `0xa00` | capacity/fragmentation decision state | High |
| `0xa04` | positive threshold-trigger byte | High |
| `0xa05` | wakelock/detect admission gate | High |
| `0xa06` | continuation flag | High |
| `0xa08` | remembered metric | Medium |
| `0xa0c` | remembered metric | Medium |
| `0xa10` | last observed segment metric | High |
| `0xa18` | remembered GC/delta metric | Medium |
| `0xa20` | proc directory pointer | High |

This remains an offset table, not a guessed formal C declaration.

## Vendor/stock boundary

```text
sbi
 |
 +-- +0x80 sm_info
 |      +-- SIT
 |      +-- free_info
 |      +-- dirty_info
 |      +-- reserved_segments
 |      +-- curseg[6]
 |      +-- flush/discard control
 |
 +-- +0x508 gc_mutex
 +-- +0x534 gc_mode
 +-- +0x4b8 mount_opt.opt
 |
 +-- Transsion worker admission
          |
          v
     tran_do_f2fs_gc
          |
          v
     stock f2fs_gc
          |
          +-- stock victim selection
          +-- stock migration
          +-- stock retry/checkpoint/cleanup
```

No downstream `tran_*` migration/scoring helper was found in the direct X683 call-target scan.

## Current remaining layout work

- exact formal vendor global-state declaration/size;
- exact source member names for still-opaque vendor fields;
- exact historical C member corresponding to the SIT quantity at `sit_info + 0x10` used by `tran_has_enough_free_segment()`;
- remaining indirect callback/proc-op container layouts;
- adjacent `f2fs_sb_info` fields outside the already-proven GC/segment-manager region.

The completed GC policy and victim/migration phases should not be reopened unless new binary evidence contradicts this boundary.
