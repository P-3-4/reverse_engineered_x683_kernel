# X683 Kernel Reverse Engineering — Current State Snapshot

Date: 2026-08-19
Repository: P-3-4/reverse_engineered_x683_kernel
Canonical working branch: `kernel-reconstruction-current`

## Completed

The F2FS segment-manager phase is complete enough to move on. Reconstructed against the stock X683 binary: `f2fs_sm_info`, `sit_info`, `free_segmap_info`, `dirty_seglist_info`, `curseg_info[6]`, `flush_cmd_control`, and `discard_cmd_control`.

Established sizes/relationships:

```text
sizeof(f2fs_sm_info)        = 0xA8
sizeof(sit_info)            = 0xA8
sizeof(free_segmap_info)    = 0x20
sizeof(dirty_seglist_info)  = 0x90
sizeof(curseg_info)         = 0x70
curseg_info count           = 6
curseg array size           = 0x2A0
sm_info + 0x98              = fcc_info
sm_info + 0xA0              = dcc_info
sizeof(discard_cmd_control) = 0x20B0
```

The dirty counters at `dirty_info + 0x68..0x7c` are the first six entries of `nr_dirty[8]`, not vendor counters. Earlier hypotheses about `sm_info + 0x40..0x47` and `+0x90..0x97` being vendor insertions were withdrawn during consumer proof.

## Transsion GC framework

Deep reconstruction covered `tran_gc_thread_func`, `tran_urgent_gc_read`, `tran_urgent_gc_write`, `tran_do_f2fs_gc`, `tran_gc_init`, and `tran_gc_stop`.

Conclusion: Transsion did not replace the core F2FS collector. It adds a policy/control layer around stock `f2fs_gc()` with urgent-GC state, GC type/mode control, thresholds, fragmentation/SSR decisions, timing, counters, and telemetry. `tran_do_f2fs_gc()` is a wrapper that selects a GC mode, invokes normal `f2fs_gc()`, and restores the previous mode.

`CONFIG_F2FS_TRAN_GC=y` is enabled in the X683 configuration.

## F2FS victim-selection / migration delta — completed 2026-08-19

The deep phase documented in `docs/reverse-engineering/x683-f2fs-victim-selection-migration-delta-2026-08-19.md` is complete.

### Correct X683 GC ABI

The stock X683 binary definitively uses the four-argument form:

```c
f2fs_gc(sbi,
        (sbi->mount_opt.opt >> 14) & 1,
        true,
        NULL_SEGNO);
```

The old three-argument statement in earlier exploratory GC notes is obsolete and must not be reused.

### Stock victim-selection algorithm retained

`get_victim_by_default()` at `0xffffff92d0dd2e74` is a standalone stock-like selector. The X683 binary preserves:

- dirty-list locking and policy construction;
- search limiting through `sbi + 0x560` (`max_victim_search`), except when the stock urgent-mode condition at `sbi + 0x534` is active;
- current-victim exclusion through `sbi + 0x530`;
- `next_victim_seg[2]` reuse through `sbi + 0x538/+0x53c`;
- SIT validity/checkpoint filtering;
- section-level candidate filtering;
- minimum-cost selection.

No independent Transsion victim picker was found.

### Stock cost / age algorithm retained

The X683 scoring path preserves the historical F2FS cases:

```text
SSR:     ckpt_valid_blocks
GREEDY:  valid-block cost / segment type weighting
CB:      mtime + utilization + age cost-benefit formula
```

Direct X683 CB fields include:

```text
SIT entry +0x20 = mtime
sit_info +0x88  = min_mtime
sit_info +0x90  = max_mtime
sbi +0x3d8      = log_blocks_per_seg
```

The CB formula matches the closest public 4.14 F2FS implementation. No vendor age/mtime algorithm has been proven.

### Stock SSR selector retained

`f2fs_need_SSR()` at `0xffffff92d0de58f8` matches the historical 4.14 threshold logic. In particular, the X683 binary directly returns true when:

```text
sbi + 0x534 == 3
```

which is the stock urgent-mode SSR fast path. The vendor does not supply a separate SSR selector.

### Genuine Transsion delta

`tran_do_f2fs_gc()` at `0xffffff92d0dfada8` is the strongest vendor-specific GC modification. It:

1. increments vendor telemetry;
2. reads a vendor configuration at global `+0x998`;
3. calls normal `f2fs_gc()` directly when the config is zero;
4. otherwise temporarily writes `sbi + 0x534` (`gc_mode`) to `3` or `2`;
5. calls the same four-argument `f2fs_gc()`;
6. restores the original mode;
7. records post-call telemetry.

Therefore Transsion changes **when and under which existing stock GC mode the collector runs**, not the stock victim-cost or migration algorithms themselves.

A `gc_mode == 3` run has stock consequences already present in X683: it bypasses the `max_victim_search` cap and forces the stock `f2fs_need_SSR()` result true.

### Vendor gates outside the core picker

The vendor helpers `tran_has_enough_free_segment()` and `is_f2fs_fragmentation()` are policy/entry gates. They inspect filesystem capacity/fragmentation state and determine whether the Transsion controller should initiate GC; they are not called as replacements for the stock victim scorer or migration engine.

`need_switch_ssr_read()` / `need_switch_ssr_write()` are retained as vendor controller-state helpers. A direct call path replacing the stock SSR picker has not been proven.

### Migration

The X683 compiler has inlined the logical `do_garbage_collect()` and `gc_data_segment()` boundaries into `f2fs_gc()`. The recovered five-phase node/data migration pipeline matches historical F2FS. No Transsion `tran_*` migration helper was found inside the core victim/migration loop by direct branch-target inspection.

The proven X683-specific differences inside the migration area are structure/statistics layout and vendor telemetry, plus the fact that the vendor controller can alter `gc_mode` before migration begins. No custom Transsion data-allocation/copy engine is proven.

### Phase conclusion

The actual X683 Transsion GC delta is best modeled as:

```text
Transsion controller policy
        -> free-space / fragmentation / urgency gates
        -> temporary gc_mode override
        -> stock f2fs_gc()
             -> stock victim filtering
             -> stock SSR/greedy/CB scoring
             -> stock age/mtime scoring
             -> stock node/data migration
```

The detailed evidence table and reconstruction are in:

`docs/reverse-engineering/x683-f2fs-victim-selection-migration-delta-2026-08-19.md`

## Next phase

Move to the remaining X683 F2FS source/layout reconstruction and integration work. Do not reopen victim-selection or migration unless new binary evidence contradicts the completed phase.

## Repository cleanup status

Many exploratory branches exist. Several GC branches are byte-for-byte identical at their tips; for example `gc-deep-pass`, `gc-deep-pass-final`, `gc-deep-pass-final2`, and `gc-pass-final` all resolve to commit `ac5db07b8b673d5b80aba800efb5a909179d6f32`.

The canonical continuation branch is now:

`kernel-reconstruction-current`

It contains the completed deep-GC state, the victim-selection/migration phase document, and this updated snapshot.

An archival branch preserves the pre-GC reconstruction state:

`archive/reconstruction-f2fs-balance-delta-2026-08-19`

Older exploratory branches should be treated as historical unless a future comparison demonstrates unique evidence in them.
