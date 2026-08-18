# X683/H694 GC reverse-engineering — five-pass final status

This is the consolidated status after completing the requested passes #1 through #5 against the verified stock Image. All code is reconstructed/inferred and is not proprietary Transsion source.

## Binary authority

Boot SHA-256:
`a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`

Decompressed Image SHA-256:
`96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

## Pass 1 — threshold/helper

Completed.

### `0x37b580`

Recovered fragmentation arithmetic:

```c
user_segments = sbi->user_block_count >> sbi->log_blocks_per_seg;
sit_segments  = sit_blocks >> sbi->log_blocks_per_seg;
span          = user_segments - sit_segments;
free_percent  = (free_segments * 100) / span;
fragmentation = 100 - free_percent;
```

The function logs:

`f2fs alloc new segment and fragmentation is %lu`

### `0x37b5d4..0x37b748`

Recovered as one boolean free-space/fragmentation policy helper.

Exact scale selection:

```c
if ((user_segments >> 15) == 0)
    scale = {0x800, 0xc00, 0x1000, 0x1000}[user_segments >> 13];
else
    scale = 0x1800;
```

Selector:

```c
selector = max(global_selector_a, global_selector_b);
```

Factor:

```text
{100,100,100,80,80,80,60,60}
```

Stop 2:

```c
delta = (s32)(free_segments - reserved_segments);
threshold = (factor * scale * 0x51EB851F) >> 37;
stop2 = delta > threshold;
```

Stop 3:

```c
span = (s64)user_segments - (s64)sit_segments;
reference = (s64)(s32)(free_segments - reserved_segments);
p = table64[selector] * span;
high = smulh(p, 0xA3D70A3D70A3D70B);
scaled = (high + p) >> 6;
scaled += (p < 0);
stop3 = scaled < reference;
```

The post-`0x37b74c` area is generic attribute/control machinery, not more GC threshold arithmetic.

## Pass 2 — complete detector/thread model

Completed to binary-supported level.

Recovered controller fields:

```text
+0x990 cycle
+0x998 controller 0/1/2
+0x9c0 controller write guard
+0x9d0 loop state
+0x9d4 detector state
+0x9d8 repeated count
+0x9e0 detector cycles
+0x9f0 running statistic/maximum
+0x9f4 saved recoverable baseline
+0x9f8 Stop4/Stop5 result
+0x9fc Stop1/2/3 condition
+0xa00 detector mode
+0xa04 50/500 cadence selector
+0xa05 loop active
+0xa06 detector enabled
+0xa08 segment baseline
+0xa0c written/recoverable baseline
```

State 3 is a real timed wait/recheck path using the verified waitqueue/scheduler primitives.

Stop outputs:

```text
Stop1 -> +0x9fc = 1
Stop2 -> +0x9fc = 2
Stop3 -> +0x9fc = 3
Stop4 -> +0x998 = 2 unless +0x9c0; +0x9f8 = 1
Stop5 -> +0x998 = 2; +0x9f8 = 2
```

A unified source layer now exists at:

`fs/f2fs/tran_gc.c`

The source deliberately takes unresolved task/vendor predicates as callbacks instead of inventing implementations.

## Pass 3 — wrapper

Completed.

At `0x37ada8..0x37af00`:

```text
controller 0 -> f2fs_gc(sbi, sync, background, NULL_SEGNO)
controller 1 -> save gc_mode, set 2, call, restore
controller 2 -> save gc_mode, set 3, call, restore
```

The stock ABI is the four-argument form.

## Pass 4 — historical 4.14 delta

Completed at the architectural level.

Historical Android/common 4.14 F2FS has the same four-argument GC entry point and the standard sequence:

```text
checkpoint/error checks
→ BG/FG escalation
→ victim selection
→ segment migration
→ merged-write submission
→ statistics
→ repeat/checkpoint
```

The X683 vendor additions are classified as:

```text
stock F2FS GC core
+
Transsion controller
+
Transsion detector
+
Stops 1–5
+
vendor statistics
+
vendor control/debug registration
```

The reconstruction no longer unlocks `gc_mutex` internally after the caller-owned GC critical section; that previous source defect was corrected.

Reference historical sources:
- Android/common 4.14 `fs/f2fs/gc.c`
- Android/common F2FS tree at the 4.14 release line.

## Pass 5 — vendor controls

Completed to the evidence ceiling.

Proven control registrations:

```text
need_switch_ssr
  registration 0x37af88
  descriptor 0x173b9d0

tran_urgent_gc
  registration 0x37b068
  descriptor 0x173bbb0

detect_charger_type
  registration 0x37b184
  descriptor 0x173bf70
```

They all use the common registration/object path:

```text
control name
→ common runtime object
→ 0x274ea0
→ 0x274dac
→ per-control descriptor
→ generic attribute operation
```

They are therefore not proven standalone functions.

`tran_gc_usb_wakelock` remains a distinct/unresolved registration/use path.

No invented direct calls to `need_switch_ssr`, `tran_urgent_gc`, or `detect_charger_type` are present in the reconstruction.

## Statistics

` sbi + 0x568 ` is `struct f2fs_stat_info *`.

Confirmed direct members:

```text
+0x164 incremented
+0x174 incremented
+0x178 incremented
+0x184 accumulated
+0x188 accumulated
+0x18c incremented
+0x190 incremented
+0x198 accumulated
```

Exact historical source names remain intentionally unassigned.

## Source tree after passes 1–5

```text
fs/f2fs/
  gc_reconstructed.c
  tran_gc.c
  tran_gc_reconstructed.c
  tran_gc_thread_reconstructed.c
  tran_gc_threshold_reconstructed.c
  tran_gc_wrapper_reconstructed.c
  victim_reconstructed.c
```

Documentation includes:

```text
docs/reverse-engineering/
  gc-threshold-helper-exact-reconstruction.md
  stat-info-vendor-control-correlation.md
  vendor-control-final-status.md
  f2fs-414-vendor-delta.md
  gc-five-pass-final.md
```

## Remaining blockers

These are not being hidden as completed facts:

1. exact generic attribute callback bodies for the three named controls;
2. separate `tran_gc_usb_wakelock` path;
3. exact semantic source name of `0xcc774`;
4. exact meaning of vendor global `+0x974` and controller-object `+0x20`;
5. exact scheduler wrapper surrounding the recovered state-3 wait;
6. exact historical names of the X683 `stat_info` members.

The core Transsion GC architecture and all Stop 1–5 controller behavior are reconstructed. The remaining items are the last vendor-specific attribution/implementation details, not gaps in the basic GC control-flow model.
