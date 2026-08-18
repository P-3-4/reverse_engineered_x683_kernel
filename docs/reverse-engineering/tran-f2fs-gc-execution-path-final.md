# X683/H694 Transsion F2FS GC — execution path final reconstruction

Source authority: stock X683/H694 boot image, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.
Decompressed Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`.

This is a binary-derived reconstruction, not recovered proprietary source.

## 1. Direct function boundary: `0x3503a8`

The decisive X683 boundary is:

```text
tran_f2fs_gc wrapper
    |
    +---- BL 0x3503a8
               |
               +---- actual X683 four-argument F2FS GC entry
```

The wrapper at the vendor `tran_f2fs_gc` site directly calls `0x3503a8`. The function at `0x3503a8` has the recovered four-argument calling convention and contains the complete GC state machine: early filesystem checks, free-space policy, victim selection, summary/migration loops, statistics, repeat/checkpoint handling, and final cleanup.

The previously documented `0x362c40` as the lower GC entry was incorrect. `0x362c40` is an internal helper called by the separate vendor policy routine at `0x366cd4`; it is not the stock `f2fs_gc()` entry.

## 2. Complete high-level path

```text
Transsion detector/controller
        |
        v
 vendor/superblock policy gate
        |
        v
 tran_f2fs_gc wrapper
        |
        +-- controller/mode policy
        +-- force-FG request from mount_opt bit 14
        |
        v
0x3503a8: X683 f2fs_gc(sbi, sync, background, segno)
        |
        +-- trace/entry bookkeeping
        +-- gc_type = sync ? FG_GC : BG_GC
        +-- active/checkpoint-error checks
        +-- BG prefree/checkpoint opportunity
        +-- BG -> FG escalation
        +-- critical-path BG rejection
        |
        v
 victim selection
        |
        +-- policy construction
        +-- DIRTY_I -> v_ops->get_victim(..., NO_CHECK_TYPE, LFS)
        +-- candidate selection/cost policy
        |
        v
 section/segment migration
        |
        +-- summary-page/SSA preparation
        +-- iterate victim section
        +-- NODE/DATA dispatch
        +-- live-block migration
        +-- segment statistics
        +-- foreground freed-segment accounting
        |
        v
 write completion / repeat
        |
        +-- merged NODE/DATA writes
        +-- asynchronous free-space recheck
        +-- repeat with segno = NULL_SEGNO when required
        +-- checkpoint after foreground work in async path
        |
        v
 stop bookkeeping / cursor restoration / inode cleanup
        |
        v
 return to tran_f2fs_gc
        |
        v
 vendor post-GC policy/statistics
```

## 3. `tran_f2fs_gc` controller wrapper

Direct X683 disassembly at the vendor wrapper shows:

```text
controller +0x998

0 -> direct call of 0x3503a8
1 -> save old sbi->gc_mode; set temporary policy state; call 0x3503a8; restore
2 -> save old sbi->gc_mode; set temporary policy state; call 0x3503a8; restore
```

The call always supplies the force-foreground request derived from `sbi->mount_opt.opt` bit 14 and the fourth segment argument is `NULL_SEGNO`.

The reconstructed controller-to-mode mapping is kept in `fs/f2fs/tran_gc_wrapper_reconstructed.c`.

## 4. Actual X683 `f2fs_gc()` entry: `0x3503a8`

The function prologue begins at `0x3503a8` and its first major blocks match the target-era F2FS GC structure.

### 4.1 Request classification

The fourth incoming argument is preserved as the initial segment request. The second argument controls the foreground/background request and is folded into the internal GC type. The third argument controls whether a background request is allowed to execute on the critical path.

### 4.2 Filesystem and free-space preparation

The entry reads the X683 F2FS fields directly:

```text
+0x3d8  log_blocks_per_seg
+0x3dc  blocks_per_seg
+0x3e0  segs_per_sec
+0x428  reserved_blocks
+0x434  current reserved/user-associated count used by policy
+0x440  quota-related count
+0x4b8  mount options
+0x534  gc_mode
```

It performs the expected free-space/reservation arithmetic before entering the victim-selection/migration loop.

## 5. Direct victim-selection boundary inside `0x3503a8`

At approximately `0x350808..0x350850`, the binary constructs the victim-selection context and then dispatches through the dirty-manager operation vector.

The decisive indirect call is:

```text
x0 = sbi
x1 = &requested/current segno output
x2 = gc_type
x3 = NO_CHECK_TYPE
x4 = LFS

blr [DIRTY_I(sbi)->v_ops->get_victim]
```

This is a direct stock-style `DIRTY_I(sbi)->v_ops->get_victim()` boundary, not a vendor pseudo-selector.

The corresponding historical four-argument change confirms the explicit victim-segment ABI and `NULL_SEGNO` calling convention. citeturn143882view0

## 6. Victim policy semantics

The target-era policy family matches the nearby 4.14/4.15 F2FS implementation:

```text
BG_GC  -> cost-benefit policy by default
FG_GC  -> greedy policy by default
```

`gc_mode` modifies the resulting policy, and policy-specific victim cursors live with the SIT manager in the four-argument revision. The historical patch moves `last_victim[]` into `SIT_I(sbi)` and adds `ALLOC_NEXT` / `FLUSH_DEVICE` cursor slots. citeturn143882view0

## 7. Vendor policy body: `0x366cd4`

`0x366cd4` is separate from `0x3503a8`. It is a Transsion policy/orchestration function used around the GC subsystem.

### 7.1 Exact first policy stage

At `0x366d00..0x366d5c` it calls several vendor helpers with selectors:

```text
selector 4
selector 0x80 through helper 0x373108
selector 1
selector 0x1c7
selector 0
```

The helper at `0x35cc18` is a multi-mode policy predicate returning a boolean. Its implementation selects different arithmetic branches from an internal mode table for values 0..5 and computes threshold comparisons from values under the `sbi + 0x70` vendor-policy object. It is not safe to rename it to a standard F2FS function.

`0x373108` first checks an X683 field at `sbi + 0x4b9` bit 5 and otherwise executes a large vendor accounting/threshold path. It returns a boolean used as a policy gate.

### 7.2 Lower helper call

When the selector-0 policy predicate is true, `0x366d4c` calls:

```text
0x362c40(sbi, 0, 0)
```

Otherwise it calls `0x363288(sbi, 0x0e38)`.

These are internal vendor helpers, not the main `f2fs_gc()` entry.

### 7.3 Post-policy guards

The routine then checks:

```text
sbi + 0x534              gc_mode
sbi + 0x444..0x45c       filesystem/vendor state counters
sbi + 0x70               vendor policy object
sm_info and child state   free/reservation relationships
```

The arithmetic at `0x366db4..0x366dc4` is the same fixed-point percentage operation recovered elsewhere (`0x51EB851F >> 37`). A second identical gate occurs around `0x366e1c..0x366e40`.

It then checks a time/counter condition using a vendor global scaled at `250 * value + base` and branches into the alternate/recheck path when the threshold is crossed.

### 7.4 Post-GC accounting path

At `0x366e84..0x366eb8` the vendor function conditionally executes:

```text
0x3e1014
0x34e224(sbi, 1)
0x3e1558
0x341250(sb, 1)
stat_info +0x16c += 1
```

The exact proprietary names of the first three helper operations are not asserted here. `0x341250` is a superblock/write-protection path and is binary-distinct from the actual `f2fs_gc()` entry.

## 8. Internal helper `0x362c40`

`0x362c40` is a sizeable vendor F2FS/segment helper. Direct disassembly shows:

- first argument `sbi`;
- additional mode/boolean arguments;
- acquisition of a lock under the `sbi + 0x70` manager;
- scans over up to `0x1c7` entries;
- accesses to manager bitmap/array state at offsets `+0xd8`, `+0xe0`, `+0xe8`, and related fields;
- repeated calls to a helper at `0x3655d8` with `(sbi, segment-like index, mode flags)`;
- linked-list/reference cleanup and error reporting.

This is therefore a **vendor segment-management helper**, not the stock `f2fs_gc()` entry. Its exact proprietary name is not yet proven.

## 9. Internal helper `0x363288`

`0x363288` operates on a list under the vendor segment manager (`+0x98`), uses `mutex_trylock()` around the list, removes list entries, decrements a manager count at `+0xa8`, invokes a reference-release helper, and returns the number of successfully drained entries.

Its role is best described as:

```text
vendor segment-manager list drain / cleanup helper
```

Do not label it `put_gc_inode()` without direct symbol/data-flow proof.

## 10. Actual victim-section migration inside `0x3503a8`

The core migration loop is inside `0x3503a8`, not `0x362c40`.

The function contains repeated segment-index loops bounded by `sbi->blocks_per_seg` / `sbi->segs_per_sec`, summary/NAT/node metadata accesses, and calls into the lower page/migration machinery.

The binary structure matches the historical `do_garbage_collect()` division:

```text
victim segment
    |
    +-- summary metadata
    |
    +-- per-segment valid-block test
    |
    +-- NODE/DATA branch
    |
    +-- live block migration
    |
    +-- per-segment accounting
```

The nearby 4.15 implementation exposes the same dispatcher and exact return rule: foreground GC counts a segment freed when its valid-block count reaches zero; the whole section is considered freed when that count reaches `segs_per_sec`. citeturn497268view0

## 11. DATA migration semantics

The matching target-era F2FS `gc_data_segment()` implementation is phase-based:

```text
phase 0 -> NAT metadata readahead
phase 1 -> node page readahead
phase 2 -> inode/node ownership validation + node readahead
phase 3 -> inode acquisition + data-page preparation
phase 4 -> live block movement
```

For live entries it distinguishes encrypted files and ordinary files, performs the appropriate block/page move, and updates the data-block GC counter. citeturn497268view0

The X683 vendor policy layer does not replace this migration architecture.

## 12. NODE migration semantics

The corresponding node path consumes summary entries, validates current node state, migrates live node blocks and updates node GC statistics.

The stock execution boundary remains:

```text
vendor policy
   -> f2fs_gc
      -> victim selection
         -> summary dispatcher
            -> node/data migration
```

## 13. Repeat/checkpoint/cleanup

The four-argument lineage provides the complete stop/repeat structure:

```text
BG free-space still insufficient
    -> segno = NULL_SEGNO
    -> repeat gc_more

BG promoted to FG
    -> write checkpoint on asynchronous completion

FG section completely freed
    -> sec_freed++

stop
    -> reset SIT last_victim[ALLOC_NEXT]
    -> restore/record FLUSH_DEVICE cursor
    -> release GC inode list
    -> sync: success if sec_freed else -EAGAIN
```

The four-argument upstream patch shows the same `segno = NULL_SEGNO` repeat and the `ALLOC_NEXT` / `FLUSH_DEVICE` cursor cleanup. citeturn143882view0turn497268view0

X683 exact ownership of `gc_mutex` is kept conservative: the source scaffold does not silently add an unlock without direct binary proof.

## 14. Final source-reconstruction boundary

The correct architecture is now explicit:

```text
fs/f2fs/gc.c
    X683 `0x3503a8` four-argument F2FS GC core

fs/f2fs/tran_gc.c
    Transsion controller/policy wrapper

fs/f2fs/tran_gc_thread.c
    detector, freezer-aware wait/re-entry, Stop 1..5

fs/f2fs/tran_gc_threshold.c
    recovered vendor arithmetic

vendor helpers around 0x366cd4
    remain separate until exact proprietary naming is proven
```

The migration engine should therefore be reconstructed from the stock F2FS 4.14-era core and adapted to the X683 binary layout, while the Transsion detector/controller/policy layer is grafted around it.

## 15. Confidence

High:

- `0x3503a8` is the actual X683 four-argument `f2fs_gc()` entry called by `tran_f2fs_gc`;
- direct `DIRTY_I -> v_ops -> get_victim` call inside the entry;
- victim section traversal and migration architecture;
- foreground complete-section accounting;
- asynchronous repeat/checkpoint semantics;
- vendor `0x366cd4` is a separate policy/orchestration layer;
- `0x362c40` and `0x363288` are internal vendor helpers, not the main GC entry.

Medium:

- exact X683-specific revisions of the lower node/data migration helpers;
- exact proprietary names of the `0x366cd4` helper cluster;
- exact `gc_mutex` ownership at the vendor/thread boundary.
