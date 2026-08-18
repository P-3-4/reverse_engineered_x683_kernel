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

There are many historical/intermediate branches. Do not continue development from them or merge them blindly. They are historical work products. Continue from `main` and preserve only evidence-backed corrections.

## 4. Stock boot image identity

The source boot image used for the direct binary analysis is:

- Size: 33,554,432 bytes (32 MiB)
- SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- Android magic: `ANDROID!`
- Board: `CY-X683-H694-E`
- Page size: 2048 / `0x800`
- Kernel load address: `0x40080000`
- Image text offset: `0x80000`

The complete `boot.img` is not stored in the repository because of GitHub's file-size/upload limitations. Its identity and the relevant extracted evidence are permanently recorded.

## 5. Kernel artifact

The user manually uploaded the compressed kernel to `main`:

`artifacts/kernel/x683_kernel_compressed.gz`

GitHub records this file as 9,640,652 bytes with blob SHA `2aad2f928a7af962831bc0d5cb33df94adf254c0`.

Local extraction from the uploaded boot image produced:

- compressed gzip member: 9,640,652 bytes
- decompressed ARM64 Image: 26,615,820 bytes
- compressed gzip SHA-256: `6ddfd017d9ee7152a856f46657f9ddd5287adf69d49cb853f7e747c2b7c18dfd`
- decompressed Image SHA-256 from that extraction: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

IMPORTANT: some older repository boot-image documents report a different compressed-kernel size/hash (`9,755,348`, SHA `670198...`). This inconsistency must be sanity-checked before using those older hashes as canonical. The actual committed kernel artifact in `artifacts/kernel/` is the current canonical binary available from GitHub.

The extraction method must stop at gzip EOF and must not treat trailing bytes in the kernel slot as additional gzip members.

## 6. Permanent binary evidence

Relevant evidence is already committed as text/hex/disassembly:

- `docs/reverse-engineering/bootimg-analysis-manifest.md`
- `docs/reverse-engineering/bootimg-artifact-index.md`
- `docs/reverse-engineering/bootimg-gc-artifacts.md`
- `docs/reverse-engineering/bootimg-gc-key-hex.txt`
- `docs/reverse-engineering/bootimg-gc-mode-evidence.md`
- `docs/reverse-engineering/transsion-gc-stop-conditions-disassembly.md`
- `docs/reverse-engineering/gc-abi-correction.md`
- `docs/reverse-engineering/gc-mode-state-machine-deep-pass.md`
- `docs/reverse-engineering/f2fs-layout.md`
- `fs/f2fs/gc_reconstructed.c`

Use the direct binary artifacts as higher authority than earlier prose that has since been corrected.

## 7. Critical kernel offsets

All following code offsets are offsets in the decompressed AArch64 kernel Image unless explicitly stated otherwise:

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

High-confidence recovered offsets:

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

The segment-manager correlations are:

- `sm_info + 0x00` = `sit_info`
- `sm_info + 0x08` = `free_info`
- `sm_info + 0x10` = `dirty_info`
- `sm_info + 0x60` = `reserved_segments`

The Transsion GC code reaches free-segment accounting through `sbi -> sm_info -> free_info`; dirty victim selection reaches `dirty_info`.

## 9. IMPORTANT ABI CORRECTION — supersedes older docs/source

The stock X683 binary **does not use the old 3-argument `f2fs_gc(sbi, sync, background)` prototype** and does not use the later 5-argument force form.

Direct disassembly proves the stock entry point at `0x3503a8` uses:

```c
int f2fs_gc(struct f2fs_sb_info *sbi,
            bool sync,
            bool background,
            unsigned int segno);
```

Evidence: the function saves `w2`, `w3`, and `w1`; later it takes/reads the saved fourth argument. Call sites also set `w3` to either a real segment or `-1`.

The Transsion wrapper passes:

```c
f2fs_gc(sbi, sync, true, -1);
```

where `-1` is `NULL_SEGNO`.

Therefore every future X683 source reconstruction must use the four-argument ABI unless new direct evidence disproves it.

Older files that still state the 3-argument ABI are historical and must not be treated as current truth:

- `docs/reverse-engineering/gc-reconstruction.md`
- `docs/reverse-engineering/f2fs-api-history.md`
- `fs/f2fs/gc_reconstructed.c`

They should eventually be corrected after the binary/source correlation pass.

## 10. gc_mode state/policy

`sbi + 0x534` is `gc_mode`.

The historically compatible four-state family is:

```c
GC_NORMAL      = 0
GC_IDLE_CB     = 1
GC_IDLE_GREEDY = 2
GC_URGENT      = 3
```

Policy selection:

- normal BG GC -> cost-benefit
- normal FG GC -> greedy
- IDLE_CB -> cost-benefit
- IDLE_GREEDY -> greedy
- URGENT -> greedy

`gc_mode` is separate from `mount_opt.opt` at `sbi + 0x4b8`.

Direct stock writers/overrides exist. The Transsion wrapper temporarily overrides `gc_mode` around `f2fs_gc()` and explicitly restores the old value.

Do not invent a charging/USB/display event that directly writes `gc_mode` unless a stock store to `sbi+0x534` proves it. The vendor trigger path primarily acts through its own controller.

## 11. Transsion GC controller

The vendor GC subsystem has a controller/global state around `+0x990..+0xa0c`.

Evidence-based semantic map:

- `+0x990`: 64-bit invocation/cycle counter
- `+0x998`: 32-bit Transsion GC controller state; `0/1/2` select normal/GREEDY/URGENT wrapper behavior
- `+0x9c0`: byte guard that can block controller writes
- `+0x9d0`: loop/termination state
- `+0x9d4`: detector state; observed values 1–4
- `+0x9d8`: repeated-detector counter
- `+0x9e0`: detector-cycle counter
- `+0x9f0`: running maximum/statistic
- `+0x9f4`: saved baseline/statistic
- `+0x9f8`: stop-result flag; observed 0/1/2
- `+0x9fc`: stop-condition code; observed 1/2/3
- `+0xa04`: cadence selector; zero -> 50, nonzero -> 500
- `+0xa05`: loop state
- `+0xa06`: detector enable/continue flag
- `+0xa08`: signed baseline segment value
- `+0xa0c`: baseline written-segment value

These are semantic labels inferred from instruction use, not claimed vendor source field names.

## 12. Transsion wrapper behavior

At `0x37ada8`, `tran_f2fs_gc` reads the controller at `+0x998`.

Observed policy behavior:

```text
controller 0:
    normal f2fs_gc()

controller 1:
    temporarily gc_mode = 2 (GREEDY)
    f2fs_gc(..., -1)
    restore old gc_mode

controller 2:
    temporarily gc_mode = 3 (URGENT)
    f2fs_gc(..., -1)
    restore old gc_mode
```

The wrapper has adjacent policy strings for COST, URGENT and GREEDY.

## 13. Stop Conditions 1–5

### Stop 1

At `0x377720/0x377724`, a signed `b.gt` compares the calculated `delta_seg` (`w25`) against a threshold. The threshold is derived from a table-selected value and `w21`, then scaled using `0x51EB851F >> 37`, approximately 2.5%.

If true:

```text
log Stop condition 1
controller +0x9fc = 1
```

### Stop 2

At `0x37776c/0x377770`, a signed `b.gt` compares a second calculated delta with a table-derived threshold.

If true:

```text
log Stop condition 2
controller +0x9fc = 2
```

### Stop 3

At `0x3777c8/0x3777cc` (with surrounding instructions through `0x3777d0`), a signed comparison tests a 64-bit scaled movement/cost quantity against a signed segment/reference quantity.

If true:

```text
log Stop condition 3
controller +0x9fc = 3
```

### Stop 4 — SSR trigger

At approximately `0x3777e8..0x37782c`, the calculated segment/write delta is compared against a vendor threshold.

If the Stop-4 predicate succeeds:

1. log:
   `match: Stop condition 4, dec_seg=%d, inc_written_seg=%d, switch to SSR`
2. test controller `+0x9c0`
3. if not blocked, write `2` to controller `+0x998`
4. write `1` to `+0x9f8`

This is direct binary proof that Stop 4 is an SSR-switch trigger.

### Stop 5 — periodic no-progress trigger

At `0x377830..0x377878`, `+0xa04` selects cadence:

- zero -> 50
- nonzero -> 500

The code computes `+0x990 % cadence`. At the selected periodic point it compares current segment/write progress against `+0xa08` / `+0xa0c` baselines.

If insufficient/no free-segment progress is detected, the path writes controller `2` and sets `+0x9f8 = 2`, with literal:

`match: Stop condition 5,every 400 times gc none free segment inc`

The literal says 400 while compiled cadence selection is 50/500. Preserve this discrepancy rather than normalizing it.

## 14. SSR decision path

Current binary-supported path:

```text
segment/write delta exceeds vendor threshold
        -> Stop Condition 4
        -> controller +0x998 = 2 (unless +0x9c0 blocks write)
        -> +0x9f8 = 1
        -> Transsion GC wrapper sees controller 2
        -> temporary sbi+0x534 = 3 (URGENT)
        -> f2fs_gc(sbi, sync, true, -1)
        -> restore previous gc_mode
```

Stop 5 is a separate periodic no-progress trigger that also drives controller 2 and records stop result 2.

## 15. Victim-selection reconstruction

The intended stock-compatible path is:

```text
f2fs_gc
  -> select GC type / policy
  -> __get_victim
  -> SIT/dirty manager locking
  -> DIRTY_I(sbi)->v_ops->get_victim(...)
  -> victim policy (cost-benefit / greedy)
  -> last_victim cursor
```

The 4.14-era locking correction is important: use `mutex_lock(&sentry_lock)` for the older lineage rather than importing the later 4.15 rwsem form blindly.

The victim cost correction is also important: greedy selection gives data segments a doubled cost relative to node segments in the historical implementation. Do not use a raw `valid_blocks` cost for all segment types.

## 16. Garbage-collection helper reconstruction

The reconstructed `do_garbage_collect` work includes the historical sequence:

- summary-page acquisition/read-ahead
- summary footer/type dispatch
- node/data segment migration
- statistics
- foreground merged-write submission
- complete-section accounting

However the current `fs/f2fs/gc_reconstructed.c` is **not build-ready** because its function prototype still uses the obsolete three-argument ABI. It must be corrected to four arguments before integration.

## 17. Current source status

Repository currently contains reconstructed/inferred F2FS GC source and documentation. It is not yet a verified stock-equivalent build.

Main source areas:

- `fs/f2fs/gc_reconstructed.c`
- `fs/f2fs/...` reconstructed F2FS pieces
- `reconstructed/` source fragments
- `docs/reverse-engineering/` evidence and analysis

Do not replace evidence-backed offset correlations with guessed struct members until the stock binary proves the correspondence.

## 18. What has been corrected during the project

1. Do not confuse `sbi+0x4b8` (`mount_opt.opt`) with `sbi+0x534` (`gc_mode`).
2. Do not use `gc_thread->gc_idle` as the X683 GC policy field.
3. Use the 4.14-era mutex `sentry_lock` where appropriate; later rwsem code is a revision mismatch.
4. Greedy victim cost must account for data-vs-node weighting.
5. Stop 4 is a real SSR-switch path, not just a logging branch.
6. Transsion controller value 2 feeds the vendor wrapper's URGENT override.
7. The stock X683 GC ABI is four arguments; this supersedes earlier three-argument notes.
8. Exact vendor triggers must not be invented from strings alone.

## 19. Known contradictions / sanity checks required

### Kernel-size/hash contradiction

Some repository documents identify the source boot image as having a 9,755,348-byte compressed kernel and report a compressed-kernel hash `670198...`, while the actual kernel artifact manually uploaded to `artifacts/kernel/` is 9,640,652 bytes and has local extraction hash `6ddfd...`.

Before doing any further absolute-offset work from the uploaded gzip artifact, verify that the committed gzip is actually the same kernel used to produce the documented offsets and that the boot-image parser did not select a different gzip member/copy. The boot image SHA itself is `a4908a...` in the existing manifest.

This check is mandatory to avoid silently mixing two kernel payloads.

### Older ABI documents

`gc-reconstruction.md`, `f2fs-api-history.md`, and the header comment in `gc_reconstructed.c` still contain the obsolete three-argument assumption. The direct binary ABI correction document is authoritative and those files should be updated.

## 20. Immediate next work

Do this in order:

1. **Sanity-check the committed `x683_kernel_compressed.gz` against the exact boot-image SHA and documented kernel slot.** Resolve the 9,640,652 vs 9,755,348 discrepancy before trusting further absolute offsets.
2. Decompress the committed gzip and verify the resulting Image SHA against the documented `965138...` hash.
3. Re-disassemble `0x3503a8`, `0x37ada8`, and `0x377700..0x3779b0` from the canonical Image to make sure all existing evidence corresponds to the same binary.
4. Correct `fs/f2fs/gc_reconstructed.c` to the four-argument ABI.
5. Reconstruct the exact `tran_gc_thread_func` around the controller fields.
6. Resolve the remaining vendor helper/threshold routine at `0x37b5d4`.
7. Trace charging, USB, framebuffer, wakelock, fragmentation, `tran_urgent_gc`, `need_switch_ssr`, and `detect_charger_type` callers to establish actual vendor triggers.
8. Match the vendor code against the exact historical F2FS revision before claiming source equivalence.
9. Reconstruct `__get_victim`, cost-benefit/greedy selection, `last_victim[]`, and dirty-segment operations against the stock binary.
10. Only after these are consistent, integrate reconstructed C and begin compile/boot validation.

## 21. Method / evidence standard

Use this confidence hierarchy:

1. Direct stock AArch64 instruction behavior.
2. Direct stock strings/literal cross-reference combined with control flow.
3. Recovered struct offsets from multiple independent accesses.
4. Historical public F2FS source matching the binary's revision neighborhood.
5. Inference only when clearly labelled.

Never promote an inference to a fact just because an upstream version looks similar.

## 22. New-chat continuation instruction

Start the next chat by reading this file and the following documents before doing new analysis:

- `docs/reverse-engineering/x683-project-handoff.md` (this file)
- `docs/reverse-engineering/bootimg-gc-artifacts.md`
- `docs/reverse-engineering/bootimg-gc-mode-evidence.md`
- `docs/reverse-engineering/transsion-gc-stop-conditions-disassembly.md`
- `docs/reverse-engineering/gc-abi-correction.md`
- `docs/reverse-engineering/f2fs-layout.md`
- `docs/reverse-engineering/gc-mode-state-machine-deep-pass.md`
- `docs/reverse-engineering/bootimg-gc-key-hex.txt`
- `fs/f2fs/gc_reconstructed.c`

Then perform the kernel-artifact/hash sanity check before continuing the reverse engineering.
