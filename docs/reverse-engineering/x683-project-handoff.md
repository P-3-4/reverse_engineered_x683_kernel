# X683 / H694 Kernel Reverse-Engineering — Project Handoff

## Canonical continuation state

Work from `main` in `P-3-4/reverse_engineered_x683_kernel`.

**User preference:** keep explanations minimal; do the actual reverse-engineering work and report concrete results/remaining blockers. Do not claim something is proven when it is only inferred.

## Binary authority

- Target: Infinix X683/H694, MT6768 ARM64, stock Android 10, Linux 4.14.141-era kernel.
- `boot.img` SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- `boot.img`: 33,554,432 bytes.
- Kernel compressed size: `0x94dad4` bytes at boot image offset `0x800`.
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

## `tran_gc_thread_func` reconstruction

Primary source: `fs/f2fs/tran_gc_thread_reconstructed.c`.

Integrated:
- detector arming/state gate;
- metric collection;
- Stop 1..5 operand chains and writes;
- state-3 timed-wait structure;
- freezer-aware task predicate;
- `+0x974` event producer/consumer role.

Metric producers:

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

## Final state-3 freezer resolution

The region `0x377494..0x377570` is now semantically resolved at the kernel/freezer level.

```text
+0x9d4 = 3
+0xd94 -> 0xce58c -> timeout conversion
0x57554 -> scheduler/NEED_RESCHED check
0x9c688 -> wait-entry initialization
0x9c6e8 -> prepare-to-wait / TASK_INTERRUPTIBLE
0x57554 -> scheduler recheck
0xcc774 -> freezer-aware task eligibility predicate
0x9c8d0 -> finish_wait
0x57554 -> final scheduler recheck
Image+0x19f0020 -> system_freezing_cnt
Image+0x19f0024 -> pm_freezing
Image+0x19f0028 -> pm_nosig_freezing
+0x974 -> vendor event-driven wake/re-entry gate
metric collection
```

### `0x1051a8`

Resolved to `cgroup_freezing(struct task_struct *)`.

Its binary shape is a protected task->cgroup->freezer-state lookup followed by `state & 0x6`, matching the Android 4.14 cgroup freezer predicate.

### `0xcc774`

Resolved as a vendor-modified inverse/eligibility form of `freezing_slow_path()`:

```c
if (task->flags & (PF_NOFREEZE | PF_SUSPEND_TASK))
    return false;
if (task->flags & PF_KSWAPD)
    return false; /* X683-specific deviation */
if (pm_nosig_freezing)
    return false;
if (cgroup_freezing(task))
    return false;
if (!pm_freezing)
    return false;
if (task->flags & PF_KTHREAD)
    return false;
return true;
```

The matching Android 4.14 freezer implementation uses `pm_nosig_freezing || cgroup_freezing(p)` and `pm_freezing && !(p->flags & PF_KTHREAD)` in `freezing_slow_path()`. The X683 binary additionally rejects `PF_KSWAPD` instead of using the standard `TIF_MEMDIE` test.

### `+0x20` correction

The previous label `controller-object +0x20` was incorrect.

`x22` in the detector points to `Image+0x19f0000`, so:

```text
[x22 + 0x20] = system_freezing_cnt
[x22 + 0x24] = pm_freezing
[x22 + 0x28] = pm_nosig_freezing
```

Therefore the detector's `+0x20` check is a direct system-freezer-state gate.

### `+0x974`

Producer at `0x37acf8`:

```text
event != 9 -> return
event == 9:
  state == 0 -> +0x974 = 1
  state == 4 -> +0x974 = 0
  other states -> unchanged
```

When auxiliary state `+0x898` exists, the callback also signals the object at `+0x978` with `(3,1,0)` on the state-changing branches.

Public/vendor event identity remains unresolved.

## Transsion GC wrapper

`fs/f2fs/tran_gc_wrapper_reconstructed.c`.

Recovered behavior:

```text
controller 0 -> normal f2fs_gc(..., NULL_SEGNO)
controller 1 -> save gc_mode; set 2; f2fs_gc(..., NULL_SEGNO); restore
controller 2 -> save gc_mode; set 3; f2fs_gc(..., NULL_SEGNO); restore
```

Do not conflate controller values with underlying `gc_mode` values.

## Stock F2FS GC reconstruction status

`fs/f2fs/gc_reconstructed.c` remains an inferred reconstruction, not recovered proprietary source. It is a scaffold and must not be presented as compile-ready or byte-equivalent.

Historical Android/common 4.14 sources confirm the four-argument `f2fs_gc()` ABI and caller-owned GC mutex model.

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

These are control/data descriptors, not proven standalone callback functions.

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

Exact X683 member names remain unresolved.

## Current honest project status

Binary-level GC architecture: ~85–90% confidence.

Source reconstruction: substantial but not final.

Buildability: not established.

Replacement-kernel readiness: not established.

High-confidence completed:
- boot/Image verification;
- critical F2FS layout/ABI mapping;
- controller semantics;
- Stop 1–5 state writes and operand chains;
- recovered threshold arithmetic;
- detector arming predicates;
- state-3 wait helper identification;
- `cgroup_freezing()` identification at `0x1051a8`;
- `pm_freezing`, `pm_nosig_freezing`, `system_freezing_cnt` identification;
- vendor-modified `freezing_slow_path()`-equivalent predicate at `0xcc774`;
- `+0x974` producer and consumer role;
- Transsion wrapper behavior;
- vendor-control registration bindings;
- stat_info offset map.

Remaining high-value gaps:
1. public/vendor identity and registration path of the `0x37acf8` event callback;
2. exact state-3 control-flow integration around the resolved freezer predicates;
3. exact semantic identity of `0x1eca60`;
4. `tran_gc_usb_wakelock` path;
5. exact stat_info member names;
6. full exact stock-X683 `gc.c` differential;
7. compilation/integration against the correct X683/H694 4.14.141 source tree.

## Recent commits

- `799cf3d7131ab0e24237a62e17ad7e5ecd42a117` — final state-3 freezer resolution.
- `f121afada67d0c8bfacd772277db5718a4adc17b` — semantic freezer predicate reconstruction.
- `89f35c65d64c6b5243639c497aa7f0ac20b4aa1a` — state-3 exact predicate evidence.
- `ccdb49883812563f12bd03c296326e9f97edfcd5` — `cc774` final-resolution notes.

Use this file as the canonical continuation point in future chats. Read it from `main` before continuing.
