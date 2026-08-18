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
+0x998  controller: 0 normal / 1 URGENT / 2 GREEDY
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
factor table @ 0x4e4 = {100,100,100,80,80,80,60,60}
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
- `+0x974` event producer/consumer role;
- superblock freeze-protection gate around the vendor GC policy path.

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

The region `0x377494..0x377570` is semantically resolved at the kernel/freezer level.

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

### Freezer-object correction

The detector's `x22 + 0x20` was previously treated as a vendor controller field. That was wrong.

```text
x22 -> Image+0x19f0000
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

## Superblock/per-CPU synchronization helpers

### `0x1eca60`

Correction: this address is **not** `__sb_start_write()`.

The current binary mapping places `0x1eca60` in the per-CPU reader synchronization family (`percpu_rwsem` / `percpu_down_read` lineage). It is called from the detector with `(level=1, wait=0)`-shaped arguments, but the exact vendor/build-specific wrapper identity should not be renamed beyond that without preserving the machine-code caveat.

### `0x1ec9e4`

This is the corresponding per-CPU reader-release path used after the detector's synchronized section.

### `0x341250`

This separate X683 helper contains the filesystem write/GC-balancing path and must not be conflated with `0x1eca60`. Its exact upstream symbolic name remains under comparison against the matching 4.14 source.

## Exact `tran_f2fs_gc()` wrapper

Primary source: stock `0x37ada8..0x37ae94`.

This is the actual Transsion controller wrapper:

```c
controller = *(u32 *)(Image + 0x1a13998);
force_fg_gc = (sbi->mount_opt.opt >> 14) & 1;

switch (controller) {
case 0:
    return f2fs_gc(sbi, force_fg_gc, true, NULL_SEGNO);
case 1:
    old = sbi->gc_mode;
    sbi->gc_mode = 2;
    ret = f2fs_gc(sbi, force_fg_gc, true, NULL_SEGNO);
    sbi->gc_mode = old;
    return ret;
case 2:
    old = sbi->gc_mode;
    sbi->gc_mode = 3;
    ret = f2fs_gc(sbi, force_fg_gc, true, NULL_SEGNO);
    sbi->gc_mode = old;
    return ret;
default:
    /* stock path does not use another controller value */
}
```

Exact wrapper facts:

```text
+0x1a13990 = invocation counter; incremented on entry
+0x1a13998 = controller
sbi + 0x4b8 = mount_opt.opt
sync       = mount_opt bit 14 = F2FS_MOUNT_FORCE_FG_GC
background = true
segno      = NULL_SEGNO
```

Nearby stock strings identify vendor GC modes:

```text
"gc mode is COST"
"gc mode is URGENT"
"gc mode is GREEDY"
```

The vendor `gc_mode` values are therefore:

```text
1 = COST
2 = URGENT
3 = GREEDY
```

Controller mapping:

```text
controller 0 -> preserve current gc_mode
controller 1 -> temporary gc_mode 2 (URGENT)
controller 2 -> temporary gc_mode 3 (GREEDY)
```

Do not confuse these controller values with the underlying vendor `gc_mode` values.

`fs/f2fs/tran_gc_wrapper_reconstructed.c` was corrected to preserve these exact numeric semantics.

## Separate F2FS/GC policy routine

`0x366cd4` is a **separate** vendor F2FS/GC policy routine. It invokes lower GC/helper paths and must not be merged with the controller wrapper at `0x37ada8`.

Current mapped calls include:

```text
0x35cc18 -> GC policy predicate/helper family
0x362c40 -> lower F2FS GC execution/migration path
0x363288 -> cleanup/slow path
0x341250 -> filesystem write/GC-balancing helper
```

Exact source-level naming of that larger policy routine remains the next differential task.

## Stock F2FS GC reconstruction status

`fs/f2fs/gc_reconstructed.c` remains an inferred reconstruction, not recovered proprietary source. It is a scaffold and must not be presented as compile-ready or byte-equivalent.

Historical Android/common 4.14 sources confirm the four-argument `f2fs_gc()` ABI and caller-owned GC mutex model. citeturn946769search8turn296107search0

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

Binary-level GC architecture: ~90% confidence.

Source reconstruction: substantial, with the controller wrapper now essentially exact but the larger vendor policy body still under reconstruction.

Buildability: not established.

Replacement-kernel readiness: not established.

High-confidence completed:
- boot/Image verification;
- critical F2FS layout/ABI mapping;
- detector/controller semantics;
- Stop 1–5 state writes and operand chains;
- threshold arithmetic;
- detector arming predicates;
- state-3 wait structure;
- freezer subsystem identification;
- vendor-modified freezer predicate;
- `+0x974` producer/consumer role;
- exact `tran_f2fs_gc()` controller wrapper at `0x37ada8`;
- separation of wrapper and `0x366cd4` policy routine;
- vendor-control registration bindings.

Remaining high-value gaps:
1. complete `0x366cd4` vendor F2FS policy-body reconstruction;
2. exact semantic names of its helper calls and return values;
3. `tran_gc_usb_wakelock` path;
4. exact stat_info member names;
5. full exact stock-X683 `gc.c` differential;
6. compilation/integration against the correct X683/H694 4.14.141 source tree.

## Recent commits

- `718250acd83fe4fd033f75059720ee0afd9a4dc0` — corrected `tran_f2fs_gc()` source to exact controller/mode/ABI semantics.
- `38fd69a4d61081c63e3cf6394f7b7cdba428ca5f` — exact `tran_f2fs_gc()` wrapper evidence.
- `f9f447f02964165bdd442a2f0df46b779f9db7e7` — GC helper-cluster deep pass.
- `85b8a278e9c20ffe8cf13f4f39e2229eaea24bee` — corrected `0x1eca60` identity.
- `01b0343e0cb0de555dfd3ca6c2379d153b0219c5` — earlier synchronization mapping note.

Use this file as the canonical continuation point in future chats. Read it from `main` before continuing.
