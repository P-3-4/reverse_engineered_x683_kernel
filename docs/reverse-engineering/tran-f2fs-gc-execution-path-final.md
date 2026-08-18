# X683/H694 Transsion F2FS GC — execution path final reconstruction

Source authority: stock X683/H694 boot image, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.
Decompressed Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`.

This is a binary-derived reconstruction, not recovered proprietary source.

## 1. Complete high-level path

```text
Transsion detector/controller
        |
        v
 freeze-protection gate
 __sb_start_write(sb, SB_FREEZE_WRITE, false)
        |
        v
 vendor policy wrapper (`tran_f2fs_gc`)
        |
        +-- controller/mode policy
        +-- free-space / reservation / filesystem-state guards
        +-- force-FG-GC request bit
        |
        v
 stock/X683 four-argument `f2fs_gc`
        |
        +-- gc_type = sync ? FG_GC : BG_GC
        +-- mounted/checkpoint-error checks
        +-- BG prefree checkpoint opportunity
        +-- BG -> FG escalation when free sections remain low
        +-- reject BG GC from critical/non-background caller
        |
        v
 victim selection
        |
        +-- DIRTY_I(sbi)->v_ops->get_victim(..., NO_CHECK_TYPE, LFS)
        +-- GC policy selected from gc_type + sbi->gc_mode
        +-- dirty bitmap/cursor scan
        +-- candidate cost comparison
        |
        v
 `do_garbage_collect`
        |
        +-- optional SSA summary-page readahead
        +-- summary-page acquisition/reference
        +-- block plug
        +-- each segment in victim section
        |      |
        |      +-- skip empty/stale/error summary
        |      +-- NODE -> gc_node_segment()
        |      +-- DATA -> gc_data_segment()
        |      +-- segment statistics
        |      +-- foreground freed-segment test
        |
        +-- foreground merged-write submission
        +-- finish block plug
        +-- GC call accounting
        |
        v
 repeat / checkpoint / cleanup
        |
        +-- async: repeat while free sections remain insufficient
        +-- foreground async path: checkpoint after GC
        +-- reset victim-section state / cursor bookkeeping as target revision requires
        +-- release GC inode list
        |
        v
 vendor post-GC policy/statistics
        |
        v
 __sb_end_write(sb, SB_FREEZE_WRITE)
```

## 2. Vendor wrapper boundary

The vendor wrapper is the function around decompressed-image `0x366cd4`/the associated `tran_f2fs_gc` call path. Its important properties are:

- it is distinct from the lower stock F2FS GC execution body;
- it consumes the vendor controller and filesystem policy state before entering lower GC machinery;
- it uses the recovered force-foreground request derived from `mount_opt.opt` bit 14 (`F2FS_MOUNT_FORCE_FG_GC`);
- controller 0/1/2 maps to normal / temporary `gc_mode=2` / temporary `gc_mode=3`, with restoration after the forced call.

The direct wrapper reconstruction is in `fs/f2fs/tran_gc_wrapper_reconstructed.c`.

## 3. Vendor policy body: `0x366cd4`

Direct instruction/data-flow recovery shows repeated policy-helper calls before the lower GC call. The wrapper checks:

- vendor GC-policy predicates;
- filesystem state around `sbi + 0x444..0x45c`;
- free/reserved segment state through the segment-manager chain;
- current-segment/time-related vendor thresholds;
- the current `gc_mode` before selecting the post-GC path.

The lower execution call is the function at `0x362c40` in the supplied Image. Its register/stack convention matches the GC entry family and it performs the victim-selection/GC loop machinery rather than the detector arithmetic.

The wrapper then executes vendor post-call accounting/control handling and finally the superblock write-protection release path.

## 4. Stock four-argument GC ABI

The X683 lower GC entry is:

```c
int f2fs_gc(struct f2fs_sb_info *sbi,
            bool sync,
            bool background,
            unsigned int segno);
```

The Transsion wrapper supplies `segno = NULL_SEGNO`.

This four-argument form is consistent with the historical change that added an explicit victim segment argument to `f2fs_gc()`. The change also introduced `init_segno` and reset bookkeeping for `ALLOC_NEXT`/`FLUSH_DEVICE`. The 2017 kernel patch explicitly changed callers to `f2fs_gc(..., NULL_SEGNO)` and added the fourth argument. citeturn824109view1

## 5. GC type and early exits

The execution core follows the target-era F2FS state machine:

```c
gc_type = sync ? FG_GC : BG_GC;
```

Then:

```text
not mounted / inactive
    -> stop with error

checkpoint error
    -> stop with -EIO-style result

BG_GC + insufficient free sections
    -> checkpoint prefree segments when available
    -> if still insufficient, promote BG_GC -> FG_GC

BG_GC + background == false
    -> stop; critical path does not perform background GC
```

This sequence is directly present in the Android/common 4.14/4.15 lineage and matches the recovered X683 call/data flow. citeturn946272search0turn117456view0

## 6. Victim selection

Victim selection crosses the segment-manager boundary:

```text
sbi
  -> SIT_I / segment manager
  -> DIRTY_I(sbi)
  -> v_ops->get_victim()
```

The recovered call signature uses:

```c
get_victim(sbi, &segno, gc_type, NO_CHECK_TYPE, LFS);
```

The target-era policy family is:

- BG GC defaults to cost-benefit;
- FG GC defaults to greedy;
- `gc_mode` can force the resulting policy;
- policy-specific `last_victim[]` cursors are used for LFS scans;
- dirty bitmap candidates are searched and scored until the policy search budget is exhausted.

The 2017 four-argument F2FS change moved `last_victim[]` into `sit_info` and added the fourth `segno` boundary. citeturn824109view1

## 7. `do_garbage_collect`

The target-era execution body is structurally the historical F2FS migration dispatcher:

```c
start_segno = victim;
end_segno = start_segno + sbi->segs_per_sec;
type = segment-entry type at start_segno;
```

Then:

1. readahead contiguous SSA summary pages when `segs_per_sec > 1`;
2. acquire/reference summary pages for the victim section;
3. start a block plug;
4. iterate each segment in the section;
5. obtain its summary page;
6. skip segments that are empty, stale, not uptodate, or blocked by checkpoint error;
7. dispatch NODE segments to `gc_node_segment()`;
8. dispatch DATA segments to `gc_data_segment()`;
9. update segment GC statistics;
10. for foreground GC, count the segment as freed when valid blocks reach zero;
11. submit merged NODE/DATA writes for foreground GC;
12. finish the block plug;
13. increment GC call accounting;
14. return the number of freed victim segments in the target-era form.

The 4.15 source exposes this exact dispatcher structure and the foreground freed-segment test. citeturn117456view0

## 8. DATA migration path

`gc_data_segment()` is a multi-phase migration dispatcher. The 4.15-era implementation that matches the target generation performs:

```text
phase 0 -> NAT metadata readahead
phase 1 -> node-page readahead
phase 2 -> validate parent node / inode ownership and readahead
phase 3 -> obtain inode / encrypted-file handling / data-page readahead
phase 4 -> locate previously prepared inode and actually move each live block
```

For live data it distinguishes encrypted and ordinary files, with the move operation updating the node mapping and writing the migrated block as required by the GC type. It then updates data-block GC statistics. citeturn117456view0

## 9. NODE migration path

`gc_node_segment()` is the corresponding node-summary migration dispatcher. Its role is to inspect the summary entries for live node blocks, validate them against current node information, migrate valid nodes, and update node GC statistics.

The vendor wrapper does not replace this lower-level migration architecture. The binary evidence supports the architectural boundary:

```text
vendor policy/controller
        |
        v
stock F2FS victim + migration machinery
```

## 10. Repeat and checkpoint semantics

After a victim section is processed:

```text
foreground GC
    -> clear current victim section state

asynchronous/background request
    -> if free sections remain insufficient:
           segno = NULL_SEGNO
           repeat GC
       else:
           if GC was promoted to FG_GC:
               write checkpoint
```

The explicit `segno = NULL_SEGNO` reset before repeating is part of the four-argument ABI change. citeturn824109view1turn946272search0

## 11. Cleanup / return

The lower GC path ends by releasing per-GC inode bookkeeping and restoring GC cursor bookkeeping associated with the explicit-segment extension. Synchronous callers convert successful complete-section reclamation into success and return `-EAGAIN` when no complete section was freed.

Historical four-argument source shows:

```c
if (sync)
    ret = sec_freed ? 0 : -EAGAIN;
```

and the explicit-segment revision resets the `ALLOC_NEXT`/`FLUSH_DEVICE` victim cursors at the stop path. citeturn117456view0turn824109view1

The exact X683 ownership of `gc_mutex` and the exact vendor post-call accounting remain separate binary targets; the reconstruction does not silently assume later-tree locking behavior.

## 12. Confirmed X683-specific additions around the core

The machine-code evidence establishes that the stock execution is surrounded by Transsion-specific layers:

```text
static detector
  -> Stop 1..5
  -> controller +0x998
  -> tran_f2fs_gc policy wrapper
  -> force-FG request / gc_mode override
  -> stock F2FS GC core
  -> vendor statistics/control state
```

This matches the independently reconstructed vendor delta: controller state, fragmentation/free-space arithmetic, SSR/urgent progress detection, vendor statistics, and event/charger/USB controls are additions around ordinary F2FS migration rather than replacements for the migration engine. 

## 13. Final execution-path confidence

High confidence:

- vendor detector/controller feeds a separate Transsion policy wrapper;
- wrapper invokes the X683 four-argument F2FS GC ABI with `NULL_SEGNO`;
- GC type is sync/background-derived;
- BG prefree/checkpoint and BG->FG escalation occur before victim selection;
- victim selection crosses `DIRTY_I -> v_ops -> get_victim`;
- victim sections are traversed by `segs_per_sec`;
- SSA summaries feed NODE/DATA migration;
- merged writes and statistics occur after migration;
- asynchronous GC repeats until free-space pressure clears;
- foreground GC can checkpoint after completion;
- synchronous completion is based on freeing a complete section.

Medium confidence / still binary-revision-sensitive:

- exact vendor policy branch conditions inside `0x366cd4`;
- exact X683 ownership/locking of `gc_mutex` around the lower entry;
- exact X683 helper identities at the cleanup/statistics boundaries;
- exact X683 revisions of `gc_node_segment()` and `gc_data_segment()`.

## 14. Practical source-reconstruction boundary

The correct implementation boundary is now:

```text
fs/f2fs/gc.c
    stock 4.14-era core, adapted to exact X683 ABI/layout

fs/f2fs/tran_gc.c
    Transsion policy/controller wrapper

fs/f2fs/tran_gc_thread.c
    detector + Stop 1..5 + state-3 runtime

fs/f2fs/tran_gc_threshold.c
    recovered vendor arithmetic
```

Do not wholesale replace the F2FS migration engine with vendor-specific pseudocode. The recovered evidence instead supports preserving the ordinary F2FS victim-selection and migration machinery and grafting the Transsion controller/detector layer around it.
