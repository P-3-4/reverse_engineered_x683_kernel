# X683 / H694 Kernel Reverse-Engineering — Project Handoff

## Canonical continuation state

Work from `main` in `P-3-4/reverse_engineered_x683_kernel`.

**User preference:** keep explanations minimal; do the actual reverse-engineering work and report concrete results/remaining blockers. Do not claim something is proven when it is only inferred.

## Binary authority

- Target: Infinix X683/H694, MT6768 ARM64, stock Android 10, Linux 4.14.141-era kernel.
- `boot.img` SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- `boot.img`: 33,554,432 bytes.
- Kernel slot: `0x94dad4`, 9,755,348 bytes.
- Decompressed Image: 26,615,820 bytes.
- Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`
- Gzip trailing bytes: 114,696.
- Stock binary is authoritative; public 4.14/Transsion sources are references only.
- Goal: reconstruct functionally equivalent stock behavior, especially the Transsion F2FS GC subsystem, then integrate/build a kernel.

## Critical F2FS layout / ABI

```text
sbi + 0x3d8 = log_blocks_per_seg
sbi + 0x408 = user_block_count
sbi + 0x4b8 = mount_opt.opt
sbi + 0x508 = gc_mutex
sbi + 0x528 = gc_thread
sbi + 0x530 = cur_victim_sec
sbi + 0x534 = gc_mode
sbi + 0x538 = next_victim_seg
sbi + 0x568 = f2fs_stat_info *

sm + 0x00 = sit_info
sm + 0x08 = free_info
sm + 0x10 = dirty_info
sm + 0x5c = main_segments
sm + 0x60 = reserved_segments
```

Stock ABI:

```c
int f2fs_gc(struct f2fs_sb_info *sbi, bool sync,
            bool background, unsigned int segno);
```

Transsion uses `NULL_SEGNO`.

## Transsion controller/state offsets

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
+0xa0c  baseline recoverable/free-progress value
```

## Reconstructed threshold/helper

Source: `fs/f2fs/tran_gc_threshold_reconstructed.c`.

Confirmed:

```c
user_segments = user_block_count >> log_blocks_per_seg;
sit_segments  = sit_blocks >> log_blocks_per_seg;
span          = user_segments - sit_segments;
free_percent  = free_segments * 100 / span;
fragmentation = 100 - free_percent;
```

Policy constants:

```text
small scale table @ Image+0x4d4 = {0x800,0xc00,0x1000,0x1000}
small-table condition: (user_segments >> 15) == 0
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

`0x37b74c+` is generic vendor attribute/control machinery, not additional GC threshold arithmetic.

## `tran_gc_thread_func` reconstruction

Primary source: `fs/f2fs/tran_gc_thread_reconstructed.c`.

Integrated: detector arming/state gate, state-3 timed wait/recheck, metric collection, Stop 1..5, controller transitions, cadence/baseline handling.

Metric producer correction:

```text
arming_dirty_segments = sum(dirty_info +0x68 .. +0x7c)
recoverable_segments  = free_info->free_segments + dirty_info->nr_dirty[PRE]
```

Stop operands:

```text
Stop1 delta = recoverable_segments - +0x9f4
Stop2 delta = recoverable_segments - sm_info->reserved_segments
Stop3 reference = recoverable_segments - reserved_segments
Stop4 delta = (running_max - recoverable_segments) + saved_sit_segments - sit_segments
Stop5 progress = sit_segments + (recoverable_segments - +0xa0c)
```

Direct Stop writes:

```text
Stop1 -> +0x9fc = 1
Stop2 -> +0x9fc = 2
Stop3 -> +0x9fc = 3
Stop4 -> +0x998 = 2 unless +0x9c0 blocks; +0x9f8 = 1
Stop5 -> +0x998 = 2; +0x9f8 = 2
```

## State-3 wait/recheck resolution

The stock state-3 sequence is now resolved at the helper level:

```text
+0x9d4 = 3
+0xd94 -> 0xce58c -> positive-ms-to-jiffies conversion
0x57554 -> TIF_NEED_RESCHED bit test
0x9c688 -> wait entry initialization
0x9c6e8 -> prepare-to-wait / TASK_INTERRUPTIBLE insertion
0x57554 -> scheduler recheck
0xcc774 -> unresolved vendor/task abort predicate
0x9c8d0 -> finish_wait
0x57554 -> final scheduler recheck
+0x974 -> producer-driven wake/recheck gate
metric collection
```

`+0x974` is no longer considered unresolved as a storage role. Its producer is the callback-like function at `0x37acf8`:

```text
event != 9 -> return

event == 9:
  state == 0 -> +0x974 = 1
  state == 4 -> +0x974 = 0
  other states -> unchanged
```

The callback reads `*(event_data + 0x8)`. When auxiliary state `+0x898` exists, it also signals the object at `+0x978` with `(3,1,0)` in both state-changing branches.

The callback's public/vendor event name and registration binding remain unresolved.

The exact helper `0xcc774` remains unresolved by semantic name. It examines task state and, on one branch, calls `0x1051a8`, which tests bits `0x6` in a field reached through `task + 0x950`.

The controller-object `+0x20` predicate remains unresolved.

`0x1eca60` remains unresolved by semantic name; its body is a generic per-object reference/locking operation.

## Transsion GC wrapper

`fs/f2fs/tran_gc_wrapper_reconstructed.c`.

Recovered behavior:

```text
controller 0 -> normal f2fs_gc(..., NULL_SEGNO)
controller 1 -> save gc_mode; set 2; f2fs_gc(..., NULL_SEGNO); restore
controller 2 -> save gc_mode; set 3; f2fs_gc(..., NULL_SEGNO); restore
```

Do not conflate controller values with the underlying `gc_mode` values: controller 1 maps to `gc_mode=2`, controller 2 maps to `gc_mode=3`.

## Stock F2FS GC reconstruction status

`fs/f2fs/gc_reconstructed.c` is an inferred reconstruction, **not recovered proprietary source**. It is currently a scaffold and must not be presented as compile-ready or byte-equivalent.

Important sanity correction already applied: historical 4.14 callers own `gc_mutex` around GC; the reconstructed GC function must not unlock it internally.

Historical Android/common 4.14 sources confirm the four-argument `f2fs_gc()` ABI and normal GC flow: checks, BG→FG escalation, victim selection, migration, merged writes, statistics, repeat/checkpoint.

## Vendor controls

Known registered descriptors:

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

They use common registry `Image+0x1a13a20` and registration path `0x274ea0 -> 0x274dac`.

These are **control/data descriptors**, not proven standalone callback symbols. Do not insert direct calls to those names into the detector without further pointer/data-flow proof.

`tran_gc_usb_wakelock` exists as a string but its separate registration/use path is unresolved.

## stat_info

`sbi + 0x568` points to stat info. Direct updates observed:

```text
+0x164 increment
+0x174 increment
+0x184 accumulated
+0x178 increment
+0x188 accumulated
+0x18c increment
+0x190 increment
+0x198 accumulated
```

Exact X683 member names remain unresolved. Do not blindly transplant historical 4.14 names onto these offsets.

## Current honest project status

Binary-level GC architecture: ~80–85% confidence.

Source reconstruction: materially lower; not yet a buildable replacement.

High-confidence completed:
- boot/Image verification
- critical F2FS layout/ABI mapping
- controller semantics
- Stop 1–5 direct state writes and operand chains
- recovered threshold arithmetic
- detector arming predicates
- state-3 wait helper identification
- `+0x974` producer and consumer role
- Transsion wrapper behavior
- stock-vs-vendor architectural separation
- vendor-control registration bindings
- stat_info offset map

Remaining high-value gaps:
1. public/vendor identity and registration path of the `0x37acf8` event callback;
2. exact semantic name/body integration for `0xcc774`;
3. exact meaning of controller-object `+0x20`;
4. exact semantic identity of `0x1eca60`;
5. `tran_gc_usb_wakelock` path;
6. exact stat_info member names;
7. final byte-accurate kthread scheduler/wakeup wrapper;
8. full compilation/integration against the correct X683/H694 4.14.141 source base.

## Known sanity-audit commits

- `96b4d5916890541b0d39ca31946ee07bfe71869e` — corrected thread scaffold / removed invalid timeout approximation.
- `c96a942af39a3fc856106475f2a0afa5d8bd9c5c` — audit documentation.
- `e7da9fa42f29a95a649509b45889a0712da97efd` — source-quality corrections.
- `53c9df976fd9ef7f1d7a7c614657c159a90393bc` — resolved `+0x974` producer semantics in thread scaffold.
- `3fd2a50bf6c08b64395c4c7eb1618becbecad033` — documented exact GC wait-flag producer.

Use this file as the canonical continuation point in future chats. Read it from `main` before continuing.