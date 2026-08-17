# X683/H694 F2FS layout reconstruction

Target stock kernel: Linux 4.14.141+ on X683/H694.

This document records field mappings supported by the stock X683 binary and historical 4.14 F2FS source correlation. Unresolved fields remain explicitly unresolved.

## `struct f2fs_sb_info`

| X683 offset | Candidate field | Confidence |
|---:|---|---|
| `0x80` | `sm_info` | **confirmed** by multiple stock GC accesses |
| `0x3d8` | `log_blocks_per_seg` | **confirmed** by stock `f2fs_gc()` |
| `0x3dc` | `blocks_per_seg` | **confirmed** by stock `f2fs_gc()` |
| `0x3e0` | `segs_per_sec` | **confirmed** by stock `f2fs_gc()` |
| `0x408` | `user_block_count` | **confirmed** by stock GC geometry calculations |
| `0x410` | `total_valid_block_count` | high structural match; additional direct call-site validation pending |
| `0x418` | `discard_blks` | high structural match; additional direct call-site validation pending |
| `0x420` | `last_valid_block_count` | high structural match; additional direct call-site validation pending |
| `0x428` | `reserved_blocks` | **confirmed structural/member sequence; direct GC use** |
| `0x430` | `current_reserved_blocks` | **confirmed as a 64-bit field; stock GC accesses its upper word at `0x434`** |
| `0x438` | `unusable_block_count` | high structural match |
| `0x440` | `nquota_files` | **confirmed structural/member sequence; direct GC use** |
| `0x4b8` | `mount_opt.opt` | **confirmed**; bit 14 is passed as `sync` to `f2fs_gc()` |
| `0x528` | `gc_thread` pointer | **confirmed** by X683 GC-thread allocation/initialization: `sbi->gc_thread = gc_th` |
| `0x530` | `cur_victim_sec` candidate | **strong structural match**; stock GC writes `NULL_SEGNO` here during victim processing |
| `0x534` | `gc_mode` | **confirmed** by Transsion wrapper read/write and GC-core usage |
| `0x538` | `next_victim_seg[0]` candidate | strong structural match |
| `0x53c` | `next_victim_seg[1]` candidate | strong structural match |
| `0x540` | `skipped_atomic_files[0]` candidate | strong historical layout match; final binary-use confirmation pending |
| `0x548` | `skipped_atomic_files[1]` candidate | strong historical layout match; direct 64-bit GC access |
| `0x550` | `skipped_gc_rwsem` candidate | strong historical layout match; direct 64-bit GC increments |
| `0x558` | `gc_pin_file_threshold` candidate | historical-layout candidate |
| `0x560` | `max_victim_search` candidate | historical-layout candidate |
| `0x564` | `migration_granularity` candidate | strong correlation; stock GC reads this field |
| `0x568` | `stat_info` pointer candidate | **strong correlation**; stock GC loads a pointer here and updates statistics through it |

### `0x528` GC-thread reconstruction

The stock X683 GC initialization path allocates a GC-thread object, stores it at `sbi + 0x528`, then starts the GC thread using the object. The object layout matches the older 4.14/Transsion form:

| GC-thread relative offset | Candidate |
|---:|---|
| `+0x00` | `f2fs_gc_task` / task pointer |
| `+0x08` | `gc_wait_queue_head` |
| `+0x20` | `urgent_sleep_time` |
| `+0x24` | `min_sleep_time` |
| `+0x28` | `max_sleep_time` |
| `+0x2c` | `no_gc_sleep_time` |
| `+0x30` | `gc_idle` |
| `+0x34` | `gc_urgent` |
| `+0x38` | `gc_wake` |

The X683 initialization sequence writes the sleep-time values at `+0x20/+0x28`, clears the state word at `+0x30`, stores the object into `sbi + 0x528`, initializes the wait queue beginning at `+0x08`, and later launches the GC thread. This is consistent with the historical 4.14 `f2fs_gc_kthread` layout that contains the task pointer, wait queue, four sleep timers, `gc_idle`, `gc_urgent`, and `gc_wake`.

### Reservation-field detail

The stock `f2fs_gc()` uses signed 32-bit accesses at `sbi + 0x428` and `sbi + 0x434`. This is consistent with `reserved_blocks` and the upper/word access of the 64-bit `current_reserved_blocks` field rather than evidence for a separate field at `0x434`.

## `struct f2fs_sm_info`

| Offset | Candidate | Confidence |
|---:|---|---|
| `0x00` | `sit_info` | high |
| `0x08` | `free_info` | high |
| `0x10` | `dirty_info` | **confirmed pointer chain** |
| `0x18` | `curseg_array` | structural match |
| `0x20` | `curseg_lock` | structural match |
| `0x40` | `seg0_blkaddr` | structural match |
| `0x48` | `main_blkaddr` | structural match |
| `0x50` | `ssa_blkaddr` | structural match |
| `0x58` | `segment_count` | high |
| `0x5c` | `main_segments` | high |
| `0x60` | `reserved_segments` | high |
| `0x64` | `additional_reserved_segments` | high |
| `0x68` | `ovp_segments` | high |

## `dirty_info` evidence

The stock Transsion GC thread dereferences `sbi + 0x80`, then `sm_info + 0x10`, and reads the sequence at `dirty_info + 0x68` through `+0x7c` as six consecutive 32-bit values. These values are accumulated by the fragmentation/free-space logic and strongly correlate with the historical F2FS `nr_dirty[]` entries. Final member naming remains provisional until a second independent stock path confirms the array interpretation.

## Confirmed GC ABI

The stock Transsion wrapper at Image offset `0x37ada8` calls:

```c
f2fs_gc(sbi,
        (sbi->mount_opt.opt >> 14) & 1,
        true,
        NULL_SEGNO);
```

The stock `f2fs_gc()` entry is at Image offset `0x3503a8`.

## Remaining work

1. Confirm `cur_victim_sec` and `next_victim_seg[]` with direct stock writes/reads in the GC core.
2. Confirm `skipped_atomic_files[]` and `skipped_gc_rwsem` from the surrounding GC accounting paths.
3. Map the `stat_info` structure reached through `sbi + 0x568`.
4. Recover the remainder of X683 `f2fs_gc()` control flow and vendor-specific modifications.
5. Replace offset helpers with normal structure members only after the complete structure is proven against the selected source revision.
