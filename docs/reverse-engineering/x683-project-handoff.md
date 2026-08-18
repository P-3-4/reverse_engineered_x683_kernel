# X683 / H694 Kernel Reverse-Engineering — Project Handoff

This is the canonical continuation document for moving the project to a new chat. Work from `main` and treat this document plus the referenced evidence artifacts as the current project state.

## 1. Objective

Reverse-engineer the stock Infinix X683 / H694 MT6768 kernel and reconstruct a functionally equivalent, buildable kernel/source tree. Initial target is the stock-equivalent Linux 4.14.141-era kernel and Android 10 userspace compatibility. Do not modernize until stock-equivalent behavior is established.

Recovered code must be labelled reconstructed/inferred and must not be represented as proprietary Transsion source.

## 2. Device / kernel target

- Device: Infinix X683 / H694
- SoC: MediaTek MT6768
- Kernel lineage: Linux 4.14.141+
- Architecture: ARM64 / AArch64
- Stock kernel is a vendor/Transsion 4.14-derived tree with later/backported F2FS changes.
- Stock boot board string: `CY-X683-H694-E`.

## 3. Canonical branch / repository

Repository: `P-3-4/reverse_engineered_x683_kernel`

Canonical branch: `main`.

Continue from `main` and preserve only evidence-backed corrections.

## 4. Stock boot image identity

The uploaded stock boot image is now directly verified in this chat:

- Size: 33,554,432 bytes (32 MiB)
- SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- Android magic: `ANDROID!`
- Board: `CY-X683-H694-E`
- Page size: 2048 / `0x800`
- Kernel compressed size: `0x94dad4` = 9,755,348 bytes
- Decompressed ARM64 Image: 26,615,820 bytes
- Decompressed Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

The gzip stream has 114,696 trailing bytes after the gzip member, so decompression must stop at gzip EOF.

## 5. Repository kernel artifact

`artifacts/kernel/x683_kernel_compressed.gz` is present on `main` and GitHub reports its blob size as 9,640,652 bytes. The boot.img supplied in this chat independently verifies the 9,755,348-byte kernel slot and the 26,615,820-byte decompressed Image above. Treat the freshly uploaded boot.img as the direct binary authority for offset work.

## 6. Permanent binary evidence

Relevant evidence:

- `docs/reverse-engineering/bootimg-analysis-manifest.md`
- `docs/reverse-engineering/bootimg-artifact-index.md`
- `docs/reverse-engineering/bootimg-gc-artifacts.md`
- `docs/reverse-engineering/bootimg-gc-key-hex.txt`
- `docs/reverse-engineering/bootimg-gc-mode-evidence.md`
- `docs/reverse-engineering/transsion-gc-stop-conditions-disassembly.md`
- `docs/reverse-engineering/gc-abi-correction.md`
- `docs/reverse-engineering/gc-mode-state-machine-deep-pass.md`
- `docs/reverse-engineering/tran-gc-detector-arming-deep-pass.md`
- `docs/reverse-engineering/f2fs-layout.md`
- `fs/f2fs/gc_reconstructed.c`
- `fs/f2fs/tran_gc_reconstructed.c`
- `fs/f2fs/tran_gc_thread_reconstructed.c`

Use direct binary artifacts and the supplied boot.img above prose that has since been corrected.

## 7. Critical kernel offsets

Offsets below are decompressed-kernel offsets:

- `0x3503a8`: stock `f2fs_gc` entry point
- `0x345d58`: `sbi + 0x534` read
- `0x345d6c`: temporary `gc_mode = 3`
- `0x345d78`: restore `gc_mode`
- `0x352f10`: `gc_mode` read in GC/victim path
- `0x352f58`: another `gc_mode` read
- `0x365918`: `gc_mode` read in GC logic
- `0x374d4c`: F2FS sysfs handler region
- `0x3750f4`: standard urgent `gc_mode = 3` writer
- `0x37515c`: standard idle `gc_mode` writer (1/2)
- `0x375168`: normal `gc_mode = 0`
- `0x376f84`: `tran_gc_thread_func` related literal/reference
- `0x377120..0x377494`: detector arming/runtime state transitions
- `0x377494`: explicit detector state transition to `+0x9d4 = 3`
- `0x377700..0x3779b0`: Stop Conditions 1–5
- `0x37ada8`: Transsion `tran_f2fs_gc` wrapper
- `0x37adc4`: Transsion controller read at `+0x998`
- `0x37adfc`: vendor temporary `gc_mode = 3`
- `0x37ae00`: vendor call to `f2fs_gc`
- `0x37ae04`: restore previous mode
- `0x37ae68`: vendor temporary `gc_mode = 2`
- `0x37ae6c`: vendor call to `f2fs_gc`
- `0x37ae7c`: restore previous mode
- `0x37b5d4..0x37b8c0`: GC threshold/helper routine

Important literals include `tran_f2fs_gc`, `gc mode is COST`, `gc mode is URGENT`, `gc mode is GREEDY`, `tran_gc_usb_wakelock`, `tran_gc_thread_func create`, `tran_gc loop static detect`, `kernel or os is holding wakelock!`, `f2fs is writing data`, `tran_urgent_gc`, `need_switch_ssr`, `detect_charger_type`, and the Stop-4 SSR string.

## 8. Critical f2fs_sb_info layout

High-confidence offsets:

| Offset | Identification | Confidence |
|---:|---|---|
| `0x3d8` | `log_blocks_per_seg` | High |
| `0x3dc` | `blocks_per_seg` | High |
| `0x3e0` | `segs_per_sec` | High |
| `0x408` | `user_block_count` | High |
| `0x428` | `reserved_blocks` | High |
| `0x430` | `current_reserved_blocks` | High |
| `0x438` | `unusable_block_count` | High |
| `0x440` | `nquota_files` | High |
| `0x4b8` | `mount_opt.opt` | High |
| `0x534` | `gc_mode` | High / directly supported by binary accesses

Segment-manager correlations:

- `sm_info + 0x00` = `sit_info`
- `sm_info + 0x08` = `free_info`
- `sm_info + 0x10` = `dirty_info`
- `sm_info + 0x5c` = `main_segments`
- `sm_info + 0x60` = `reserved_segments`

Relevant dirty-manager offsets used by the detector:

- `dirty_info + 0x68..0x7c`: six consecutive per-type dirty counters
- `dirty_info + 0x84`: prefree/recoverable counter used by detector

## 9. ABI authority

Stock X683 uses:

```c
int f2fs_gc(struct f2fs_sb_info *sbi,
            bool sync,
            bool background,
            unsigned int segno);
```

Transsion wrapper uses `segno = -1` (`NULL_SEGNO`).

All future X683 reconstruction must use the four-argument ABI.

## 10. gc_mode policy

`sbi + 0x534` is `gc_mode`.

Compatible policy family:

```c
GC_NORMAL      = 0
GC_IDLE_CB     = 1
GC_IDLE_GREEDY = 2
GC_URGENT      = 3
```

Vendor wrapper:

```text
controller 0 -> normal f2fs_gc
controller 1 -> temporary gc_mode 2 / GREEDY / restore
controller 2 -> temporary gc_mode 3 / URGENT / restore
```

## 11. Transsion controller state

Known semantic fields:

- `+0x990`: 64-bit invocation/cycle counter
- `+0x998`: controller: 0 normal / 1 greedy / 2 urgent
- `+0x9c0`: controller-write guard
- `+0x9d0`: loop/termination state
- `+0x9d4`: detector state; stock writes include 2 and 3
- `+0x9d8`: repeated-detector counter
- `+0x9e0`: detector-cycle counter
- `+0x9f0`: running maximum/statistic
- `+0x9f4`: saved recoverable-segment baseline
- `+0x9f8`: stop result, 1/2 for Stops 4/5
- `+0x9fc`: stop condition, 1/2/3 for Stops 1/2/3
- `+0xa00`: detector mode/state gate
- `+0xa04`: cadence selector, 0 -> 50 / nonzero -> 500
- `+0xa05`: loop-active/state byte
- `+0xa06`: detector-active/continue byte
- `+0xa08`: signed segment baseline
- `+0xa0c`: written/recoverable baseline

## 12. Detector arming: current exact reconstruction

At `0x377120..0x377494`, the static detector first increments a global cycle/statistic and maintains `+0x9f0` from a vendor object field.

The initial F2FS-derived quantities are:

```c
sm        = SM_I(sbi);
sit       = sm->sit_info;
free_i    = sm->free_info;
dirty_i   = sm->dirty_info;

recoverable = sum(dirty_i counters at 0x68..0x7c);
user_segments = sbi->user_block_count >> sbi->log_blocks_per_seg;
```

A preliminary ratio is computed from main/free/SIT/user quantities and divided by the recoverable-plus-free denominator. The result is held in `w21` and must reach at least `0x15f` for the static arming path to continue.

The detector then checks:

1. recoverable dirty state is large enough relative to `user_segments`;
2. the computed ratio `w21 >= 351`;
3. free-segment capacity passes a main-segment-derived threshold;
4. `(user_block_count - sit_blocks)` exceeds a ~2.5% scaled threshold based on `13 * user_block_count`;
5. `sbi + 0x3f0` exceeds a ~2.5% scaled threshold based on `27 * sit_blocks`.

If all pass:

```text
+0xa00 = 1
+0xa04 = 1
```

Otherwise:

```text
+0xa00 = 2
```

The common continuation then sets:

```text
+0x9d4 = 2
+0xa06 = 1
```

This is now a proven state transition, not a guessed statistic.

## 13. Detector state 3 transition

At `0x377494` the stock image explicitly performs:

```text
+0x9d4 = 3
vendor-state +0x158 = 1
```

Two helper calls follow. Their exact semantic names remain unresolved.

Subsequent runtime guards check superblock state, filesystem counters, nested objects under `sm_info + 0x80`, vendor state `+0x974`, and vendor/reference state at `+0xa10`.

## 14. Stop Conditions

### Stop 1

```c
recoverable = free_segments + prefree/recoverable_dirty;
delta1 = recoverable - controller->saved_baseline;

bucket = max(global[0x890], global[0x894]);
if (bucket <= 7)
    factor = table[0..7] = {100,100,100,80,80,80,60,60};

threshold1 = factor * selected_scale * 0x51EB851F >> 37;
```

Predicate:

```c
delta1 > threshold1
```

→ `+0x9fc = 1`.

### Stop 2

```c
delta2 = recoverable - reserved_segments;
threshold2 = factor * threshold_base * 0x51EB851F >> 37;
```

Predicate:

```c
delta2 > threshold2
```

→ `+0x9fc = 2`.

### Stop 3

```c
span = user_segments - sit_segments;
scaled = signed_fixed_point(table2[bucket] * span);
reference = recoverable - reserved_segments;
```

`table2 = {80,80,80,70,70,70,60,60}` and stock uses the signed multiply-high constant `0xA3D70A3D70A3D70B` followed by the observed shift/correction sequence.

Predicate:

```c
scaled < reference
```

→ `+0x9fc = 3`.

### Stop 4

Direct stock result:

```text
condition succeeds
-> controller +0x998 = 2 unless +0x9c0 blocks
-> +0x9f8 = 1
```

Log explicitly says switch to SSR.

### Stop 5

```c
interval = +0xa04 ? 500 : 50;
if (cycle % interval == 0) {
    progress = current_sit_component +
               (current_recoverable - baseline_recoverable);
    if (progress <= baseline_segment) {
        controller = 2;
        +0x9f8 = 2;
    }
}
```

## 15. Current source status

Main reconstructed sources:

- `fs/f2fs/gc_reconstructed.c`
- `fs/f2fs/tran_gc_reconstructed.c`
- `fs/f2fs/tran_gc_thread_reconstructed.c`
- `fs/f2fs/victim_reconstructed.c`

The code remains reconstructed/inferred and is not yet a verified stock-equivalent build.

## 16. Next reverse-engineering target

The next target is the runtime path following detector state 3:

```text
0x377494 onward
-> helper at 0xce58c
-> helper at 0x57554
-> +0x974 guard
-> nested sm_info +0x80 objects
-> helper 0xe0693c
-> helper 0x1eca60
-> transition into static Stop-1..5 evaluation
```

Highest priority is resolving the helper call targets and determining whether they correspond to `detect_charger_type`, `need_switch_ssr`, `tran_urgent_gc`, wakelock checks, or filesystem write-state checks.
