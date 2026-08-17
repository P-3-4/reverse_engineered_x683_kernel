# X683/H694 F2FS layout reconstruction

Target stock kernel: Linux 4.14.141+ on X683/H694.

This document records field mappings supported by the stock X683 binary and historical 4.14 F2FS source correlation. Unresolved fields remain explicitly unresolved.

## `struct f2fs_sb_info`

| X683 offset | Candidate field | Confidence |
|---:|---|---|
| `0x80` | `sm_info` | **confirmed** by multiple stock GC accesses |
| `0x3d8` | `log_blocks_per_seg` | **confirmed by stock `f2fs_gc()`** |
| `0x3dc` | `blocks_per_seg` | **confirmed by stock `f2fs_gc()`** |
| `0x3e0` | `segs_per_sec` | **confirmed by stock `f2fs_gc()`** |
| `0x408` | `user_block_count` | **confirmed by stock GC geometry calculations** |
| `0x410` | `total_valid_block_count` | high structural match; additional direct call-site validation pending |
| `0x418` | `discard_blks` | high structural match; additional direct call-site validation pending |
| `0x420` | `last_valid_block_count` | high structural match; additional direct call-site validation pending |
| `0x428` | `reserved_blocks` | **confirmed structural/member sequence; direct GC use** |
| `0x430` | `current_reserved_blocks` | **confirmed as a 64-bit field; stock GC accesses its upper word at `0x434`** |
| `0x438` | `unusable_block_count` | high structural match |
| `0x440` | `nquota_files` | **confirmed structural/member sequence; direct GC use** |
| `0x4b8` | `mount_opt.opt` | **confirmed**; bit 14 is passed as `sync` to `f2fs_gc()` |
| `0x508` | `gc_mutex` | **confirmed directly from stock `tran_gc_thread_func`** |
| `0x528` | `gc_thread` pointer | **confirmed** by X683 GC-thread allocation/initialization |
| `0x530` | `cur_victim_sec` | **confirmed structurally**; initialized to `NULL_SEGNO` and reset to `NULL_SEGNO` during GC |
| `0x534` | `gc_mode` | **confirmed** by Transsion wrapper read/write and GC-core usage |
| `0x538` | `next_victim_seg[2]` | **confirmed structurally**; initialized as a 64-bit pair to `NULL_SEGNO` values |
| `0x540` | `skipped_atomic_files[0]` | strong historical-layout match; final direct X683 accounting use pending |
| `0x548` | `skipped_atomic_files[1]` | strong historical-layout match; direct 64-bit GC read |
| `0x550` | `skipped_gc_rwsem` | **very strong**; direct 64-bit GC read/increment and historical position match |
| `0x558` | `gc_pin_file_threshold` | **very strong structural match**; adjacent to `skipped_gc_rwsem` and `max_victim_search`, with direct X683 accesses |
| `0x560` | `max_victim_search` | **confirmed structurally**; initialized to `0x1000` |
| `0x564` | `migration_granularity` | **very strong structural match**; initialized from an X683 configuration value and directly read by GC |
| `0x568` | `stat_info` pointer | **strong**; GC loads a 64-bit pointer here and updates statistics members through it |
| `0x5d4` | `bg_gc` | **strong structural candidate; direct stock identity validation still pending** |
| `0x5d8` | `io_skip_bggc` | **strong structural candidate; direct stock identity validation still pending** |
| `0x5dc` | `other_skip_bggc` | **strong structural candidate; direct stock identity validation still pending** |

### Newly confirmed `gc_mutex @ 0x508`

The previous unresolved `gc_mutex` position is now closed by direct stock-binary evidence.

At `tran_gc_thread_func` the binary computes:

```asm
add x8, x19, #0x508
str x8, [sp, #0x38]
...
ldr x0, [sp, #0x38]
bl  mutex_trylock
```

Here `x19` is the `struct f2fs_sb_info *`. The value passed to `mutex_trylock()` is therefore exactly `sbi + 0x508`.

The same mutex address is then used for the corresponding unlock path. This is no longer an inference from structure size: the address is materialized from `sbi` by the stock Transsion GC thread itself.

The resulting sequence is:

```text
0x4b8  mount_opt.opt
0x508  gc_mutex
0x528  gc_thread
0x530  cur_victim_sec
0x534  gc_mode
```

### `0x528` GC-thread reconstruction

The stock X683 GC initialization path allocates a GC-thread object, stores it at `sbi + 0x528`, then initializes its wait queue and launches the task. The object layout matches the historical 4.14 F2FS form:

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

### Reservation-field detail

The stock `f2fs_gc()` uses signed 32-bit accesses at `sbi + 0x428` and `sbi + 0x434`. This is consistent with `reserved_blocks` and the upper/word access of the 64-bit `current_reserved_blocks` field rather than evidence for a separate field at `0x434`.

### GC-state sequence after `gc_thread`

The X683 binary now strongly matches the historical sequence:

```text
0x528  gc_thread
0x530  cur_victim_sec
0x534  gc_mode
0x538  next_victim_seg[2]
0x540  skipped_atomic_files[2]
0x550  skipped_gc_rwsem
0x558  gc_pin_file_threshold
0x560  max_victim_search
0x564  migration_granularity
0x568  stat_info
```

## Historical source fingerprint

The X683 binary is now a particularly close match to the older F2FS generation containing:

```c
int f2fs_gc(struct f2fs_sb_info *sbi,
            bool sync,
            bool background,
            unsigned int segno);
```

and the manager sequence:

```c
struct mutex gc_mutex;
struct f2fs_gc_kthread *gc_thread;
unsigned int cur_victim_sec;
unsigned int gc_mode;
unsigned int next_victim_seg[2];
unsigned long long skipped_atomic_files[2];
unsigned long long skipped_gc_rwsem;
u64 gc_pin_file_threshold;
unsigned int max_victim_search;
unsigned int migration_granularity;
struct f2fs_stat_info *stat_info;
```

The four-argument ABI and the manager sequence are both present in historical Android/common F2FS source. The November 2018 Android/common merge also shows the surrounding `CONFIG_F2FS_STAT_FS` members discussed below. citeturn877109search0turn705608search2

## `stat_info` / post-`stat_info` structure fingerprint

A useful new constraint comes from normal C alignment in the historical `struct f2fs_sb_info` when `CONFIG_F2FS_STAT_FS=y`.

The stock X683 configuration has F2FS statistics enabled. In the historical source generation matching the X683 GC manager, the fields following `stat_info` are:

```text
sbi + 0x568  stat_info pointer
sbi + 0x570  meta_count[4]
sbi + 0x580  segment_count[2]
sbi + 0x588  block_count[2]
sbi + 0x590  inplace_count
sbi + 0x594  padding for 64-bit alignment
sbi + 0x598  total_hit_ext
sbi + 0x5a0  read_hit_rbtree
sbi + 0x5a8  read_hit_largest
sbi + 0x5b0  read_hit_cached
sbi + 0x5b8  inline_xattr
sbi + 0x5bc  inline_inode
sbi + 0x5c0  inline_dir
sbi + 0x5c4  aw_cnt
sbi + 0x5c8  vw_cnt
sbi + 0x5cc  max_aw_cnt
sbi + 0x5d0  max_vw_cnt
sbi + 0x5d4  bg_gc
sbi + 0x5d8  io_skip_bggc
sbi + 0x5dc  other_skip_bggc
```

This alignment is exact: `META_MAX` is four entries in this F2FS generation, and `atomic64_t` members must begin on 8-byte boundaries. Historical Android/common source explicitly places `meta_count[META_MAX]`, `segment_count[2]`, `block_count[2]`, `inplace_count`, the extent-hit atomics, `inline_*`, atomic-write counters, `bg_gc`, `io_skip_bggc`, and `other_skip_bggc` in this order. citeturn705608search2turn877109search0

Therefore the unresolved X683 offsets now have a strong structural explanation:

```text
0x5d4  -> bg_gc
0x5d8  -> io_skip_bggc
0x5dc  -> other_skip_bggc
```

This is **not yet promoted to binary-confirmed identity**. The remaining proof requirement is to find stock X683 call sites whose operation semantics distinguish the three counters. The structural match is nevertheless strong enough to guide that search and sharply narrows the historical source revision.

### Why this matters for source identification

The older Android/common F2FS history shows `bg_gc` and the two background-GC skip counters being added together with the corresponding statistics plumbing. That gives us a second fingerprint beyond the GC-manager fields:

```text
GC manager sequence
        +
CONFIG_F2FS_STAT_FS post-stat_info sequence
        +
f2fs_gc(sbi, sync, background, segno)
        +
X683 binary access pattern
```

A candidate source tree that matches all four is substantially stronger than one selected only from the Linux 4.14 version number. citeturn877109search0

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

The stock Transsion GC thread dereferences `sbi + 0x80`, then `sm_info + 0x10`, and reads the sequence at `dirty_info + 0x68` through `+0x7c` as six consecutive 32-bit values. These values are accumulated by the fragmentation/free-space logic and strongly correlate with the historical F2FS `nr_dirty[]` entries. Final X683 member naming remains provisional until another stock path confirms the same array relationship.

## Unresolved vendor-specific counters

The only remaining uncertainty around the `0x5d4–0x5dc` region is now the direct stock semantic proof. Structurally these offsets line up with `bg_gc`, `io_skip_bggc`, and `other_skip_bggc`; until stock call sites distinguish their operations, they remain candidates rather than confirmed names.

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

1. Find direct stock accesses that distinguish `0x5d4`, `0x5d8`, and `0x5dc` and promote the structural candidates to binary-confirmed names.
2. Map the `f2fs_stat_info` object reached through `sbi + 0x568` by following the pointer and matching member offsets.
3. Confirm `skipped_atomic_files[2]` by finding stock counter reads/writes at both `0x540` and `0x548`.
4. Recover the remaining X683 `f2fs_gc()` victim-selection/accounting control flow.
5. Identify the closest historical 4.14 F2FS revision by comparing complete `f2fs.h`, `gc.c`, `gc.h`, `segment.c`, and statistics plumbing—not by kernel version alone.
6. Replace offset helpers with normal structure members only after the complete structure is proven against the selected source revision.
