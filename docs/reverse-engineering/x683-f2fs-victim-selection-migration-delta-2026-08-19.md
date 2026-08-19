# X683/H694 F2FS GC Victim Selection and Migration — 2026-08-19

## Scope

This phase isolates the algorithmic Transsion delta inside the stock X683/H694 F2FS garbage collector.

Target binary:

```text
Linux 4.14.141+ / ARM64 / MT6768
f2fs_gc(): runtime 0xffffff92d0dd03a8
          image   0x3503a8
get_victim_by_default(): runtime 0xffffff92d0dd2e74
f2fs_need_SSR(): runtime 0xffffff92d0de58f8
tran_do_f2fs_gc(): runtime 0xffffff92d0dfada8
tran_has_enough_free_segment(): runtime 0xffffff92d0dfb5d4
is_f2fs_fragmentation(): runtime 0xffffff92d0dfb580
```

Direct binary evidence is authoritative. Historical 4.14 F2FS is used only to identify the corresponding stock algorithms.

## Executive result

The stock X683 binary does **not** contain evidence of a Transsion replacement for the core F2FS victim picker, cost function, SSR selector, age/mtime calculation, or physical victim-migration engine.

The genuine Transsion GC delta is a **control/policy layer around stock F2FS GC**:

```text
Transsion GC controller
        |
        +-- free-space / fragmentation / urgency gates
        |
        +-- temporary sbi->gc_mode override
        v
stock f2fs_gc(sbi, sync, background, segno)
        |
        +-- stock victim selection
        +-- stock cost/age/mtime scoring
        +-- stock SSR test
        +-- stock node/data migration
        +-- stock GC retry/checkpoint/cleanup
```

The strongest vendor-specific effect is `tran_do_f2fs_gc()`, which temporarily writes `sbi + 0x534` (`gc_mode`) to `3` or `2`, calls the normal X683 `f2fs_gc()` with `segno = NULL_SEGNO`, and restores the previous mode. This is a policy change in **when and under what stock GC mode** the collector runs, not a replacement of the stock victim algorithm.

For `gc_mode == 3`, the stock X683 selector also takes the normal historical urgent-mode consequences: the `max_victim_search` cap is bypassed and `f2fs_need_SSR()` returns true. Those effects come from the stock picker/SSR logic already present in X683; Transsion reaches them by changing the mode.

## Critical ABI correction

The X683 stock wrapper call is four-argument:

```c
f2fs_gc(
    sbi,
    (sbi->mount_opt.opt >> 14) & 1,
    true,
    NULL_SEGNO);
```

Binary evidence at `tran_do_f2fs_gc()`:

```text
0x37adf4: x1 = (sbi + 0x4b8 bit 14)
0x37adf8: x0 = sbi
0x37adfc: BL f2fs_gc
```

The third argument is `1`; the fourth is `-1` (`NULL_SEGNO`). Older repository notes claiming a three-argument ABI are stale and must not be reused.

## 1. Victim-selection entry and policy setup

### `get_victim_by_default()`

The X683 compiler left the victim picker as a real standalone function at `0xffffff92d0dd2e74`.

Important arguments recovered directly from the prologue:

```text
x0  sbi
x1  result
w2  gc_type
w3  type
w4  alloc_mode
```

The function derives:

```text
sbi + 0x80  -> sm_info
sm_info + 0x10 -> dirty_info pointer
sm_info + 0x58 -> segment_count
```

It locks the dirty segment-list lock at:

```text
dirty_info + 0x48
```

The local selection-policy fields are materialized on the stack:

```text
sp + 0x10  alloc_mode
sp + 0x14  policy gc_mode
sp + 0x18  dirty_segmap pointer
sp + 0x20  max_search
sp + 0x24  min_cost
sp + 0x28  ofs_unit
sp + 0x2c  initial/max cost
sp + 0x30  min_segno
```

### SSR policy path

When `alloc_mode == 1` the binary sets:

```text
policy gc_mode = 1
policy dirty map = dirty category selected by type
policy max_search = corresponding dirty count field
policy ofs_unit = 1
```

This is the direct X683 equivalent of the historical SSR `select_policy()` path.

### LFS/normal policy path

For non-SSR allocation, X683 derives the policy mode from `sbi + 0x534` and `gc_type`. The compiler emits a small table lookup from Image offset `0xe73f40`; the observed table bytes begin:

```text
{ 0, 1, 1, ... }
```

The safe binary-backed conclusion is that X683 converts the current `gc_mode`/GC type into an internal picker policy mode before scoring. The exact source enum name for this internal value should not be invented from the table alone.

## 2. Search-window filtering

The selector contains the historical search-limit condition:

```text
if (gc_type != foreground && sbi->gc_mode != 3)
    cap max_search at sbi + 0x560;
```

Direct X683 offsets:

```text
sbi + 0x534 = gc_mode
sbi + 0x560 = max_victim_search
```

The relevant instructions are at approximately `0x352f4c..0x352f78`.

The historical 4.14 implementation has the same structure: non-foreground GC is limited by `max_victim_search`, except urgent GC, which scans beyond that cap. The public v4.14 implementation documents this in `get_victim_by_default()` / `select_policy()`. urlChromiumOS Linux 4.14 F2FS gc.chttps://chromium.googlesource.com/chromiumos/third_party/kernel/+/v4.14/fs/f2fs/gc.c

This is important for the vendor mode change: `gc_mode == 3` changes the **search breadth** without changing the victim cost formula.

## 3. Victim filtering

### Candidate validity

The binary treats the result segment as a 0x28-byte SIT entry and starts from the SIT entry array reached through:

```text
sbi + 0x80 -> sm_info
sm_info + 0x00 -> sit_info
sit_info + 0x68 -> SIT entry array base
```

For candidate `segno`, it computes:

```text
entry = sit_info + 0x68 + segno * 0x28
```

and rejects candidates whose packed validity field fails the same valid-segment test used by the stock selector. At `0x352ff4` the binary tests the SIT word with `0xffc0`.

### Section-level exclusion

The selector accesses a separate object through:

```text
sm_info + 0x18
```

and compares the candidate section number against six section-use values at this object offsets:

```text
+0x5c
+0xcc
+0x13c
+0x1ac
+0x21c
+0x28c
```

Each value is divided by `sbi + 0x3e0` (`segs_per_sec`) and compared with the candidate section. A match rejects the candidate.

These offsets must remain named only as `object(sm_info + 0x18) + offset`; they are **not** the previously reconstructed `dirty_info + 0x68..0x7c` counters and must not be conflated with `nr_dirty[]`.

The shape of this filtering is consistent with the stock section-level victim restrictions in the historical 4.14 selector.

### Current-victim exclusion

Candidate section/segment handling also checks:

```text
sbi + 0x530 = cur_victim_sec
```

and rejects a candidate in the currently protected victim section.

### Checkpoint/usage filter

For the relevant non-empty candidate path the binary tests the segment/SIT packed field and rejects candidates whose checkpointed-valid-block state violates the current GC requirement. The exact source macro name for this X683 predicate is not assigned here because the binary only proves the field test, not the original C identifier.

### next_victim_seg shortcut

The picker reuses the stock `next_victim_seg[]` state:

```text
sbi + 0x538 = next_victim_seg[0]
sbi + 0x53c = next_victim_seg[1]
```

When a previously identified next victim is available, the binary can return it directly and clears the corresponding slot. The logic is structurally the historical F2FS shortcut rather than a Transsion replacement.

## 4. Victim scoring / cost calculation

The X683 cost loop is one of the strongest proofs that the scoring algorithm remains stock F2FS.

### SSR score

At `0x35337c`:

```text
if alloc_mode == SSR:
    score = SIT_entry(segno)->ckpt_valid_blocks
```

The X683 field is loaded from the SIT entry at offset `+0x2`, masked with `0x3ff`.

This is the historical `get_gc_cost()` SSR case. The closest 4.14 source uses `ckpt_valid_blocks` for SSR.

### Greedy score

For the LFS/greedy policy path, X683 computes a valid-block cost from the SIT entry. For sectioned/large-section cases it sums the corresponding per-segment field over `segs_per_sec` entries and combines the segment type with the same weighted valid-block concept as historical `get_greedy_cost()`.

Representative direct accesses:

```text
SIT entry base + 0x68
SIT auxiliary array + 0x70
segment size = 0x28 bytes
```

The structure and arithmetic correspond to the historical greedy cost path.

### Cost-benefit / age score

For the non-greedy LFS policy, the X683 binary executes the full cost-benefit algorithm in-line. This is not a simplified vendor score.

Direct X683 behavior:

1. sum `mtime` across the section at SIT entry offset `+0x20`;
2. divide by `segs_per_sec`;
3. obtain valid blocks for the candidate section;
4. divide by `segs_per_sec`;
5. compute utilization:

```text
u = (vblocks * 100) >> sbi->log_blocks_per_seg
```

with:

```text
sbi + 0x3d8 = log_blocks_per_seg
```

6. update X683 SIT statistics:

```text
sit_info + 0x88 = min_mtime
sit_info + 0x90 = max_mtime
```

7. compute age:

```text
age = 0                              if max_mtime == min_mtime
age = 100 - 100*(mtime-min)/(max-min) otherwise
```

8. compute final cost:

```text
cost = UINT_MAX -
       (100 * (100-u) * age) / (100+u)
```

The compiler emits the final subtraction as `mvn`.

This is the historical 4.14 `get_cb_cost()` algorithm essentially instruction-for-instruction in control structure and arithmetic. The public v4.14 source uses the same `mtime`, `min_mtime`, `max_mtime`, utilization, age, and cost formula. urlChromiumOS Linux 4.14 F2FS gc.chttps://chromium.googlesource.com/chromiumos/third_party/kernel/+/v4.14/fs/f2fs/gc.c

### Minimum-cost selection

The binary keeps the smallest computed cost and the corresponding `segno` in the local policy state, then advances the candidate search. The comparator and min-cost initialization are the normal F2FS pattern.

**Conclusion:** no Transsion-specific cost/scoring function was found.

## 5. Age / mtime use

Age/mtime is **stock F2FS**, not a Transsion addition.

Direct X683 evidence:

```text
sit_info + 0x88  min_mtime
sit_info + 0x90  max_mtime
SIT entry + 0x20 mtime
sbi + 0x3d8      log_blocks_per_seg
```

The exact normalization and cost formula match the historical 4.14 `get_cb_cost()` implementation.

No vendor helper, vendor global, or vendor function call is inserted into this calculation.

Confidence: **VERY HIGH**.

## 6. SSR selection

The actual stock selector is `f2fs_need_SSR()` at `0xffffff92d0de58f8`.

Direct X683 inputs:

```text
sbi + 0x3e0  segs_per_sec
sbi + 0x3dc  blocks_per_seg
sbi + 0x434  current_reserved_blocks candidate/field used by stock math
sbi + 0x3d8  log_blocks_per_seg
sbi + 0x428  reserved_blocks
sbi + 0x440  nquota_files
sbi + 0x4ba  mount-option byte
sbi + 0x534  gc_mode
sbi + 0x48   filesystem flag word used by the stock shortcut
sbi + 0x80   sm_info
sm_info + 0x08 -> free_info
free_info + 0x94 and +0x5c -> free/dirty segment counts used in the threshold
```

Behavior is direct:

```c
if (LFS_option)
    return false;

if (sbi->gc_mode == 3)
    return true;

if (stock_force_ssr_flag)
    return true;

return free_sections <=
       dirty_node_sections +
       2 * dirty_data_sections +
       dirty_imeta_sections +
       min_ssr_sections +
       reserved_sections;
```

The threshold arithmetic is the stock historical F2FS SSR test. In particular, `gc_mode == 3` directly forces the SSR result to true in the X683 binary.

Historical 4.14 F2FS contains the same `GC_URGENT` fast path and free-section threshold in `f2fs_need_SSR()`. urlAndroid common / F2FS 4.14 lineagehttps://android.googlesource.com/kernel/common/+/refs/heads/upstream-f2fs-stable-linux-4.14.y/fs/f2fs/f2fs.h

**Conclusion:** there is no independently implemented Transsion SSR picker in the stock X683 core. Transsion changes whether stock SSR behavior is reached by changing `gc_mode` and by its external policy gates.

## 7. `tran_do_f2fs_gc()` — genuine Transsion delta

Function:

```text
0xffffff92d0dfada8
```

Direct binary behavior:

```c
++global[0x990];
cfg = global[0x998];

if (cfg == 0) {
    ret = f2fs_gc(sbi,
                  (sbi->mount_opt.opt >> 14) & 1,
                  1,
                  NULL_SEGNO);
} else {
    old_mode = sbi->gc_mode;

    if (cfg == 2)
        sbi->gc_mode = 3;
    else
        sbi->gc_mode = 2;

    ret = f2fs_gc(sbi,
                  (sbi->mount_opt.opt >> 14) & 1,
                  1,
                  NULL_SEGNO);

    sbi->gc_mode = old_mode;
}

++global[0x9a0];
return ret;
```

Direct SBI offset:

```text
sbi + 0x534 = gc_mode
```

This is the clearest proven vendor algorithmic boundary.

### Functional effect

The vendor code does not calculate a victim. Instead it selects the stock GC mode before the selector runs.

For `gc_mode == 3`, the stock X683 picker:

- bypasses `max_victim_search` limiting;
- reaches stock urgent-mode SSR forcing through `f2fs_need_SSR()`.

For `gc_mode == 2`, the stock selector uses its normal mode-to-policy mapping. The direct binary proves the value change, but this document does **not** force an upstream enum name onto the X683 internal policy table.

Confidence: **VERY HIGH** for the temporary mode override; **HIGH** for the downstream stock effects because those are directly visible in the X683 selector/SSR routines.

## 8. `tran_has_enough_free_segment()` — vendor entry gate, not victim picker

Function:

```text
0xffffff92d0dfb5d4
```

Direct inputs include:

```text
sbi + 0x80  sm_info
sbi + 0x408 user_block_count
sbi + 0x3d8 log_blocks_per_seg
sm_info + 0x00 -> sit_info
sit_info + 0x10
sm_info + 0x08 -> free_info
```

The helper also consumes vendor threshold tables in the Transsion data region and computes whether the filesystem has enough free segment capacity for the current policy.

It is called from the Transsion GC control path, not from the core victim loop.

Therefore its role is:

```text
Should Transsion invoke GC?
```

not:

```text
Which victim segment should stock F2FS select?
```

Confidence: **HIGH** for role and input fields; **MEDIUM** for the exact vendor threshold table semantics.

## 9. `is_f2fs_fragmentation()` — vendor entry gate, not victim scorer

Function:

```text
0xffffff92d0dfb580
```

Direct accesses include:

```text
vendor global +0x8a0 -> controller context
context +0x80       -> sm_info
context +0x408      -> user_block_count
context +0x3d8      -> log_blocks_per_seg
sm_info child pointers and segment metadata
```

The function derives a fragmentation percentage/condition and reports it through the Transsion control/debug path. It is not called by the stock victim loop and does not replace `get_victim_by_default()` or `get_gc_cost()`.

Therefore this is a vendor **policy gate**, not a vendor victim-selection algorithm.

Confidence: **HIGH** for gate role; **MEDIUM** for the exact semantic decomposition of every child field because some structure members are still intentionally unnamed.

## 10. `need_switch_ssr_read()` / `need_switch_ssr_write()`

Recovered vendor functions:

```text
need_switch_ssr_read  = 0xffffff92d0df87dc
need_switch_ssr_write = 0xffffff92d0df88e8
```

They operate on Transsion GC controller state. A direct call path from these helpers into the stock victim picker or `f2fs_need_SSR()` was not established.

Accordingly, they are classified as vendor control/state helpers, not proven replacements for SSR selection.

Confidence: **MEDIUM** for exact internal semantics; **HIGH** that they are outside the stock victim-scoring loop.

## 11. Migration policy

The X683 compiler inlines the logical `do_garbage_collect()` and `gc_data_segment()` boundaries directly into `f2fs_gc()`.

The recovered migration structure is:

```text
victim selection
  -> section/SIT/SSA preparation
  -> per-segment summary processing
     -> node migration OR data migration
  -> merged-write submission
  -> statistics/call accounting
  -> GC retry/checkpoint/cleanup
```

The data path preserves the historical five-phase sequence, including:

```text
NAT readahead
node-page readahead
node validation
inode/data preparation
phase-4 physical data movement
```

The X683 migration path contains stock F2FS page/block movement logic. No Transsion `tran_*` function call was found inside the core migration loop by direct branch-target scanning.

### X683-specific migration differences actually proven

1. Vendor/statistics layout differs from public source.
2. The GC wrapper can change `gc_mode` before migration starts.
3. X683 writes vendor/statistical counters around segment/data/node accounting.
4. X683 structure packing and offsets are vendor-specific.

No unique Transsion data-allocation or block-copy algorithm has been proven.

Confidence for stock migration lineage: **VERY HIGH**.

## 12. Direct caller/callee scope check

The X683 kallsyms and branch-target scan show:

- `f2fs_gc()` is at `0x3503a8`.
- `get_victim_by_default()` is a standalone helper called through the stock victim-selection path.
- `tran_do_f2fs_gc()` calls `f2fs_gc()` directly.
- `tran_has_enough_free_segment()` is called by the vendor controller.
- No direct `tran_*` BL target appears in the main `f2fs_gc()` victim/migration ranges inspected.
- `f2fs_need_SSR()` is a stock helper used outside the vendor controller, with X683 behavior matching historical F2FS.
- Lower-level `get_victim_by_cost`, `get_cb_cost`, `get_greedy_cost`, `do_garbage_collect`, `gc_data_segment`, and the physical data-movement helpers are largely inlined and therefore do not appear as independent kallsyms symbols.

This caller/callee boundary is consistent with a stock F2FS core wrapped by a vendor policy controller.

## 13. Vendor delta matrix

| Function / area | X683 field/offset | Observed behavior | Stock F2FS equivalent | Exact difference | Confidence |
|---|---|---|---|---|---|
| `tran_do_f2fs_gc()` | `sbi+0x534` | Temporarily writes `3` or `2`, calls normal `f2fs_gc`, restores | Stock GC is entered without this vendor wrapper | Adds vendor mode selection around stock GC | **VERY HIGH** |
| `tran_do_f2fs_gc()` | global `+0x998`, `+0x990`, `+0x9a0` | Config branch and call counters | No corresponding vendor layer | Vendor control/telemetry | **HIGH** |
| `get_victim_by_default()` search cap | `sbi+0x560`, `sbi+0x534` | `gc_mode==3` bypasses `max_victim_search` | Historical urgent-GC exception | **No picker code change; mode exposes stock urgent behavior** | **VERY HIGH** |
| victim filters | `sbi+0x530`, `+0x538`, `+0x53c`, SIT/section metadata | current victim exclusion, next-victim shortcut, section restrictions | Historical victim picker does same roles | No vendor-specific algorithm proven | **VERY HIGH** |
| SSR score | SIT `+0x2` | `ckpt_valid_blocks` used for SSR | `get_gc_cost()` SSR case | None | **VERY HIGH** |
| greedy score | SIT `+0x68`, aux `+0x70` | valid-block based weighted cost | `get_greedy_cost()` | None proven | **HIGH** |
| CB score / age | SIT entry `+0x20`; `sit_info+0x88/+0x90`; `sbi+0x3d8` | exact mtime/age/utilization cost formula | `get_cb_cost()` | None | **VERY HIGH** |
| `f2fs_need_SSR()` | `sbi+0x534`, free-info fields | `gc_mode==3` returns true, otherwise stock threshold | historical `f2fs_need_SSR()` | No custom selector; vendor reaches existing mode path | **VERY HIGH** |
| `tran_has_enough_free_segment()` | `sbi+0x408`, `+0x3d8`, `sm_info` children | vendor free-capacity gate before GC | no stock Transsion gate | Changes when vendor GC is invoked, not victim scoring | **HIGH** |
| `is_f2fs_fragmentation()` | context `+0x408`, `+0x3d8`, `+0x80` | vendor fragmentation gate/report | no stock Transsion gate | Changes when/mode policy, not victim scoring | **HIGH** |
| migration core | `sbi+0x564`, `+0x538/+0x53c` and vendor stats | stock section/node/data migration | historical `do_garbage_collect` / `gc_data_segment` | No custom migration engine proven | **VERY HIGH** |

## 14. Reconstructed complete model

```text
             Transsion GC controller
                        |
         +--------------+----------------+
         |                               |
  free-space gate                 fragmentation/state gate
  tran_has_enough_free_segment    is_f2fs_fragmentation
         |                               |
         +---------------+---------------+
                         v
                 tran_do_f2fs_gc
                         |
             save sbi->gc_mode
                         |
        cfg == 0 ? direct stock call :
           temporarily write 3 or 2
                         |
                         v
 f2fs_gc(sbi, (sbi->mount_opt.opt >> 14)&1, true, NULL_SEGNO)
                         |
                         v
             stock F2FS victim picker
                         |
      +------------------+-------------------+
      |                  |                   |
     SSR            greedy/LFS          CB / age
  ckpt_valid       valid-block cost     mtime + age
      |                  |                   |
      +------------------+-------------------+
                         |
                         v
                stock victim filters
                         |
                         v
              stock GC migration engine
           node path / data path / writes
                         |
                         v
                vendor/stat telemetry
                         |
                         v
                  restore gc_mode
```

The important point is that Transsion moves the **decision boundary outside** the stock picker. Once `f2fs_gc()` is entered, X683 still behaves like historical F2FS for victim scoring and migration.

## 15. Answers to the phase questions

### 1. Victim selection

Stock F2FS selection is retained. X683 `get_victim_by_default()` is structurally the historical picker, including dirty-list locking, policy setup, search limiting, candidate filtering, and minimum-cost selection.

### 2. Victim scoring / cost calculation

Stock F2FS scoring is retained. SSR uses `ckpt_valid_blocks`, greedy uses valid-block cost, and CB uses the historical age/mtime formula.

### 3. SSR selection

Stock `f2fs_need_SSR()` is retained. `gc_mode == 3` reaches the existing urgent true path; no vendor SSR replacement was proven.

### 4. Age / mtime

Stock. X683 contains the normal `min_mtime`/`max_mtime` tracking and CB formula.

### 5. Victim filtering

Stock. X683 shows the historical current-victim, next-victim, validity, section-use, and search-window restrictions. No unique Transsion filter inside the picker was proven.

### 6. Migration policy

Stock. The X683 migration engine is the historical F2FS section/node/data migration pipeline with vendor statistics/structure differences.

### 7. Does Transsion modify victim selection itself?

**Not proven. Current evidence says no.** The vendor changes `gc_mode` and therefore changes which existing stock policy/search behavior is activated.

### 8. Does Transsion only change when/mode stock `f2fs_gc()` runs?

**Yes, as far as the present binary evidence shows.** The vendor controller gates entry with free-space/fragmentation policy and changes `gc_mode` before the normal collector.

### 9. `get_victim_by_default()` / cost / SSR paths

The X683 implementations match the stock 4.14 lineage closely enough that no vendor-specific replacement has been identified. `get_cb_cost`-class age scoring is directly preserved; `f2fs_need_SSR()` directly preserves the stock urgent-mode path.

### 10. Vendor functions directly selecting or migrating victims

The directly relevant vendor functions are policy/control functions:

```text
tran_do_f2fs_gc()
tran_has_enough_free_segment()
is_f2fs_fragmentation()
need_switch_ssr_read()
need_switch_ssr_write()
```

Of these, only `tran_do_f2fs_gc()` is proven to directly invoke the stock collector. No vendor `tran_*` victim-selection or block-migration helper is proven inside the stock migration loop.

## 16. Genuine Transsion modifications vs stock F2FS

### Genuine Transsion

```text
- GC controller/thread policy
- free-space gating
- fragmentation gating
- temporary gc_mode overrides
- urgent/SSR behavior reached by those mode overrides
- vendor counters / telemetry
- vendor statistics layout/accounting
```

### Stock F2FS retained by X683

```text
- victim selection algorithm
- greedy cost
- cost-benefit scoring
- age/mtime calculation
- victim filtering
- SSR selector
- node migration
- data migration
- merged-write submission
- stock GC retry/cleanup architecture
```

### Not yet proven

```text
- exact semantic names of every object at sm_info+0x18 and its section-use fields
- exact source-level enum names for the X683 internal policy table
- exact vendor meanings of all global controller thresholds
- byte-for-byte identity to one exact upstream commit
```

## 17. Confidence summary

```text
VERY HIGH:
  stock victim picker retained
  stock CB/age formula retained
  stock SSR selector retained
  stock migration pipeline retained
  tran_do_f2fs_gc() mode override

HIGH:
  vendor free-space gate
  vendor fragmentation gate
  no vendor helper in core migration path
  section-level victim filtering correspondence

MEDIUM:
  exact semantic names for some vendor/child fields
  exact enum naming for X683 internal policy table
```

## 18. Phase conclusion

The actual Transsion delta is narrower than a custom GC implementation:

> **Transsion controls entry into and mode of the stock F2FS collector; it does not replace the stock victim scoring/migration engine in the X683 4.14 binary.**

The most important stock consequences of the vendor override are:

```text
gc_mode = 3
  -> bypass stock max_victim_search cap
  -> force stock f2fs_need_SSR() true

otherwise
  -> use normal stock picker/scoring paths selected by the mode
```

This phase is complete for the requested victim-selection/migration delta. The next reconstruction work can move to the remaining X683 F2FS source/layout areas rather than reopening GC victim selection.
