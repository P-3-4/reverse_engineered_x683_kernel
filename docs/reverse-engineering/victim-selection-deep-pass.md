# X683/H694 F2FS victim-selection deep pass

## Scope

Target: X683/H694 MT6768, Linux 4.14.141-era F2FS.

This pass resolves the generic dirty-segment victim-selection machinery surrounding `DIRTY_I(sbi)->v_ops->get_victim()` and corrects two earlier reconstruction errors: the `sentry_lock` primitive and the owner of `last_victim[]`.

## 1. Locking correction

The earlier reconstruction used `mutex_lock(&SIT_I(sbi)->sentry_lock)`. That is incorrect.

The historical 4.15 implementation uses:

```c
down_write(&SIT_I(sbi)->sentry_lock);
ret = DIRTY_I(sbi)->v_ops->get_victim(...);
up_write(&SIT_I(sbi)->sentry_lock);
```

The X683 reconstruction now follows that shape. The binary must still be checked for a vendor-local lock primitive, but the previous mutex claim is removed.

## 2. `last_victim[]` owner correction

`last_victim[]` is associated with `sit_info`, not directly with `f2fs_sb_info` in the historical target lineage:

```c
struct sit_info *sm = SIT_I(sbi);
sm->last_victim[p.gc_mode]
```

This is important because the GC search cursor and the SIT manager are coupled. The recovered X683 manager relationship `sm_info -> sit_info` therefore explains the cursor accesses without requiring an additional `f2fs_sb_info` field.

## 3. Policy selection

For LFS GC:

- `gc_mode = GC_CB` for BG_GC by default.
- `gc_mode = GC_GREEDY` for FG_GC by default.
- dirty bitmap = `dirty_segmap[DIRTY]`.
- search count = `nr_dirty[DIRTY]`.
- search unit = `segs_per_sec`.

For SSR:

- `gc_mode = GC_GREEDY`.
- bitmap = type-specific dirty bitmap.
- search count = type-specific `nr_dirty`.
- search unit = 1 segment.

Historical 4.15 also resets the search offset to zero for hot-data/node selection and otherwise starts from `SIT_I(sbi)->last_victim[p.gc_mode]`.

## 4. Cost model

The historical cost path is now reconstructed explicitly.

### Greedy

For LFS + `GC_GREEDY`:

```c
cost = get_valid_blocks(sbi, segno, true);
```

The minimum valid-block victim wins.

### Cost-benefit

For LFS + `GC_CB`:

1. Average segment modification time across the section.
2. Average valid blocks across the section.
3. Compute utilization `u` as a percentage.
4. Maintain `min_mtime` / `max_mtime` in `sit_info`.
5. Convert the age to a 0..100 scale.
6. Return:

```c
UINT_MAX - ((100 * (100 - u) * age) / (100 + u))
```

This makes lower returned cost preferable.

### SSR

SSR uses checkpoint-valid blocks rather than the LFS GC cost function.

## 5. Candidate scan

The scan is section-aware when `ofs_unit > 1`:

1. Start at `p.offset`.
2. Find the next dirty bit.
3. Advance to the next section boundary.
4. Count dirty bits in that section toward `nsearched`.
5. Convert segment → section.
6. Reject unsafe/used sections.
7. Reject a section already reserved as a BG victim when running BG_GC.
8. For FG_GC/LFS, reject sections with no suitable FG candidate.
9. Calculate cost.
10. Keep the lowest-cost segment.

This is the core candidate-selection state machine.

## 6. Search cursor behavior

At the end of a bounded search, the historical code updates:

```c
sm->last_victim[p.gc_mode]
```

using the previous cursor and the last scanned segment, then wraps it by `MAIN_SEGS(sbi)`.

This is not merely an optimization: it prevents repeated scanning of the same dirty range and therefore is part of the observable GC state machine.

## 7. BG/FG victim handoff

Background GC marks the selected section in `dirty_i->victim_secmap`.

Foreground GC can first consume entries from that map using `check_bg_victims()`; sections are rejected if `sec_usage_check()` or `no_fggc_candidate()` says they are unsuitable.

Therefore the manager has a two-stage relationship:

```text
BG candidate
   -> victim_secmap
   -> FG check_bg_victims()
   -> FG victim
```

This is stronger than treating BG and FG selection as independent scans.

## 8. `f2fs_gc()` API boundary

Historical source shows the victim-segment argument was added after the three-argument API. The patch that added the fourth argument also introduced `init_segno` and the `FLUSH_DEVICE` cursor restoration. Therefore the X683 three-argument target must **not** import `init_segno`/`FLUSH_DEVICE` restoration merely because it appears in later 4.15 trees.

The current reconstruction consequently does not claim a `FLUSH_DEVICE` reset for the X683 three-argument ABI.

## 9. Recovered X683 layout correlation

Known high-confidence relationships remain:

```text
f2fs_sb_info
  -> sm_info
       +0x00 sit_info
       +0x08 free_info
       +0x10 dirty_info
       +0x60 reserved_segments

f2fs_sb_info
  +0x3d8 log_blocks_per_seg
  +0x3dc blocks_per_seg
  +0x3e0 segs_per_sec
  +0x4b8 mount_opt.opt
  +0x534 gc_mode (semantic identity: high confidence)
```

The candidate selector directly exercises `log_blocks_per_seg`, `blocks_per_seg`, and `segs_per_sec`, while the cursor state is supplied by `sit_info`.

## 10. What this pass proves vs. does not prove

### Strongly anchored

- `sentry_lock` is a write semaphore in the historical GC wrapper.
- `DIRTY_I()->seglist_lock` protects victim selection.
- `DIRTY_I()->victim_secmap` is the BG→FG handoff bitmap.
- `SIT_I()->last_victim[]` is the search cursor.
- `GC_GREEDY`/`GC_CB` selection and their cost functions.
- section-sized LFS search via `segs_per_sec`.

### Still stock-specific / unresolved

- Any Transsion modification to `select_gc_type()`.
- Any Transsion change to candidate cost/scoring.
- Any vendor-specific exclusion predicate not present in the historical code.
- Exact binary field widths for `gc_mode` at `sbi + 0x534`.
- Exact X683 `gc_node_segment()` / `gc_data_segment()` body.
- Whether X683 retained the historical `build_gc_manager()` `fggc_threshold` calculation unchanged.

## Evidence

The 4.15 source directly shows the lock/dispatch boundary, policy selection, cost functions, dirty bitmap scan, BG victim bitmap, `last_victim[]` update, and `cur_victim_sec` assignment. citeturn2view0

The later API-extension patch explicitly shows that the fourth `segno` argument, `init_segno`, and `FLUSH_DEVICE` restoration were added together, providing a clean revision boundary for the X683 three-argument target. citeturn10search6

The Linux F2FS documentation independently describes greedy selection as minimum valid blocks and cost-benefit selection as combining segment age with valid-block count. citeturn0search0
