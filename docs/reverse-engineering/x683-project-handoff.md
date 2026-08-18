# X683 / H694 Kernel Reverse-Engineering — Canonical Project Handoff

Last consolidated: 2026-08-18

## 0. Continuation rule

Work from `main` in `P-3-4/reverse_engineered_x683_kernel`.

Read this file first before continuing.

**User preference:** do the work, keep explanations minimal, do not ask unnecessary confirmation questions, and do not present inference as proof.

**Binary authority:** stock X683/H694 `boot.img` and its decompressed kernel Image. Public Android/Linux/F2FS sources are comparison references only.

---

# 1. Target and binary identity

Device:
- Infinix X683 / H694
- MediaTek MT6768
- ARM64
- Android 10 stock generation
- Linux 4.14.141-era kernel

Stock boot image:
- size: `33,554,432` bytes
- SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- Android boot magic: `ANDROID!`
- page size: `0x800`
- kernel compressed offset: `0x800`
- kernel compressed size: `0x94dad4` = `9,755,348` bytes
- ramdisk size: `0x0e6528` = `943,464` bytes

Compressed kernel:
- SHA-256: `6701980890b0b18d34e88369ef50d624e3f3bee0b5a481d833141b2d256e20bd`
- gzip member ends before `114,696` trailing bytes; extraction must stop at gzip EOF

Decompressed Image:
- size: `26,615,820` bytes
- SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

Important offsets below are **decompressed Image offsets**, not boot.img offsets.

---

# 2. Primary objective

Original goal:

1. Determine the X683/H694 kernel/F2FS GC behavior.
2. Reverse-engineer the Transsion GC detector/controller and vendor policy layer.
3. Reconstruct the stock F2FS GC execution path at the correct 4.14-era ABI.
4. Reconstruct functionally equivalent source where evidence permits.
5. Compare vendor behavior against historical Android/Linux 4.14 F2FS.
6. Integrate into a real X683/H694 4.14.141 source tree and eventually establish buildability.

Current state: the binary-level GC architecture is largely recovered; source is not yet build-proven.

---

# 3. High-confidence F2FS `f2fs_sb_info` layout

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
```

Other observed SBI fields relevant to vendor policy:

```text
sbi +0x444..+0x45c  seven individually tested vendor/filesystem guard words
sbi +0x4b9           vendor mount-option byte; bit 5 used by 0x373108; bit 7 used by terminal helper path
```

Do not give the `0x444..0x45c` fields symbolic names until matched to the real X683 source tree.

Stock X683 F2FS GC ABI is:

```c
int f2fs_gc(struct f2fs_sb_info *sbi,
            bool sync,
            bool background,
            unsigned int segno);
```

Transsion wrapper always supplies:

```text
segno = NULL_SEGNO = -1
background = true
sync = mount_opt bit 14 / FORCE_FG_GC
```

---

# 4. Segment-manager topology

High-confidence `sm_info` relationships:

```text
sm +0x00  sit_info
sm +0x08  free_info
sm +0x10  dirty_info
sm +0x18  curseg_array
sm +0x5c  main_segments
sm +0x60  reserved_segments
```

Important correction:

Older notes incorrectly treated offsets such as `dirty_info +0x1ac/+0x21c/+0x28c` as dirty_info members. That was wrong.

The stock binary accesses those through:

```text
sm +0x18 -> curseg_array
```

The X683 `curseg_info` stride is `0x70` bytes.

The repeated member offset is `+0x5c`:

```text
0x1ac = 3 * 0x70 + 0x5c
0x21c = 4 * 0x70 + 0x5c
0x28c = 5 * 0x70 + 0x5c
```

The six array entries follow the standard six current-log ordering:

```text
0 = CURSEG_HOT_DATA
1 = CURSEG_WARM_DATA
2 = CURSEG_COLD_DATA
3 = CURSEG_HOT_NODE
4 = CURSEG_WARM_NODE
5 = CURSEG_COLD_NODE
```

The binary uses `curseg_array[i] +0x5c` as the current-segment-related value; the strongest mapping is segment number, but keep the exact vendor member name conservative until its writers/readers are matched.

Separate dirty-info candidate region:

```text
dirty_info +0x68..+0x7c
```

was used in earlier detector work for six counters, but retain the project note that this definition should be independently validated against stock writers before treating it as a final source-level `nr_dirty[]` mapping.

---

# 5. Transsion detector/controller object

Recovered relative fields:

```text
+0x990  cycle/invocation counter
+0x998  controller state
+0x9c0  controller-write guard
+0x9d0  loop/termination state
+0x9d4  detector state
+0x9d8  repeated-detector counter
+0x9e0  detector-cycle counter
+0x9f0  running statistic / maximum
+0x9f4  saved recoverable baseline
+0x9f8  stop-result code
+0x9fc  stop-condition code
+0xa00  detector mode gate
+0xa04  cadence selector
+0xa05  loop-active byte
+0xa06  detector active/continue byte
+0xa08  signed baseline segment value
+0xa0c  baseline written-segment/progress value
```

Static detector has five vendor stop predicates.

Direct stop writes:

```text
Stop 1 -> +0x9fc = 1
Stop 2 -> +0x9fc = 2
Stop 3 -> +0x9fc = 3
Stop 4 -> +0x998 = 2 unless +0x9c0 blocks; +0x9f8 = 1
Stop 5 -> +0x998 = 2; +0x9f8 = 2
```

---

# 6. Threshold / fragmentation arithmetic

Recovered source candidate:

`fs/f2fs/tran_gc_threshold_reconstructed.c`

Confirmed arithmetic:

```c
user_segments = user_block_count >> log_blocks_per_seg;
sit_segments  = sit_blocks >> log_blocks_per_seg;
span          = user_segments - sit_segments;
free_percent  = free_segments * 100 / span;
fragmentation = 100 - free_percent;
```

Policy constants:

```text
small-device scale table = {0x800,0xc00,0x1000,0x1000}
small-table condition    = (user_segments >> 15) == 0
otherwise scale          = 0x1800
selector                 = max(byte @ 0x1a13890, byte @ 0x1a13894)
factor table             = {100,100,100,80,80,80,60,60}
```

Stop-2 style threshold:

```c
delta = (s32)(free_segments - reserved_segments);
threshold = (factor * scale * 0x51EB851F) >> 37;
stop2 = delta > threshold;
```

Stop-3 signed fixed-point comparison:

```c
span = (s64)user_segments - sit_segments;
reference = (s64)(s32)(free_segments - reserved_segments);
p = left_space_table64[selector] * span;
high = smulh(p, 0xA3D70A3D70A3D70B);
scaled = (high + p) >> 6;
scaled += (p < 0);
stop3 = scaled < reference;
```

---

# 7. Detector metric roles

The project established an important metric separation:

### Arming metric

The detector arming side uses a dirty-segment family, previously reconstructed as a sum over six dirty counters.

### Stop metrics

The stop predicates use:

```text
recoverable_segments = free_segments + dirty_info->nr_dirty[PRE]
```

and then compare this against the saved baseline/reserved/capacity/progress values.

Stop operand reconstruction:

```text
Stop1 delta     = recoverable_segments - +0x9f4
Stop2 delta     = recoverable_segments - reserved_segments
Stop3 reference = recoverable_segments - reserved_segments
Stop4 delta     = (running_max - recoverable_segments) + saved_sit_segments - sit_segments
Stop5 progress  = sit_segments + (recoverable_segments - +0xa0c) <= +0xa08
```

Keep metric-producer assignments conservative if a source-level member identity has not been directly proven.

---

# 8. State-3 wait / freezer path

The region around `0x377494..0x377570` is semantically resolved at the kernel/freezer level.

High-level structure:

```text
+0x9d4 = 3
   -> timeout conversion helper
   -> scheduler/NEED_RESCHED checks
   -> wait-entry initialization
   -> prepare-to-wait / TASK_INTERRUPTIBLE
   -> scheduler recheck
   -> freezer eligibility predicate
   -> finish_wait
   -> final scheduler recheck
   -> system freezer globals / vendor +0x974 gate
   -> metric collection/re-entry
```

### `0x1051a8`

Resolved as:

```c
cgroup_freezing(struct task_struct *)
```

Binary shape is a protected task->cgroup->freezer-state lookup followed by a `state & 0x6` test, matching the Android 4.14 cgroup freezer predicate.

### `0xcc774`

Resolved as a vendor-modified inverse/eligibility form of the freezer slow path.

Conceptual behavior:

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

Do not claim the exact source name unless verified against the matching 4.14 source tree; binary semantics are the authority.

### System freezer globals

The detector's formerly anonymous `x22 + 0x20` was incorrectly treated as a vendor object field.

Correct mapping:

```text
x22 -> Image+0x19f0000
[x22 + 0x20] = system_freezing_cnt
[x22 + 0x24] = pm_freezing
[x22 + 0x28] = pm_nosig_freezing
```

### Vendor `+0x974`

Producer at `0x37acf8`:

```text
event != 9 -> return
state == 0 -> +0x974 = 1
state == 4 -> +0x974 = 0
other states -> unchanged
```

If auxiliary state `+0x898` exists, notification is sent through `+0x978` with `(3,1,0)` on state-changing branches.

Public/vendor event identity remains unresolved.

---

# 9. Synchronization helper correction

Important correction:

`0x1eca60` is **not** `__sb_start_write()`.

Current mapping places `0x1eca60` in the per-CPU reader synchronization family (`percpu_rwsem` / `percpu_down_read` lineage).

`0x1ec9e4` is the associated per-CPU reader-release path.

The separate filesystem write/GC-balancing helper is `0x341250`.

Do not conflate these addresses.

---

# 10. Exact Transsion one-argument `tran_f2fs_gc()` wrapper

Primary binary: `0x37ada8..0x37ae94`.

`+0x1a13990` is incremented as an invocation counter.

`+0x1a13998` is the Transsion controller.

Exact wrapper behavior:

```c
controller = *(u32 *)(Image + 0x1a13998);
sync = (sbi->mount_opt.opt >> 14) & 1;
background = true;
segno = NULL_SEGNO;

switch (controller) {
case 0:
    return f2fs_gc(sbi, sync, true, NULL_SEGNO);
case 1:
    old = sbi->gc_mode;
    sbi->gc_mode = 2; /* URGENT */
    ret = f2fs_gc(sbi, sync, true, NULL_SEGNO);
    sbi->gc_mode = old;
    return ret;
case 2:
    old = sbi->gc_mode;
    sbi->gc_mode = 3; /* GREEDY */
    ret = f2fs_gc(sbi, sync, true, NULL_SEGNO);
    sbi->gc_mode = old;
    return ret;
}
```

Important distinction:

```text
controller 0/1/2
is NOT the same numbering as
sbi->gc_mode
```

Nearby strings identify the vendor `gc_mode` meanings:

```text
1 = COST
2 = URGENT
3 = GREEDY
```

Thus:

```text
controller 1 -> temporary gc_mode 2 (URGENT)
controller 2 -> temporary gc_mode 3 (GREEDY)
```

Source:
- `fs/f2fs/tran_gc_wrapper_reconstructed.c`

---

# 11. Actual X683 `f2fs_gc()` entry and execution path

**Critical final boundary:**

```text
0x3503a8 = actual X683 four-argument f2fs_gc()
```

`0x366cd4` is NOT the stock `f2fs_gc()`.
`0x362c40` is NOT the stock `f2fs_gc()`.

High-confidence execution chain:

```text
Transsion detector/controller
    -> freeze/per-CPU synchronization gate
    -> vendor tran_f2fs_gc wrapper
    -> 0x3503a8 f2fs_gc(sbi, sync, background, segno)
    -> victim selection
    -> victim section migration
    -> statistics
    -> repeat/checkpoint
    -> cursor cleanup
    -> gc_mutex unlock
    -> put_gc_inode
    -> return
```

### Entry/control flow

The X683 binary follows the four-argument target-era F2FS shape:

```text
sync -> gc_type = FG_GC
sync == false -> BG_GC

active filesystem check
checkpoint-error check
BG free-space / prefree handling
BG -> FG escalation when free sections remain insufficient
reject BG GC when background == false
```

### Victim selection boundary

Around `0x350808..0x350850`, the binary performs the dirty-manager vector call:

```c
DIRTY_I(sbi)->v_ops->get_victim(
    sbi,
    &segno,
    gc_type,
    NO_CHECK_TYPE,
    LFS);
```

This is the actual victim-selection boundary.

Target-era policy family:

```text
BG_GC -> cost-benefit by default
FG_GC -> greedy by default
```

`gc_mode` modifies the resulting policy.

### Migration engine

The victim section is processed with the ordinary F2FS migration architecture:

```text
victim section
  -> optional SSA/summary-page readahead
  -> summary-page acquisition
  -> block plug
  -> iterate segs_per_sec segments
  -> empty/stale/checkpoint-error skips
  -> NODE -> gc_node_segment()
  -> DATA -> gc_data_segment()
  -> per-segment stats
  -> FG freed-segment test
  -> merged NODE/DATA writes
  -> finish plug
  -> GC call accounting
```

Historical 4.14/4.15 F2FS matches this architecture; the X683 machine code directly confirms the section traversal and dispatch structure.

### NODE/DATA statistics — exact X683 mapping

`stat_info` pointer is `sbi +0x568`.

Direct X683 mapping:

```text
stat +0x164 = call_count
stat +0x18c = tot_segs
stat +0x190 = data_segs
stat +0x194 = node_segs
stat +0x198 = bg_data_segs
stat +0x19c = bg_node_segs
```

Direct binary evidence:

```text
DATA branch: segment types 0,1,2 (w23 <= 2)
    -> +0x18c++
    -> +0x190++
    -> +0x198 += local_bg_gc_flag

NODE branch: segment types 3,4,5 (w23 > 2)
    -> +0x18c++
    -> +0x194++
    -> +0x19c += local_bg_gc_flag
```

Historical `stat_inc_seg_count()` shape matches these roles, but X683 offsets were independently established from machine code.

Additional vendor stats:

```text
stat +0x170 = vendor GC total-style counter, exact source name unresolved
stat +0x174 = vendor NODE counter
stat +0x178 = vendor DATA counter
stat +0x184 = vendor NODE/background accumulator
stat +0x188 = vendor DATA/background accumulator
```

The names `gc_node_counter/gc_data_counter` are descriptive placeholders only; do not present them as original member names.

Other known stat regions:

```text
stat +0x1a0       related to SBI +0x540 state/counter
stat +0x1a8       related to SBI +0x548 state/counter
stat +0x1b0..1c4  six dirty-info-derived counters
stat +0x1c8..1f4  normalized/derived dirty statistics
stat +0x1f8..218  contiguous SBI-statistics copies/derived values
```

Exact widths and source-level names remain unresolved.

### Retry/skip machinery

The X683 core contains the full target-era retry structure, not a simplified vendor GC fork.

Recovered elements include:

```text
skipped_atomic_files
skipped_gc_rwsem
MAX_SKIP_GC_COUNT-style comparisons
f2fs_drop_inmem_pages_all(sbi, true)
repeat GC when pressure remains
foreground/promotion checkpoint handling
```

The X683 binary uses SBI skip counters around `+0x548/+0x550`; do not assign final names to adjacent skip fields until exact source structure is matched.

### Foreground completion

For each section:

```text
seg_freed++ when FG_GC and get_valid_blocks(segno) == 0
```

A full section is recognized when:

```c
seg_freed == sbi->segs_per_sec
```

which then contributes to `sec_freed`.

### Repeat / checkpoint

Async/background path repeats with:

```text
segno = NULL_SEGNO
```

when free sections remain insufficient.

Promoted/foreground async GC can issue a checkpoint after collection.

### Cursor cleanup / epilogue

Direct binary proof:

```text
SIT_I(sbi)->last_victim[ALLOC_NEXT] = 0
SIT_I(sbi)->last_victim[FLUSH_DEVICE] = init_segno
```

The explicit-segment API change explains this state.

Direct binary proof at `0x352ba8`:

```text
mutex_unlock(&sbi->gc_mutex)
```

Then:

```text
put_gc_inode()
```

Synchronous outcome structure:

```text
complete section freed -> success
no complete section freed -> -EAGAIN
```

Source scaffold:
- `fs/f2fs/gc_reconstructed.c`

This remains reconstructed/inferred source, not proprietary-source recovery and not build-proven.

---

# 12. Separate vendor policy routine at `0x366cd4`

`0x366cd4` is the remaining major vendor-specific policy/orchestration block.

Binary range analyzed:

```text
0x366cd4..0x3671c4
```

Call site relationship from detector:

```c
per-CPU/synchronization gate
tran_f2fs_gc(sbi)
release synchronization
```

### Entry guards

```text
sbi +0x48 bit3 set -> return

0x35cc18(sbi,4)
    false -> 0x373108(sbi,0x80)

0x35cc18(sbi,1)
    false -> 0x35d22c(sbi,455)

0x35cc18(sbi,0)
    true  -> 0x362c40(sbi,0,0)
    false -> 0x363288(sbi,0xe38)
```

These helper functions are intentionally kept opaque.

### `gc_mode` branch

`0x366cd4` directly reads:

```text
sbi +0x534 = gc_mode
```

`gc_mode == 3` takes the urgent branch before the normal seven-field guard sequence.

For non-urgent path, the wrapper checks:

```text
sbi +0x444
sbi +0x448
sbi +0x44c
sbi +0x450
sbi +0x454
sbi +0x458
sbi +0x45c
```

Any active/nonzero guard changes the path.

The wrapper then repeats fixed-point free-space/capacity tests using the recovered `0x51EB851F >> 37` arithmetic family, dirty/reservation comparisons, and time/current-value gates.

### Common policy path

The later common path uses:

```text
0x35cc18(sbi,1)
    false -> return

0x35cc18(sbi,3)
    false -> return

compare dirty_info +0x84 against sm_info +0x64

repeat free/main percentage guard
repeat current block/segment guard
repeat elapsed-time guard
```

Only after these gates succeed does the vendor policy routine enter the final GC/write/side-effect machinery.

### Helper roles recovered

`0x35cc18`:
- multi-mode vendor GC policy predicate family
- selector values observed: `0..5`
- returns Boolean-like condition values
- exact symbolic names remain unresolved

`0x373108`:
- vendor policy gate keyed from `sbi +0x4b9` bit 5
- has additional segment/threshold accounting
- not merely a byte accessor

`0x35d22c`:
- vendor state/list-management helper
- called with literal `455`
- exact semantic name unresolved

`0x362c40`:
- vendor segment-manager/bitmap traversal helper
- called as `(sbi,0,0)` from the policy wrapper
- scans manager state and calls lower helper `0x3655d8`
- definitely not stock `f2fs_gc()`

`0x363288`:
- vendor linked-list drain/cleanup-style helper
- called with `(sbi,0xe38)` from policy wrapper
- do not equate with `put_gc_inode()` without additional proof

`0x341250`:
- separate filesystem write/GC-balancing helper
- reached from the final terminal path

Terminal path when `sbi +0x4b9` bit 7 is set:

```text
0x3e1014
 -> 0x34e224(sbi,1)
 -> 0x3e1558
 -> 0x341250(sbi->sb,1)
 -> vendor stat +0x16c increment
```

`+0x16c` is a vendor GC-policy statistic; exact source member name unresolved.

### Important source rule

Do not replace `0x366cd4` with guessed upstream F2FS code. It is a Transsion-specific policy layer around the stock F2FS engine.

---

# 13. Vendor control registrations

Known registration/descriptors:

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

These are control/data descriptors, not proven standalone callback symbols.

Known string:

```text
tran_gc_usb_wakelock
```

Separate registration/use path remains unresolved.

---

# 14. Historical-source comparison status

The target is the Android/Linux 4.14-era four-argument F2FS implementation matching the stock X683 Image.

The comparison has established:

```text
X683 keeps the normal F2FS GC engine:
    victim selection
    summary/SSA handling
    node/data migration
    merged writes
    segment stats
    retry/checkpoint
    cursor cleanup
    mutex release

Transsion adds around it:
    static detector
    Stop 1..5
    controller state
    gc_mode override wrapper
    fragmentation/capacity arithmetic
    vendor policy predicates
    vendor stats/control registrations
    charger/USB/display-related control coupling
```

Correct reconstruction philosophy:

```text
stock-ish X683 4.14 F2FS core
+
Transsion detector/policy/controller layer
```

Do not wholesale replace `gc.c` with vendor pseudocode.

Historical public source is comparison evidence only. Exact X683 revisions still need matching against the correct source tree before compile use.

---

# 15. Current source files in repository

Core reconstruction files:

```text
fs/f2fs/gc_reconstructed.c
fs/f2fs/tran_gc_thread_reconstructed.c
fs/f2fs/tran_gc_threshold_reconstructed.c
fs/f2fs/tran_gc_wrapper_reconstructed.c
fs/f2fs/tran_gc_policy_reconstructed.c
```

Important documentation/artifacts include:

```text
docs/reverse-engineering/tran-f2fs-gc-execution-path-final.md
docs/reverse-engineering/tran-f2fs-gc-wrapper-deep-pass.md
docs/reverse-engineering/tran-f2fs-gc-policy-binary-pass2.md
docs/reverse-engineering/x683-f2fs-gc-2018-differential.md
docs/reverse-engineering/x683-stat-info-gc-mapping.md
docs/reverse-engineering/x683-curseg-array-resolution.md
docs/reverse-engineering/f2fs-414-vendor-delta.md
docs/reverse-engineering/gc-reconstruction.md
docs/reverse-engineering/gc-victim-deep-pass.md
docs/reverse-engineering/gc-mode-state-machine-deep-pass.md
docs/reverse-engineering/bootimg-gc-artifacts.md
docs/reverse-engineering/f2fs-gc-call.md
docs/reverse-engineering/f2fs-layout.md
```

Raw/key evidence artifacts already in repository:

```text
docs/reverse-engineering/bootimg-analysis-manifest.md
docs/reverse-engineering/bootimg-artifact-index.md
docs/reverse-engineering/bootimg-gc-key-hex.txt
```

---

# 16. Important source corrections already made

1. **X683 GC ABI**
   - Wrong earlier 3-argument assumption was removed.
   - Correct stock X683 entry: four arguments with `segno`.

2. **`tran_f2fs_gc()` boundary**
   - `0x37ada8` is the actual Transsion one-argument controller wrapper.
   - `0x3503a8` is the actual X683 `f2fs_gc()` body.
   - `0x366cd4` is a separate Transsion policy wrapper.
   - `0x362c40` is not `f2fs_gc()`.

3. **`0x1eca60`**
   - Earlier `__sb_start_write()` label was wrong.
   - Current identity is per-CPU reader synchronization (`percpu_rwsem` / `percpu_down_read` lineage).

4. **Freezer `+0x20`**
   - Earlier vendor-controller interpretation was wrong.
   - `Image+0x19f0000 +0x20` is `system_freezing_cnt`.

5. **`0xcc774`**
   - It is a vendor-modified freezer slow-path eligibility predicate.
   - X683 additionally rejects `PF_KSWAPD`.

6. **Curseg offsets**
   - Old `dirty_info +0x1ac/+0x21c/+0x28c` interpretation was wrong.
   - Correct path is `sm_info +0x18 -> curseg_array`, stride `0x70`, member `+0x5c`.

7. **Metric-producer split**
   - Arming dirty metric and stop recoverable metric are distinct.
   - Do not collapse them into one generic detector metric.

8. **GC execution path**
   - The migration engine is inside `0x3503a8`.
   - It is not `0x362c40` and not the vendor policy wrapper.

9. **GC mutex**
   - X683 `f2fs_gc()` epilogue directly unlocks `sbi +0x508` (`gc_mutex`) at `0x352ba8`.

10. **`stat_info` segment counters**
   - Exact mapping:

```text
+0x18c tot_segs
+0x190 data_segs
+0x194 node_segs
+0x198 bg_data_segs
+0x19c bg_node_segs
```

---

# 17. Current confidence / honest status

Approximate project state:

```text
Binary GC architecture:                 ~93–95% confidence
Detector/controller:                    high
tran_f2fs_gc wrapper:                   essentially exact
X683 f2fs_gc execution path:             high
Vendor 0x366cd4 policy body:            substantial but incomplete
Source reconstruction:                  not build-proven
Replacement-kernel readiness:            not established
```

What is genuinely complete at high confidence:

- stock boot/Image verification
- critical X683 F2FS layout relationships
- four-argument X683 GC ABI
- Transsion controller semantics
- Stop 1–5 direct writes
- threshold arithmetic
- detector state machine at architectural level
- freezer/cgroup integration
- `+0x974` producer/consumer role
- victim-selection boundary
- X683 section migration architecture
- NODE/DATA dispatch
- exact core segment-stat mapping
- async retry/checkpoint architecture
- SIT cursor cleanup
- `gc_mutex` release
- vendor wrapper vs stock core boundaries
- substantial `0x366cd4` branch map

Remaining major blockers:

1. exact source-level semantics of `0x35cc18` selector cases;
2. exact semantics/names of `0x35d22c`;
3. exact implementation of `0x362c40` and `0x3655d8`;
4. exact implementation of `0x363288`;
5. exact identity/behavior of `0x3e1014`, `0x34e224`, `0x3e1558`, `0x341250`;
6. X683-specific `gc_node_segment()` / `gc_data_segment()` deltas vs the matched 4.14 source revision;
7. `tran_gc_usb_wakelock` registration/use path;
8. final vendor-specific `stat_info` member names around `+0x16c`, `+0x170..+0x188`, and secondary regions;
9. integration against the correct X683/H694 4.14.141 source tree;
10. compile validation and eventual kernel image construction.

Do not claim source/build readiness until these are resolved or explicitly isolated.

---

# 18. Recent authoritative commits

Recent important commits on `main` include:

```text
98c2eceb7549339eb997013d86591eb1ce953d6f
  corrected final GC entry boundary / execution-path document

4fb4860f6e4ca34c66a96db6f73d1807c4861b6c
  corrected X683 f2fs_gc mutex release

f99d88723683ded23aeff891aeebc2000a19d3f5
  tightened four-argument GC reconstruction

a9e9d17ca3e20f07bab01670bbb5a5a4a3bbacd7
  initial full execution-path reconstruction

718250acd83fe4fd033f75059720ee0afd9a4dc0
  exact tran_f2fs_gc wrapper correction

38fd69a4d61081c63e3cf6394f7b7cdba428ca5f
  exact wrapper evidence

85b8a278e9c20ffe8cf13f4f39e2229eaea24bee
  corrected 0x1eca60 identity

f9f447f02964165bdd442a2f0df46b779f9db7e7
  GC helper-cluster deep pass

3adca1781e7c8e4a24bda7638b1ec0d4a064e155
  curseg-array resolution artifact

351028809ffd98a7dcea61e9d00c20510ede12bb
  curseg resolution update / correction

20475f4358c005dad264e18134d8111453369269
  stat_info GC mapping update

e44a76778487cd3355e02d97660a7227969b2082
  exact DATA/NODE stat mapping update

28ee5c41b87e72781fa8c862f29bbcbd4e79716c
  2018/4.14 GC differential notes

a0b09c0002ec3aa6818e0c1b749fda6feaaef39c
  tightened gc_reconstructed.c

b734a4add48a74cedcecdbb9902261d76470ebf7
  vendor GC policy binary pass 2

d8f2ee6519f0dea0336fd2475e590142319f365c
  sanitized tran_gc_policy_reconstructed.c

c1529d8c42bbbd1bcc9e5ad576262570b0b060e9
  stronger vendor policy/helper findings
```

Some intermediate documentation commits may predate later corrections; when conflicting, **this handoff and the latest binary-derived notes are authoritative**.

---

# 19. Exact next work after handoff

Continue in this order:

### A. Finish `0x366cd4`

Trace every `0x35cc18` selector case and resolve its data dependencies.

Then resolve:

```text
0x35d22c
0x362c40
0x3655d8
0x363288
0x3e1014
0x34e224
0x3e1558
0x341250
```

Classify each as:

```text
upstream F2FS helper
vendor helper
vendor policy wrapper
filesystem balance/write helper
control/data descriptor path
```

### B. Exact X683 migration deltas

Compare the stock `0x3503a8` NODE/DATA migration calls against the exact nearest 4.14.141 source revision.

Resolve whether any of:

```text
gc_node_segment()
gc_data_segment()
f2fs_ra_meta_pages()
f2fs_get_sum_page()
f2fs_submit_merged_write()
f2fs_drop_inmem_pages_all()
```

have vendor modifications.

### C. Vendor controls

Trace:

```text
need_switch_ssr
tran_urgent_gc
detect_charger_type
tran_gc_usb_wakelock
```

from registration descriptor through reads/writes/callbacks.

### D. Final stat_info map

Resolve remaining source-level names for:

```text
+0x16c
+0x170..+0x188
+0x1a0
+0x1a8
+0x1b0..+0x218
```

### E. Source-tree integration

Only after the binary reconstruction is internally consistent:

```text
obtain/match correct X683/H694 4.14.141 source tree
apply recovered vendor layer
replace/augment reconstructed gc.c pieces
compile
fix structural/API mismatches
compare symbols/control-flow to stock
then attempt kernel image production
```

---

# 20. Practical warning for future chats

Do not restart the investigation from generic F2FS documentation.

Do not revert to the old three-argument `f2fs_gc()` theory.

Do not call `0x366cd4` the stock `f2fs_gc()`.

Do not call `0x362c40` the stock `f2fs_gc()`.

Do not label `0x1eca60` as `__sb_start_write()` without revisiting the correction.

Do not treat the detector's `x22+0x20` as a vendor controller field.

Do not use newer F2FS states/structures merely because they resemble the binary.

The stock Image is authoritative whenever public source and binary interpretation disagree.

Use this file as the single canonical handoff for the next chat.
