# X683/H694 F2FS GC state / FG-threshold deep pass

## Conclusion

The previous target was framed too narrowly as `build_gc_manager() -> fggc_threshold -> no_fggc_candidate()`. The recovered X683 layout contains `gc_mode` at `f2fs_sb_info + 0x534`. Historical 4.14 F2FS instead used `fggc_threshold` in this part of the structure. A later F2FS GC-state change explicitly replaced `fggc_threshold` with `gc_mode`.

Therefore the X683 binary is not a clean upstream 4.14.141 F2FS GC implementation. It incorporates a later GC-state lineage and/or a vendor backport of that change. We must not reconstruct `fggc_threshold` at the recovered `0x534` field.

## 1. Upstream 4.14 baseline

The 4.14-era F2FS tree added:

- `cur_victim_sec`;
- `u64 fggc_threshold`;
- `max_victim_search`.

`build_gc_manager()` initializes `DIRTY_I(sbi)->v_ops` and computes `fggc_threshold` from main/reserved/overprovisioned segment counts and blocks per section.

The corresponding `no_fggc_candidate()` rejects a section when its valid-block count reaches the threshold.

## 2. Later GC-state transition

A later F2FS change replaced the `fggc_threshold` field with:

    unsigned int gc_mode;

and added GC skip-state accounting. The same change also introduced GC-state selection machinery and later evolved into the modern GC state model.

This is directly relevant because the recovered X683 field at `0x534` is identified as `gc_mode`.

## 3. Consequence for X683

The following assumption is now rejected:

    sbi + 0x534 == fggc_threshold

The recovered evidence instead supports:

    sbi + 0x534 == gc_mode

Therefore a stock-equivalent reconstruction should not write a 64-bit threshold through that location.

Likewise, the existence of `no_fggc_candidate()` in the exact X683 revision remains unresolved. Its presence in historical 4.14/early-4.15 source cannot by itself prove that the vendor tree retained it after replacing the threshold field.

## 4. `build_gc_manager()` that can be retained

The high-confidence portion is:

```c
void build_gc_manager(struct f2fs_sb_info *sbi)
{
    DIRTY_I(sbi)->v_ops = &default_v_ops;
}
```

The historical threshold calculation must remain quarantined unless stock evidence identifies a separate threshold field.

The historical formula was:

```c
main_count = SM_I(sbi)->main_segments << sbi->log_blocks_per_seg;
resv_count = SM_I(sbi)->reserved_segments << sbi->log_blocks_per_seg;
ovp_count  = SM_I(sbi)->ovp_segments << sbi->log_blocks_per_seg;
blocks_per_sec = sbi->blocks_per_seg * sbi->segs_per_sec;

fggc_threshold = div64_u64(
    (main_count - ovp_count) * blocks_per_sec,
    (main_count - resv_count));
```

This formula is historical reference material, not yet an X683 claim.

## 5. Important victim-selection correction

The previous reconstruction incorrectly used:

```c
return get_valid_blocks(sbi, segno, true);
```

for every greedy victim.

The historical greedy cost is type-sensitive:

```c
valid_blocks = get_valid_blocks(sbi, segno, true);
return IS_DATASEG(get_seg_entry(sbi, segno)->type) ?
       valid_blocks * 2 : valid_blocks;
```

Thus data segments carry double greedy cost relative to node segments with the same valid-block count. The reconstructed source has been corrected.

## 6. What the `gc_mode` finding implies

`gc_mode` is not the mount option word at `0x4b8`. The known X683 layout therefore has two separate GC controls:

```text
sbi + 0x4b8 -> mount_opt.opt
sbi + 0x534 -> gc_mode
```

The stock wrapper already uses bit 14 of `mount_opt.opt` as the `sync`/force-FG-GC selector. `gc_mode` is consequently internal GC state/policy, not the trigger flag.

## 7. Likely revision family

The combination of:

- three-argument `f2fs_gc(sbi, sync, background)`;
- `gc_mode` in `f2fs_sb_info`;
- `SIT_I()->last_victim[]` policy cursor;
- dirty-segment victim bitmap;
- historical victim-selection machinery;

points to a vendor tree assembled from multiple nearby upstream F2FS revisions rather than a pristine tag.

This explains why copying a single upstream 4.14 or 4.15 `gc.c` wholesale produces field/API contradictions.

## 8. New reverse-engineering priority

The next target is no longer the threshold formula. It is the **exact GC-state transition path**:

```text
GC trigger
  -> sync/background classification
  -> gc_mode write
  -> select_gc_type()
  -> victim policy
  -> last_victim[gc_mode]
  -> GC completion
  -> gc_mode restoration/transition
```

Specifically, stock instructions around `sbi+0x534` should be classified as:

- byte/halfword/word load/store;
- compare against `GC_NORMAL`, `GC_IDLE_CB`, `GC_IDLE_GREEDY`, `GC_URGENT`-family constants;
- direct index into `last_victim[]`;
- switch/table dispatch;
- transient state assignment.

That will identify the exact vendor backport boundary far more reliably than source-date matching.

## Confidence

| Finding | Confidence |
|---|---|
| `sbi+0x534` is `gc_mode` | High |
| `mount_opt.opt` and `gc_mode` are distinct | High |
| historical `fggc_threshold` occupied an older lineage position | High |
| X683 is a mixed/backported F2FS GC lineage | High |
| X683 retains `no_fggc_candidate()` | Unresolved |
| X683 has a separate FG threshold field elsewhere | Unresolved |
| exact `gc_mode` enum/value set | Medium |
| exact vendor GC-state transitions | Unresolved |
