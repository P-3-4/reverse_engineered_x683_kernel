# X683 / H694 Kernel Reverse-Engineering — Project Handoff

Work from `main` in `P-3-4/reverse_engineered_x683_kernel`.

## Binary authority

- Target: Infinix X683/H694, MT6768 ARM64, stock Android 10, Linux 4.14.141-era kernel.
- `boot.img` SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- `boot.img`: 33,554,432 bytes.
- Kernel: gzip at boot `0x800`, compressed size `9,755,348` bytes; decompressed Image size `26,615,820`.
- Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`.
- Gzip trailing bytes: `114,696`.
- Stock binary is authoritative. Public 4.14/Android sources are references.

## Critical F2FS layout / ABI

```text
sbi +0x3d8  log_blocks_per_seg
sbi +0x3dc  blocks_per_seg
sbi +0x3e0  segs_per_sec
sbi +0x408  user_block_count
sbi +0x4b8  mount_opt.opt
sbi +0x508  gc_mutex
sbi +0x528  gc_thread
sbi +0x530  cur_victim_sec
sbi +0x534  gc_mode
sbi +0x538  next_victim_seg
sbi +0x568  f2fs_stat_info *

sm +0x00  sit_info
sm +0x08  free_info
sm +0x10  dirty_info
sm +0x5c  main_segments
sm +0x60  reserved_segments
```

Stock ABI:

```c
int f2fs_gc(struct f2fs_sb_info *sbi, bool sync,
            bool background, unsigned int segno);
```

## Exact Transsion controller wrapper

Primary binary range: `0x37ada8..0x37ae94`.

```text
+0x1a13990 = invocation counter
+0x1a13998 = controller
```

Controller mapping:

```text
controller 0 -> f2fs_gc(sbi, FORCE_FG_GC, true, NULL_SEGNO)
controller 1 -> save gc_mode; temporary gc_mode=2 (URGENT); call; restore
controller 2 -> save gc_mode; temporary gc_mode=3 (GREEDY); call; restore
```

`FORCE_FG_GC` is mount option bit 14.

The direct lower call target is **`0x3503a8`**.

Source: `fs/f2fs/tran_gc_wrapper_reconstructed.c`.

## Actual X683 `f2fs_gc()` execution path

**`0x3503a8` is the actual four-argument X683 F2FS GC entry.**

Recovered flow:

```text
0x3503a8 f2fs_gc(sbi, sync, background, segno)
    |
    +-- trace/entry bookkeeping
    +-- gc_type = sync ? FG_GC : BG_GC
    +-- active/checkpoint-error checks
    +-- BG free-space / prefree checkpoint handling
    +-- BG -> FG escalation when free sections remain low
    +-- reject BG GC when background == false
    |
    +-- victim selection
    |     |
    |     +-- build policy context
    |     +-- DIRTY_I(sbi)->v_ops->get_victim()
    |           args: sbi, &segno, gc_type, NO_CHECK_TYPE, LFS
    |
    +-- victim section migration
    |     |
    |     +-- SSA/summary preparation
    |     +-- iterate segs_per_sec segments
    |     +-- NODE/DATA migration dispatch
    |     +-- valid-block / section-free accounting
    |     +-- merged-write submission for FG GC
    |
    +-- stats / cur_victim_sec
    +-- async repeat with segno=NULL_SEGNO when free-space remains low
    +-- checkpoint after promoted/foreground async GC
    |
    +-- SIT_I(sbi)->last_victim[ALLOC_NEXT] = 0
    +-- SIT_I(sbi)->last_victim[FLUSH_DEVICE] = init_segno
    +-- mutex_unlock(&sbi->gc_mutex)
    +-- put_gc_inode()
    +-- return
```

Direct binary proof of `mutex_unlock(&sbi->gc_mutex)` is the call from `0x3503a8` epilogue at `0x352ba8` with `x0 = sbi + 0x508`.

Direct binary proof of SIT cursor cleanup is the stores around `0x352a8c..0x352a9c` through `sit_info`.

The four-argument API/cursor changes are also confirmed by the 2017 F2FS API change. citeturn143882view0

## Victim selection

Inside `0x3503a8`, around `0x350808..0x350850`, the binary performs an indirect call through the dirty-manager vector:

```c
DIRTY_I(sbi)->v_ops->get_victim(sbi, &segno,
        gc_type, NO_CHECK_TYPE, LFS);
```

This is the actual victim-selection boundary.

The target-era policy family remains:

```text
BG_GC -> cost-benefit by default
FG_GC -> greedy by default
```

with `gc_mode` altering the selected policy.

## Actual migration engine

The section-migration logic is inside `0x3503a8`, not `0x362c40`.

The binary contains the same core structure as the 4.14/4.15 F2FS dispatcher:

```text
victim section
  -> summary/SSA pages
  -> per-segment valid-block checks
  -> NODE/DATA dispatch
  -> live-block migration
  -> segment statistics
  -> FG complete-section detection
  -> merged NODE/DATA writes
```

Historical 4.15 `do_garbage_collect()` confirms the same `segs_per_sec` iteration, summary-page handling, NODE/DATA dispatch, foreground freed-segment count, merged writes and `sec_freed` rule. citeturn497268view0

Historical 4.15 `f2fs_gc()` confirms the BG/FG checks, repeat/checkpoint flow, SIT cursor cleanup and mutex release. citeturn497268view0turn143882view0

## Separate vendor policy routine: `0x366cd4`

`0x366cd4` is **not** `f2fs_gc()` and is not the controller wrapper.

It is the separate Transsion F2FS/GC policy/orchestration routine.

Direct branch map:

```text
0x366d00 -> 0x35cc18(selector 4)
0x366d10 -> 0x373108(selector 0x80)
0x366d1c -> 0x35cc18(selector 1)
0x366d2c -> 0x35d22c(selector 0x1c7)
0x366d38 -> 0x35cc18(selector 0)
```

If selector-0 returns true:

```text
0x366d4c -> 0x362c40(sbi,0,0)
```

otherwise:

```text
0x366d5c -> 0x363288(sbi,0xe38)
```

Then `0x366cd4` checks `gc_mode`, `sbi +0x444..0x45c`, free/reservation state, fixed-point thresholds, and time/counter guards. It has a post-GC path through `0x3e1014`, `0x34e224`, `0x3e1558`, and `0x341250`, and increments `stat_info +0x16c`.

### Helper identities

`0x35cc18`:

- multi-mode vendor GC policy predicate;
- selector 0..5 dispatches different threshold arithmetic;
- returns boolean.

`0x373108`:

- vendor policy gate keyed from `sbi +0x4b9` bit 5;
- additional segment/threshold accounting;
- returns boolean.

`0x362c40`:

- internal vendor segment-management helper;
- scans manager bitmaps/arrays and invokes lower helper `0x3655d8`;
- **not** the `f2fs_gc()` entry.

`0x363288`:

- internal vendor linked-list drain/cleanup helper;
- **not** `put_gc_inode()` without further proof.

`0x341250`:

- separate filesystem write/GC-balancing helper.

## Detector / state-3

Controller/detector state offsets:

```text
+0x990 cycle
+0x998 controller
+0x9c0 controller-write guard
+0x9d0 loop state
+0x9d4 detector state
+0x9d8 repeated counter
+0x9e0 detector cycles
+0x9f0 running max
+0x9f4 saved recoverable baseline
+0x9f8 stop result
+0x9fc stop condition
+0xa00 detector mode
+0xa04 cadence selector
+0xa05 loop active
+0xa06 detector active/continue
+0xa08 signed baseline segment
+0xa0c baseline recoverable/free-progress
```

Metrics:

```text
arming_dirty_segments = sum(dirty_info +0x68..+0x7c)
recoverable_segments  = free_info->free_segments + dirty_info->nr_dirty[PRE]
```

Stop operands:

```text
Stop1 = recoverable - +0x9f4
Stop2 = recoverable - reserved_segments
Stop3 = scaled(user_segments - sit_segments) < (recoverable - reserved_segments)
Stop4 = (running_max - recoverable) + saved_sit_segments - sit_segments
Stop5 = sit_segments + (recoverable - +0xa0c) <= +0xa08
```

Direct writes:

```text
Stop1 -> +0x9fc=1
Stop2 -> +0x9fc=2
Stop3 -> +0x9fc=3
Stop4 -> +0x998=2 unless +0x9c0 blocks; +0x9f8=1
Stop5 -> +0x998=2; +0x9f8=2
```

State-3 freezer resolution:

```text
0x1051a8 = cgroup_freezing(task)
+0x20 at Image+0x19f0000 = system_freezing_cnt
+0x24 = pm_freezing
+0x28 = pm_nosig_freezing
0xcc774 = vendor-modified freezing_slow_path eligibility predicate
+0x974 = event 9 state 0/4 wake-reentry flag
```

`0xcc774` additionally rejects `PF_KSWAPD` in X683.

## Threshold arithmetic

Scale table:

```text
{0x800, 0xc00, 0x1000, 0x1000}; otherwise 0x1800
```

Factor table:

```text
{100,100,100,80,80,80,60,60}
```

Selector: `max(Image+0x1a13890, Image+0x1a13894)`.

Stop-2 fixed-point multiplier: `0x51EB851F >> 37`.

Stop-3 signed fixed-point multiplier: `0xA3D70A3D70A3D70B` followed by the recovered `smulh`/shift sequence.

## Current source files

```text
fs/f2fs/tran_gc_thread_reconstructed.c
fs/f2fs/tran_gc_threshold_reconstructed.c
fs/f2fs/tran_gc_wrapper_reconstructed.c
fs/f2fs/gc_reconstructed.c
```

`gc_reconstructed.c` is still an inferred scaffold, not proprietary-source recovery or build-proven source, but its major X683 control-flow corrections now include:

- four-argument ABI;
- victim selection boundary;
- section migration structure;
- SIT cursor cleanup;
- `mutex_unlock(&sbi->gc_mutex)` at the proven epilogue;
- synchronous `0/-EAGAIN` outcome structure.

## Vendor controls

```text
need_switch_ssr        registration 0x37af88, descriptor Image+0x173b9d0
tran_urgent_gc         registration 0x37b068, descriptor Image+0x173bbb0
detect_charger_type    registration 0x37b184, descriptor Image+0x173bf70
```

These are control/data descriptors, not proven standalone callbacks.

`tran_gc_usb_wakelock` string exists; separate registration/use path unresolved.

## Honest project status

- Binary-level GC architecture: **~92% confidence**.
- Vendor detector/controller: **high confidence**.
- `tran_f2fs_gc()` wrapper: **essentially exact at binary level**.
- Actual X683 `f2fs_gc()` entry/execution path: **high confidence at architectural/control-flow level**.
- Vendor `0x366cd4` policy body: **substantial, but helper names/edge branches remain**.
- Source reconstruction: **not build-proven**.
- Replacement-kernel readiness: **not established**.

Remaining high-value gaps:

1. finish exact source-level differential for `0x366cd4` helper semantics;
2. identify the remaining X683 `gc_node_segment()` / `gc_data_segment()` deviations from reference 4.14;
3. resolve `tran_gc_usb_wakelock` path;
4. resolve exact `stat_info` member names;
5. integrate/compile against the correct X683/H694 4.14.141 source tree.

## Recent authoritative commits

- `98c2eceb7549339eb997013d86591eb1ce953d6f` — corrected final GC entry boundary and execution-path document.
- `4fb4860f6e4ca34c66a96db6f73d1807c4861b6c` — corrected X683 `f2fs_gc()` mutex release.
- `f99d88723683ded23aeff891aeebc2000a19d3f5` — tightened four-argument GC reconstruction.
- `a9e9d17ca3e20f07bab01670bbb5a5a4a3bbacd7` — initial full execution-path reconstruction.
- `718250acd83fe4fd033f75059720ee0afd9a4dc0` — exact `tran_f2fs_gc()` wrapper correction.

Use this file as the canonical continuation point in future chats. Read it from `main` before continuing.
