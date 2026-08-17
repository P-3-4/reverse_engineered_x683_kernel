# X683/H694 F2FS layout reconstruction

Target stock kernel: Linux 4.14.141+ on X683/H694.

This document records only field mappings supported by binary evidence and a structurally matching 4.14 F2FS reference. Unresolved fields remain unresolved.

## `struct f2fs_sb_info`

| X683 offset | Candidate field | Confidence |
|---:|---|---|
| `0x80` | `sm_info` | **confirmed** by multiple stock GC accesses |
| `0x3d8` | `log_blocks_per_seg` | **confirmed** by stock `f2fs_gc()` |
| `0x3dc` | `blocks_per_seg` | **confirmed** by stock `f2fs_gc()` |
| `0x3e0` | `segs_per_sec` | **confirmed** by stock `f2fs_gc()` |
| `0x408` | `user_block_count` | **confirmed** by stock `f2fs_gc()` geometry calculation |
| `0x410` | `total_valid_block_count` | high structural match |
| `0x418` | `discard_blks` | high structural match |
| `0x420` | `last_valid_block_count` | high structural match |
| `0x428` | `reserved_blocks` | **confirmed structural/member sequence; direct GC use** |
| `0x430` | `current_reserved_blocks` | **confirmed as 64-bit field via `0x434` word access** |
| `0x438` | `unusable_block_count` | high structural match |
| `0x440` | `nquota_files` | **confirmed structural/member sequence; direct GC use** |
| `0x4b8` | `mount_opt.opt` | **confirmed**; bit 14 is passed as `sync` |
| `0x528` | GC thread/state pointer | **confirmed direct access from GC core** |
| `0x534` | `gc_mode` | **confirmed** by Transsion wrapper read/write and GC-core read |
| `0x548` | GC-related pointer/state | candidate; direct GC-core access |
| `0x550` | GC-related field | candidate; direct GC-core write |
| `0x5d4` | GC counter/state | candidate; direct GC-core increment |
| `0x5d8` | GC counter/state | candidate; direct GC-core increment |
| `0x5dc` | GC counter/state | candidate; direct GC-core increment |

### Reservation-field detail

The stock `f2fs_gc()` uses signed 32-bit accesses at `sbi + 0x428` and `sbi + 0x434`. This is consistent with `reserved_blocks` and the high/word portion of a 64-bit `current_reserved_blocks` field rather than evidence for unrelated fields at `0x434`.

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

## `dirty_info` evidence

The stock Transsion GC thread dereferences `sbi + 0x80`, then `sm_info + 0x10`, and reads the sequence at `dirty_info + 0x68` through `+0x7c` as six consecutive 32-bit values. These values are accumulated by the fragmentation/free-space logic and strongly correlate with the six historical F2FS `nr_dirty[]` entries. Final member naming should be retained as `nr_dirty[]` only after a second stock function confirms the same pointer/array relationship.

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

1. Map `gc_thread` and `gc_mutex` from the stock GC initialization and thread paths.
2. Confirm the six `dirty_info + 0x68..0x7c` values as the `nr_dirty[]` array with another binary path.
3. Resolve the GC-related fields at `0x548`, `0x550`, `0x5d4`, `0x5d8`, and `0x5dc` against historical `f2fs_sb_info` revisions.
4. Recover the remaining `f2fs_gc()` control flow, victim selection, and vendor additions.
5. Replace offset helpers with normal structure members only after the complete structure is proven against the selected source revision.
