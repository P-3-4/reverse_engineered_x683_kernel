# X683/H694 F2FS 4.14 vendor delta

This is a binary-derived comparison against historical Android/common 4.14 F2FS. It is not proprietary-source recovery.

## Historical baseline

The Android/common 4.14 lineage exposes the same four-argument GC ABI used by X683:

```c
int f2fs_gc(struct f2fs_sb_info *sbi,
            bool sync,
            bool background,
            unsigned int segno);
```

Historical `gc.c` also follows the core sequence:

```text
select GC type
→ verify mounted / checkpoint-error state
→ optionally convert BG_GC to FG_GC when free space is insufficient
→ select victim
→ garbage-collect victim segment(s)
→ submit merged writes
→ update statistics
→ repeat or checkpoint as required
```

The 4.14 source also contains the standard `stat_inc_call_count()` accounting at the end of the segment GC path. cite historical Android/common 4.14 gc.c search result.

## X683 additions / replacements

### 1. Controller layer

X683 adds a vendor controller around the stock GC entry:

```text
+0x998 = 0 normal
        1 GREEDY
        2 URGENT
```

The wrapper temporarily changes `sbi->gc_mode`:

```text
controller 1 → gc_mode = 2
controller 2 → gc_mode = 3
```

then invokes the stock ABI with `segno = NULL_SEGNO` and restores the previous mode.

### 2. Static detector

X683 adds a detector loop around F2FS segment/free-space state. It maintains:

```text
+0x990 cycle counter
+0x9d4 detector state
+0x9e0 detector cycles
+0x9f0 running statistic
+0x9f4 saved baseline
+0x9f8 stop result
+0x9fc stop condition
+0xa04 cadence
+0xa08/+0xa0c progress baselines
```

The detector introduces five vendor stop predicates. Stops 1–3 set `+0x9fc`; Stops 4–5 set `+0x9f8` and may force controller 2.

### 3. Stop 4 / SSR trigger

Historical F2FS does not contain this X683 controller transition. X683 adds:

```text
segment/write delta > vendor threshold
→ Stop 4
→ controller = URGENT unless +0x9c0 blocks
→ stop_result = 1
→ later wrapper forces gc_mode = URGENT
```

The stock binary also logs that this condition is a switch-to-SSR trigger.

### 4. Stop 5 / no-progress trigger

X683 adds a periodic progress detector:

```text
cycle % (50 or 500) == 0
→ compare current progress to +0xa08/+0xa0c baselines
→ insufficient progress
→ controller = URGENT
→ stop_result = 2
```

This is not standard historical `f2fs_gc()` policy.

### 5. Vendor detector arithmetic

X683 adds the `0x37b5d4..0x37b748` helper family:

- user/sit segment span calculation;
- free-minus-reserved threshold;
- vendor factor table `{100,100,100,80,80,80,60,60}`;
- small-device scale table `{0x800,0xc00,0x1000,0x1000}`;
- signed fixed-point left-space calculation using `0xA3D70A3D70A3D70B`;
- fragmentation percentage logging.

These are Transsion policy additions around ordinary F2FS segment-manager data.

### 6. Vendor statistics

The X683 `stat_info` object contains binary-proven counters/aggregates at `+0x164` and `+0x170..0x198` whose exact names differ from or extend the historical structure. Historical 4.14 `f2fs_stat_info` includes `call_count`, GC counters, segment counts, block counts and related accounting. cite historical Android/common 4.14 f2fs.h search result.

The safe mapping is therefore:

```text
historical role correspondence = plausible
exact X683 member-name correspondence = not yet proven
```

### 7. Vendor control registration

X683 additionally registers controls named:

```text
need_switch_ssr
tran_urgent_gc
detect_charger_type
tran_gc_usb_wakelock
```

The first three are control descriptors in a common registration system, not standalone GC functions proven by direct calls.

## What remains stock

The core migration/data path remains recognizably historical F2FS:

```text
victim selection
→ summary-page access
→ node/data migration
→ merged write submission
→ segment statistics
→ completion
```

Thus the correct reconstruction strategy is **stock F2FS core + X683 vendor control/detector layer**, not a wholesale replacement of `gc.c`.

## Confidence

High:

- four-argument ABI;
- controller semantics;
- Stop 4/5 controller transition;
- detector arithmetic inputs;
- vendor factor/scale tables;
- vendor statistics object existence;
- historical 4.14 core GC correspondence.

Medium:

- exact names of X683 `stat_info` members;
- exact kthread sleep/abort integration;
- exact callback behavior of registered control descriptors.

Unresolved:

- separate `tran_gc_usb_wakelock` implementation path;
- final semantic binding of `need_switch_ssr`, `tran_urgent_gc`, and `detect_charger_type` descriptor values.
