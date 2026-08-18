# X683/H694 F2FS victim-selection deep pass

## Scope

Target: X683/H694 MT6768, Linux 4.14.141-era F2FS.

This pass resolves the generic dirty-segment victim-selection machinery around `DIRTY_I(sbi)->v_ops->get_victim()` and separates the 4.14 target from later 4.15 changes.

## 1. `sentry_lock` revision boundary

The previous reconstruction incorrectly promoted the later 4.15 lock form to the X683 target.

The relevant history shows the older GC wrapper using:

```c
mutex_lock(&sit_i->sentry_lock);
ret = DIRTY_I(sbi)->v_ops->get_victim(...);
mutex_unlock(&sit_i->sentry_lock);
```

The 4.15 development line later converted this to `down_write()/up_write()` as part of the broader `sentry_lock` rwsem conversion. The 4.14 stable lineage still contains `mutex_lock(&sit_i->sentry_lock)` in the corresponding segment-management code, so the X683 4.14.141 reconstruction retains the mutex until stock binary evidence says otherwise.

This is now a **revision-specific correction**, not an unresolved generic lock choice.

## 2. `last_victim[]` owner

The victim-selection implementation in the later 4.15 lineage uses:

```c
struct sit_info *sm = SIT_I(sbi);
sm->last_victim[p.gc_mode]
```

Earlier development history temporarily exposed the cursor through `sbi`; the later refactor moved/normalized it through `sit_info`. Therefore the semantic identity is definitely the GC search cursor, while the exact X683 storage owner must be matched against the stock binary before declaring the C member placement final.

The current reconstruction uses `SIT_I(sbi)->last_victim[]` because it matches the recovered manager topology and the target-era source family.

## 3. Policy selection

For LFS GC:

- `GC_CB` for BG_GC by default.
- `GC_GREEDY` for FG_GC by default.
- dirty bitmap = `dirty_segmap[DIRTY]`.
- search count = `nr_dirty[DIRTY]`.
- search unit = `segs_per_sec`.

For SSR:

- `GC_GREEDY`.
- type-specific dirty bitmap/count.
- search unit = 1.

Hot-data/node selection starts from offset zero in the historical policy code; other LFS selection starts from the GC-mode-specific last-victim cursor.

## 4. Cost model

### Greedy

```c
cost = get_valid_blocks(sbi, segno, true);
```

Lowest valid-block count wins.

### Cost-benefit

For each section, average modification time and valid blocks are calculated. Utilization and normalized age produce:

```c
UINT_MAX - ((100 * (100 - u) * age) / (100 + u))
```

Lower cost wins.

### SSR

Checkpoint-valid blocks are used as the cost.

## 5. Candidate scan

The selector:

1. locks `dirty_i->seglist_lock`;
2. initializes the policy;
3. starts from `last_victim[gc_mode]` where applicable;
4. scans dirty entries with `find_next_bit()`;
5. aligns LFS searches to `segs_per_sec`;
6. counts dirty candidates toward `max_search`;
7. rejects unsafe sections;
8. rejects BG sections already present in `victim_secmap`;
9. rejects FG/LFS sections with `no_fggc_candidate()`;
10. calculates the candidate cost;
11. records the minimum-cost victim;
12. updates the search cursor and wraps it at the main-area segment count.

## 6. BG → FG handoff

BG GC places the selected section in `dirty_i->victim_secmap`.

FG GC can preferentially consume these candidates through `check_bg_victims()`, subject to `sec_usage_check()` and `no_fggc_candidate()`.

```text
BG candidate
    -> victim_secmap
    -> check_bg_victims()
    -> FG victim
```

## 7. `do_garbage_collect()` return semantics

For the three-argument target lineage, the reconstructed helper returns a **section-freed boolean/count of one**, not a raw segment count:

```c
return (gc_type == FG_GC &&
        seg_freed == sbi->segs_per_sec);
```

The caller then increments `sec_freed` when the helper succeeds during foreground GC.

This distinction is important and has been corrected in `gc_reconstructed.c`.

## 8. Three-argument ABI boundary

The historical change that added the fourth `segno` argument also introduced `init_segno` and `FLUSH_DEVICE` cursor restoration. Those later fields must not be imported into the X683 three-argument implementation without binary evidence.

The current X683 reconstruction therefore does not claim an `init_segno`/`FLUSH_DEVICE` reset.

## 9. X683 layout correlation

Known high-confidence relationships:

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

The candidate selector directly exercises `log_blocks_per_seg`, `blocks_per_seg`, and `segs_per_sec`; the search cursor and mtime range come from the SIT manager.

## 10. Stock-specific unresolved work

- Transsion modifications to `select_gc_type()`.
- Transsion candidate scoring or extra exclusions.
- Exact X683 `last_victim[]` storage offset.
- Exact binary width/packing of `gc_mode` at `sbi + 0x534`.
- Exact X683 `gc_node_segment()` and `gc_data_segment()` revisions.
- Exact `build_gc_manager()` / `fggc_threshold` implementation.

## Confidence

| Item | Confidence |
|---|---|
| Dirty-manager victim path | High |
| `seglist_lock` selection boundary | High |
| BG/FG victim bitmap relationship | High |
| Greedy/CB cost model | High |
| Section-aligned scan | High |
| Three-argument ABI | High |
| 4.14 mutex lock revision | High |
| Exact X683 cursor storage | Medium |
| Transsion scoring delta | Unresolved |

## Evidence

The 4.15 source directly exposes the policy, cost, dirty-bitmap scan and victim handoff machinery. citeturn2view0

The historical API change confirms that the fourth victim-segment parameter and `FLUSH_DEVICE` restoration belong to a later revision. citeturn10search6

The 4.14-era history shows the older three-argument `f2fs_gc()` and the older `mutex_lock(sentry_lock)` form before the rwsem conversion. citeturn11search10turn12search8
