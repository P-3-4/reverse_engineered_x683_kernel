# X683/H694 F2FS GC reconstruction

## ABI match

Direct X683 binary evidence proves that the stock GC call uses the four-argument form:

```c
f2fs_gc(
    sbi,
    (sbi->mount_opt.opt >> 14) & 1,
    true,
    NULL_SEGNO);
```

The wrapper at `tran_do_f2fs_gc()` directly supplies the fourth argument as `NULL_SEGNO` (`-1`). The obsolete three-argument description that was present in this document is superseded.

The X683 target is therefore:

```c
f2fs_gc(sbi, sync, background, segno);
```

with the Transsion caller using `background = true` and `segno = NULL_SEGNO`.

## Reconstructed stock state machine

The X683 `f2fs_gc()` preserves the historical 4.14 F2FS control structure:

1. validate the filesystem/checkpoint state;
2. determine foreground/background GC mode;
3. handle prefree/free-section pressure and GC promotion;
4. reject background GC in paths where background collection is disallowed;
5. select a victim through the stock `__get_victim()` / `get_victim_by_default()` path;
6. migrate the selected victim with the stock section/node/data migration engine;
7. account freed segments/sections and vendor/statistics counters;
8. perform the GC-more/retry logic;
9. checkpoint/cleanup as required;
10. reset victim bookkeeping and release the GC inode list.

The logical `do_garbage_collect()` and `gc_data_segment()` bodies are heavily inlined into the X683 `f2fs_gc()` function, but their structure matches the historical 4.14 implementation.

## Victim-selection result

The completed deep pass establishes that the stock X683 victim selector itself is not replaced by a Transsion algorithm.

`get_victim_by_default()` remains a standalone function at `0xffffff92d0dd2e74` and retains the historical victim-selection structure:

- dirty-list locking and policy setup;
- SSR/LFS policy distinction;
- `max_victim_search` limiting;
- current-victim and next-victim handling;
- SIT validity/checkpoint filtering;
- section-level candidate filtering;
- minimum-cost selection.

The X683 search-cap condition directly uses:

```text
sbi + 0x534 = gc_mode
sbi + 0x560 = max_victim_search
```

and leaves the search uncapped when the urgent-mode value `3` is active.

## Victim scoring result

The X683 scoring path preserves the historical F2FS cases:

```text
SSR      -> ckpt_valid_blocks
GREEDY   -> valid-block cost / segment-type weighting
CB       -> age/mtime cost-benefit
```

The CB calculation directly uses:

```text
SIT entry +0x20 = mtime
sit_info +0x88  = min_mtime
sit_info +0x90  = max_mtime
sbi +0x3d8      = log_blocks_per_seg
```

and computes the same utilization, age, and final cost formula as historical 4.14 F2FS. No vendor-specific age or cost formula was found.

## SSR result

`f2fs_need_SSR()` is at `0xffffff92d0de58f8`.

Its behavior remains stock-like, including:

```text
if LFS option is active -> false
if sbi + 0x534 == 3     -> true
otherwise               -> free-section threshold test
```

The X683 threshold uses the same reserved/dirty/free segment relationships as the historical 4.14 F2FS implementation. The `gc_mode == 3` shortcut is therefore an existing stock urgent-mode semantic exposed by the vendor wrapper.

## Transsion-specific boundary

The genuine Transsion modification is outside the victim scoring loop.

`tran_do_f2fs_gc()` at `0xffffff92d0dfada8`:

```c
++global[0x990];
cfg = global[0x998];

if (cfg == 0) {
    ret = f2fs_gc(sbi, (sbi->mount_opt.opt >> 14) & 1,
                  true, NULL_SEGNO);
} else {
    old_mode = sbi->gc_mode;
    sbi->gc_mode = (cfg == 2) ? 3 : 2;
    ret = f2fs_gc(sbi, (sbi->mount_opt.opt >> 14) & 1,
                  true, NULL_SEGNO);
    sbi->gc_mode = old_mode;
}

++global[0x9a0];
```

This changes the **mode and entry policy** of the stock collector, not the underlying victim-score equations.

For `gc_mode == 3`, the stock X683 selector bypasses the `max_victim_search` cap and the stock X683 `f2fs_need_SSR()` returns true. Those are stock behaviors, not new Transsion algorithms.

The vendor helpers `tran_has_enough_free_segment()` and `is_f2fs_fragmentation()` are policy gates used by the Transsion control layer before entering GC. No direct vendor replacement for the victim picker or migration engine has been proven.

## Migration result

The binary retains the historical F2FS migration sequence:

```text
victim
  -> SSA/summary preparation
  -> segment iteration
  -> node/data GC
  -> merged-write submission
  -> accounting
  -> retry/checkpoint/cleanup
```

The data path retains the historical five-phase `gc_data_segment()` pipeline. No Transsion `tran_*` block-migration helper was found in the core migration loop by direct branch-target inspection.

Proven X683-specific differences in the migration area are:

- vendor-diverged structure/statistics layout;
- vendor/statistics accounting;
- mode selection performed by the external Transsion wrapper;
- X683-specific packing/offsets.

No custom Transsion data-allocation or physical-copy engine is proven.

## X683 field correlation

The recovered layout establishes:

- `sbi + 0x3d8`: `log_blocks_per_seg`
- `sbi + 0x3dc`: `blocks_per_seg`
- `sbi + 0x3e0`: `segs_per_sec`
- `sbi + 0x408`: `user_block_count`
- `sbi + 0x428`: `reserved_blocks`
- `sbi + 0x430`: `current_reserved_blocks`
- `sbi + 0x438`: `unusable_block_count`
- `sbi + 0x440`: `nquota_files`
- `sbi + 0x4b8`: `mount_opt.opt`
- `sbi + 0x508`: `gc_mutex`
- `sbi + 0x528`: `gc_thread`
- `sbi + 0x530`: `cur_victim_sec`
- `sbi + 0x534`: `gc_mode`
- `sbi + 0x538/+0x53c`: `next_victim_seg[2]`
- `sbi + 0x560`: `max_victim_search`
- `sbi + 0x564`: `migration_granularity`
- `sbi + 0x568`: `stat_info` pointer

The `stat_info` pointer is a separate object; `sbi + 0x570..0x5dc` are not `stat_info` members.

## Confidence

- four-argument X683 GC ABI: **VERY HIGH**
- stock victim-selection algorithm retained: **VERY HIGH**
- stock CB/age/mtime cost retained: **VERY HIGH**
- stock SSR selection retained: **VERY HIGH**
- stock migration architecture retained: **VERY HIGH**
- `tran_do_f2fs_gc()` mode override: **VERY HIGH**
- exact semantics of every vendor threshold/global: **not fully resolved**
- exact one-commit upstream ancestry: **not yet proven**

## Detailed phase document

See:

`docs/reverse-engineering/x683-f2fs-victim-selection-migration-delta-2026-08-19.md`
