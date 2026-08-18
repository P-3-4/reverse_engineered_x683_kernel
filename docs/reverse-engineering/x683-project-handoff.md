# X683 / H694 Kernel Reverse-Engineering — Project Handoff

Canonical continuation state. Work from `main`.

## Binary authority

- boot.img SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- boot.img size: `33,554,432`
- kernel slot: `0x94dad4` / `9,755,348` bytes
- decompressed Image: `26,615,820` bytes
- Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`
- trailing bytes after gzip member: `114,696`

The target is the Infinix X683/H694 MT6768 stock-equivalent 4.14.141-era kernel. Reconstructed code is inferred from binary behaviour, not proprietary-source recovery.

## Critical F2FS layout / ABI

```text
sbi + 0x3d8 = log_blocks_per_seg
sbi + 0x408 = user_block_count
sbi + 0x4b8 = mount_opt.opt
sbi + 0x534 = gc_mode
sbi + 0x568 = f2fs_stat_info *

sm + 0x00 = sit_info
sm + 0x08 = free_info
sm + 0x10 = dirty_info
sm + 0x5c = main_segments
sm + 0x60 = reserved_segments
```

Stock entry ABI:

```c
int f2fs_gc(struct f2fs_sb_info *sbi,
            bool sync,
            bool background,
            unsigned int segno);
```

Transsion passes `NULL_SEGNO` (`-1`).

## Transsion controller

```text
+0x990  cycle/invocation counter
+0x998  controller: 0 normal / 1 GREEDY / 2 URGENT
+0x9c0  controller-write guard
+0x9d0  loop/termination state
+0x9d4  detector state
+0x9d8  repeated-detector counter
+0x9e0  detector-cycle counter
+0x9f0  running statistic/maximum
+0x9f4  saved recoverable baseline
+0x9f8  stop result: 1=Stop4, 2=Stop5
+0x9fc  stop condition: 1..3
+0xa00  detector mode gate
+0xa04  cadence selector: 0=50, nonzero=500
+0xa05  loop-active byte
+0xa06  detector-active/continue byte
+0xa08  signed segment baseline
+0xa0c  written/recoverable baseline
```

## Pass #1 — threshold helper

`0x37b580..0x37b8c0` reconstructed in:

- `fs/f2fs/tran_gc_threshold_reconstructed.c`
- `docs/reverse-engineering/gc-threshold-helper-exact-reconstruction.md`

Confirmed:

```c
user_segments = user_block_count >> log_blocks_per_seg;
sit_segments  = sit_blocks >> log_blocks_per_seg;
span          = user_segments - sit_segments;
free_percent  = free_segments * 100 / span;
fragmentation = 100 - free_percent;
```

The policy helper uses:

```text
small scale table @ Image+0x4d4 = {0x800,0xc00,0x1000,0x1000}
only when (user_segments >> 15) == 0
otherwise scale = 0x1800

selector = max(byte @ 0x1a13890, byte @ 0x1a13894)
factor table @ Image+0x4e4 = {100,100,100,80,80,80,60,60}
```

Stop-2:

```c
delta = (s32)(free_segments - reserved_segments);
threshold = (factor * scale * 0x51EB851F) >> 37;
stop2 = delta > threshold;
```

Stop-3:

```c
span = (s64)user_segments - sit_segments;
reference = (s64)(s32)(free_segments - reserved_segments);
p = left_space_table64[selector] * span;
high = smulh(p, 0xA3D70A3D70A3D70B);
scaled = (high + p) >> 6;
scaled += (p < 0);
stop3 = scaled < reference;
```

The post-`0x37b74c` range is generic vendor attribute/control machinery, not additional GC arithmetic.

## Pass #2 — complete thread reconstruction

Primary source:

`fs/f2fs/tran_gc_thread_reconstructed.c`

Integrated elements:

- detector arming/state gate
- state-3 timed wait/recheck
- metric collection from SBI/segment-manager objects
- Stop 1..5 predicates
- controller transitions
- cadence and baseline handling

Directly proven Stop results:

```text
Stop1 -> +0x9fc = 1
Stop2 -> +0x9fc = 2
Stop3 -> +0x9fc = 3
Stop4 -> +0x998 = 2 (unless +0x9c0 blocks), +0x9f8 = 1
Stop5 -> +0x998 = 2, +0x9f8 = 2
```

State 3 uses a real timed wait path:

```text
+0x9d4 = 3
→ timeout source / 500-ms fallback
→ 0xce58c timeout conversion
→ 0x9c688 wait-entry init
→ 0x9c6e8 wait setup
→ task-state/recheck path
→ 0x377570 metric collection
```

`0xcc774`, `+0x974`, and controller-object `+0x20` remain vendor/task abort predicates whose proprietary semantics are not proven.

## Pass #3 — Transsion wrapper

New source:

`fs/f2fs/tran_gc_wrapper_reconstructed.c`

Recovered wrapper behaviour:

```text
controller 0
    → x683_f2fs_gc(..., NULL_SEGNO)

controller 1
    → save old sbi->gc_mode
    → sbi->gc_mode = 2 (GREEDY)
    → f2fs_gc(..., NULL_SEGNO)
    → restore old mode

controller 2
    → save old sbi->gc_mode
    → sbi->gc_mode = 3 (URGENT)
    → f2fs_gc(..., NULL_SEGNO)
    → restore old mode
```

## Pass #4 — historical 4.14 delta

Document:

`docs/reverse-engineering/f2fs-414-vendor-delta.md`

Historical Android/common 4.14 F2FS contains the same four-argument GC ABI and the normal flow:

```text
GC type selection
→ mounted/checkpoint checks
→ BG→FG escalation when needed
→ victim selection
→ segment migration
→ merged-write submission
→ statistics
→ repeat/checkpoint
```

The X683 vendor delta is therefore best modeled as:

```text
stock F2FS GC core
+
Transsion controller
+
Transsion static detector
+
Stops 1..5
+
vendor statistics
+
vendor controls/debug registration
```

The repository's previous `gc_reconstructed.c` had an incorrect `gc_mutex` unlock. It has been corrected: historical callers own `gc_mutex` around the GC call; `x683_f2fs_gc()` no longer unlocks it internally.

Historical source confirms `stat_inc_call_count()` in the standard GC path and the four-argument ABI. cite Android/common 4.14 gc.c / f2fs.h sources searched during this pass.

## Pass #5 — vendor controls

Document:

`docs/reverse-engineering/vendor-control-final-status.md`

Confirmed registrations:

```text
need_switch_ssr
    registration 0x37af88
    descriptor Image+0x173b9d0

tran_urgent_gc
    registration 0x37b068
    descriptor Image+0x173bbb0

detect_charger_type
    registration 0x37b184
    descriptor Image+0x173bf70
```

All three use the common runtime registry at `Image+0x1a13a20` and generic registration `0x274ea0 -> 0x274dac`.

These are **control/data descriptors**, not proven standalone callback function symbols.

`tran_gc_usb_wakelock` exists as a string but its separate registration/use path remains unresolved.

Do not invent direct calls to `need_switch_ssr()`, `tran_urgent_gc()`, or `detect_charger_type()` inside the detector; Stop-4 itself directly writes controller 2.

## stat_info

Document:

`docs/reverse-engineering/x683-stat-info-reconstruction.md`

Directly confirmed members:

```text
stat +0x164  incremented
stat +0x174  incremented
stat +0x184  accumulated
stat +0x178  incremented
stat +0x188  accumulated
stat +0x18c  incremented
stat +0x190  incremented
stat +0x198  accumulated
```

Exact X683 member names are not yet promoted from reconstruction labels. Historical 4.14 `f2fs_stat_info` has analogous call/GC/segment accounting, so role correspondence is plausible but offsets must not be transplanted blindly. cite Android/common 4.14 f2fs.h source.

## Source quality status

`tran_gc_threshold_reconstructed.c` no longer contains placeholder null-address global reads. The two selector bytes are explicit parameters because they are vendor-global storage rather than normal F2FS fields.

`tran_gc_thread_reconstructed.c` is an integrated reconstruction/scaffold, not a byte-for-byte compilable proprietary replacement. The exact kthread wrapper and unresolved vendor-task predicates remain isolated behind explicit callbacks/inputs instead of guessed names.

## Current project state after passes #1–#5

Completed to high confidence:

- threshold/helper reconstruction
- Stop 1..5 state machine
- controller semantics
- Transsion wrapper semantics
- stock-vs-vendor architectural separation
- named vendor-control registration bindings
- vendor stat_info offset map

Still unresolved:

1. exact callback/read/write semantics for the three control descriptors;
2. `tran_gc_usb_wakelock` separate path;
3. exact proprietary semantics of `0xcc774`, `+0x974`, controller-object `+0x20`;
4. exact X683 stat_info member names;
5. final byte-accurate reconstruction of the kthread scheduler/wakeup wrapper.

These are the only remaining high-value gaps in the current GC-controller reconstruction.
