# X683 kallsyms validation — 2026-08-19

## Purpose

Validate the binary offsets used in the 2026-08-18 F2FS reconstruction against the shipped `x683_kallsyms.txt` symbol table.

Binary symbol-address base used by the stock image:

```text
Image offset 0x0 -> 0xffffff92d0a80000
_stext          -> 0xffffff92d0a80800 (+0x800)
```

This means an analysis offset `X` maps to `0xffffff92d0a80000 + X`.

## Major correction

The function beginning at `Image+0x366cd4` is not an unnamed vendor policy function. `x683_kallsyms.txt` identifies it exactly as:

```text
Image+0x366cd4 -> f2fs_balance_fs_bg
```

The function runs through `Image+0x366f2f`; the next symbol is:

```text
Image+0x366f30 -> f2fs_issue_flush
```

Therefore the previously recovered CFG at `0x366cd4..0x366f2c` is the X683 implementation of `f2fs_balance_fs_bg()`, including Transsion/vendor modifications. The prior semantic label `x683_gc_policy_global_16c6980` and the claim that `0x366cd4` was a separate policy function are superseded.

## Exact symbol map for the recovered paths

| Image offset | Exact symbol | Confidence |
|---:|---|---|
| `0x3503a8` | `f2fs_gc` | exact kallsyms hit |
| `0x37ada8` | `tran_do_f2fs_gc` | exact kallsyms hit |
| `0x366cd4` | `f2fs_balance_fs_bg` | exact kallsyms hit |
| `0x366f30` | `f2fs_issue_flush` | exact kallsyms hit |
| `0x341250` | `f2fs_sync_fs` | exact kallsyms hit |
| `0x34e224` | `f2fs_sync_dirty_inodes` | exact kallsyms hit |
| `0x35cc18` | `f2fs_available_free_memory` | exact kallsyms hit |
| `0x35d22c` | `f2fs_try_to_free_nats` | exact kallsyms hit |
| `0x362c40` | `f2fs_build_free_nids` | exact kallsyms hit |
| `0x363288` | `f2fs_try_to_free_nids` | exact kallsyms hit |
| `0x373108` | `f2fs_shrink_extent_tree` | exact kallsyms hit |
| `0x3e1014` | `blk_start_plug` | exact kallsyms hit |
| `0x3e1558` | `blk_finish_plug` | exact kallsyms hit |

The following offsets are inside `tran_gc_thread_func`, rather than function starts:

```text
Image+0x377410 -> tran_gc_thread_func + 0x540
Image+0x37742c -> tran_gc_thread_func + 0x55c
```

The following offsets are inside `f2fs_balance_fs_bg` itself:

```text
0x366da4 = +0xd0
0x366de4 = +0x110
0x366ee0 = +0x20c
0x366f2c = +0x258
```

## Stock F2FS correspondence

The 4.14-era `f2fs_balance_fs_bg()` implementation has the same high-level sequence found in the X683 binary:

```text
SBI/POR guard
 -> shrink extent cache if memory is low
 -> free NAT entries if memory is low
 -> free/build free NIDs
 -> dirty-NAT / dirty-threshold / prefree / roll-forward checks
 -> in-flight-IO / recent-request checks
 -> checkpoint timeout check
 -> partial-cache memory check
 -> optional DATA_FLUSH path:
      lock flush_lock
      blk_start_plug
      f2fs_sync_dirty_inodes
      blk_finish_plug
      unlock flush_lock
 -> f2fs_sync_fs(sb, true)
 -> increment background checkpoint count
```

The public 4.14 F2FS source also places `f2fs_balance_fs_bg()` immediately before `f2fs_issue_flush()`, matching the X683 symbol layout. citeturn290622view0

## Consequences for the reconstruction

The binary-derived logic at `0x366cd4` should now be reconstructed as a modified `f2fs_balance_fs_bg(sbi, from_bg)` rather than as a standalone Transsion GC policy routine.

The recovered helper calls now have exact identities:

```text
available-free-memory tests -> f2fs_available_free_memory()
NAT cleanup                 -> f2fs_try_to_free_nats()
free-NID cleanup            -> f2fs_try_to_free_nids()
free-NID rebuild            -> f2fs_build_free_nids()
extent-cache shrink         -> f2fs_shrink_extent_tree()
DATA_FLUSH inode writeout   -> f2fs_sync_dirty_inodes()
plug lifecycle              -> blk_start_plug()/blk_finish_plug()
final checkpoint            -> f2fs_sync_fs()
BG checkpoint statistic     -> bg_cp_count increment
```

This also resolves two earlier weak candidates:

```text
0x34e224 is not f2fs_balance_fs(); it is f2fs_sync_dirty_inodes().
0x341250 is not merely a medium-confidence candidate; it is exactly f2fs_sync_fs().
```

## What remains genuinely vendor-specific

The remaining reconstruction problem is no longer naming the surrounding F2FS routine. It is identifying the X683/Transsion deltas inside the stock `f2fs_balance_fs_bg()` structure:

```text
- exact mapping of selector constants used by f2fs_available_free_memory()
- exact X683 dirty-I/O / jiffies conditions and their correspondence to stock predicates
- exact X683 statistics fields and any additional vendor state
- exact source names for vendor threshold fields not represented by exported kallsyms
```

The symbol table therefore promotes the 0x366cd4 analysis from "vendor-policy CFG" to "vendor-modified F2FS balance_fs_bg CFG" and gives a direct path for source reconstruction.
