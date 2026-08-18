# X683 / H694 Kernel Reverse-Engineering — Complete Findings Handoff

Date: 2026-08-18
Repository: `P-3-4/reverse_engineered_x683_kernel`
Branch: `reconstruction`

## 0. Continuation rule

This document is the authoritative project handoff for the next chat. Keep explanations minimal and work-first. Do not stop for unnecessary questions. Direct evidence from the stock X683/H694 boot.img/kernel is authoritative. Historical/public source is correspondence only.

Evidence order:
1. Direct stock binary instruction/data evidence
2. Proven register/base relationships
3. Repeated independent stock accesses
4. Historical 4.14 source correspondence
5. Semantic inference

Never promote unresolved offsets or guessed source names to facts.

## 1. Objective

Reconstruct a functionally equivalent stock-era Infinix X683/H694 Linux 4.14.141+ kernel, including Transsion modifications, with the eventual goal of booting the stock Android 10 userspace and then investigating modernization/Android 16 feasibility.

Pipeline:

```text
stock boot.img
 -> kernel Image / DTB / DTBO / config
 -> binary reverse engineering
 -> recover structures/APIs/vendor deltas
 -> reconstruct 4.14.141-era source
 -> build
 -> boot-test stock Android 10
 -> modernize
 -> investigate Android 16
```

Android 16 is not the immediate milestone.

## 2. Device / kernel

```text
Device: Infinix X683 / H694
SoC/platform: MediaTek MT6768-compatible
Architecture: ARM64
Stock userspace: Android 10
Kernel: Linux 4.14.141+
Build: Fri Nov 5 15:56:25 CST 2021
Compiler: Android clang 9.0.3
```

Identifiers recovered include `x683_h694`, `tran_x683`, `tran_x683_e1`, and `tran_pcba_h694`. Do not confuse X683/H694 with X6833/X6833B.

Embedded source paths include:

```text
kernel-4.14/fs/f2fs/f2fs.h
kernel-4.14/fs/f2fs/gc.c
kernel-4.14/fs/f2fs/segment.c
kernel-4.14/fs/f2fs/super.c
```

## 3. Boot/kernel anchors

Stock boot.img was extracted/decompressed successfully to ARM64 Image. Corrected approximate Image offsets:

```text
f2fs_gc             ~0x3503a8
tran_gc_thread_func ~0x376ed0
tran_do_f2fs_gc     ~0x37ada8
```

Earlier addresses around `0x3fa5a8` were incorrect/mixed address spaces and must not be reused.

## 4. Critical F2FS ABI

The stock X683 binary proves the four-argument form:

```c
f2fs_gc(sbi, sync, background, segno);
```

Transsion wrapper call is conceptually:

```c
f2fs_gc(
    sbi,
    (sbi->mount_opt.opt >> 14) & 1,
    true,
    NULL_SEGNO
);
```

Do not revert to the obsolete three-argument assumption.

## 5. Transsion GC architecture

Recovered vendor functions:

```text
tran_gc_init()
tran_gc_stop()
tran_gc_thread_func()
tran_do_f2fs_gc()
tran_has_enough_free_segment()
is_f2fs_fragmentation()
```

Architecture:

```text
tran_gc_thread_func
    -> tran_do_f2fs_gc
        -> manipulate/temporarily change GC mode
        -> normal f2fs_gc()
```

The vendor layer wraps the normal F2FS GC engine; it is not a completely separate filesystem GC implementation.

Vendor strings include:

```text
tran_f2fs_gc
mount userdata f2fs tran_gc is going
tran_f2fs wait emmc gc begin
tran_f2fs wait emmc gc end
f2fs alloc new segment and fragmentation is %lu
```

## 6. Confirmed/strong SBI offsets

```text
0x080  sm_info
0x3d8  log_blocks_per_seg
0x3dc  blocks_per_seg          unresolved/needs final width check
0x3e0  segs_per_sec            unresolved/needs final width check
0x408  user_block_count
0x410  total_valid_block_count
0x418  discard_blks
0x420  last_valid_block_count
0x428  reserved_blocks         unresolved semantic name
0x430  current_reserved_blocks
0x434  high 32 bits of a 64-bit quantity candidate; do not assume independent u32
0x438  unusable_block_count
0x440  nquota_files
0x4b8  mount-option/state field; exact semantic name unresolved
0x508  gc_mutex
0x528  gc_thread
0x530  cur_victim_sec
0x534  gc_mode
0x538  next_victim_seg[2]
0x540  skipped_atomic_files[0]
0x548  skipped_atomic_files[1]
0x550  skipped_gc_rwsem
0x558  gc_pin_file_threshold
0x560  max_victim_search
0x564  migration_granularity
0x568  stat_info *
0x5d4  bg_gc candidate/high confidence
0x5d8  io_skip_bggc candidate/high confidence
0x5dc  other_skip_bggc candidate/high confidence
```

`0x568` is directly established as a pointer to a separate statistics object.

## 7. sm_info reconstruction

```text
sm_info +0x00  sit_info
sm_info +0x08  free_info
sm_info +0x10  dirty_info
sm_info +0x40  seg0_blkaddr
sm_info +0x48  main_blkaddr
sm_info +0x50  ssa_blkaddr
sm_info +0x58  segment_count
sm_info +0x5c  main_segments
sm_info +0x60  reserved_segments      unresolved final semantic validation
sm_info +0x64  additional_reserved_segments
sm_info +0x68  ovp_segments
```

Dirty-info accesses at `+0x05c`, `+0x0cc`, `+0x13c`, `+0x1ac`, `+0x21c`, `+0x28c` feed six statistics, but exact semantic names remain unresolved.

## 8. stat_info findings

Direct pointer relationship:

```text
sbi + 0x568 -> stat_info *
```

Known direct mappings:

```text
sbi + 0x5a8 -> stat + 0x038
sbi + 0x5b0 -> stat + 0x040
sbi + 0x5a0 -> stat + 0x048
sbi + 0x598 -> stat + 0x058

sbi + 0x3a4 -> stat + 0x060
sbi + 0x3b8 -> stat + 0x064
sbi + 0x3bc -> stat + 0x068

sbi + 0x434 -> stat + 0x06c
sbi + 0x428 -> stat + 0x070
sbi + 0x438 -> stat + 0x074
sbi + 0x440 -> stat + 0x078
sbi + 0x42c -> stat + 0x07c
sbi + 0x430 -> stat + 0x080
sbi + 0x43c -> stat + 0x084

sbi + 0x5e0 -> stat + 0x088
sbi + 0x5e4 -> stat + 0x08c
sbi + 0x424 -> stat + 0x090
sbi + 0x5e8 -> stat + 0x094

sbi + 0x5c4 -> stat + 0x118
sbi + 0x5c8 -> stat + 0x120
sbi + 0x5cc -> stat + 0x11c
sbi + 0x5d0 -> stat + 0x124

sbi + 0x570 -> stat + 0x1f8
sbi + 0x574 -> stat + 0x1fc
sbi + 0x578 -> stat + 0x200
sbi + 0x57c -> stat + 0x204
sbi + 0x580 -> stat + 0x208
sbi + 0x584 -> stat + 0x20c
sbi + 0x588 -> stat + 0x210
sbi + 0x58c -> stat + 0x214
sbi + 0x590 -> stat + 0x218
```

Confirmed/high-confidence statistics:

```text
stat +0x18c  node_segs-like increment
stat +0x194  free_segs-like increment
stat +0x19c  GC-related accumulation
stat +0x1a0  skipped_atomic_files[0]
stat +0x1a8  skipped_atomic_files[1]
stat +0x1b0..0x1c4  six dirty-info-derived values; semantic names unresolved
stat +0x1c8..0x1f4  normalized/derived dirty statistics
```

Important vendor divergence: public/upstream structure ordering cannot simply be copied. X683 binary proves `stat+0x1a0` and `stat+0x1a8` are copies of `sbi+0x540` and `sbi+0x548`, respectively.

## 9. GC architecture

Stock compiler has inlined major logical helpers into `f2fs_gc()`.

```text
f2fs_gc()
  -> GC type/policy
  -> filesystem/background checks
  -> inline __get_victim()
  -> do_garbage_collect() logic inline
       -> summary preparation
       -> node/data migration
       -> segment accounting
       -> merged-write/accounting
  -> skip accounting
  -> GC-more/retry
  -> optional checkpoint
  -> cleanup
```

Historical 4.14 correspondence is strong, but source boundaries may be compiler-inlined in X683.

`__get_victim()` reconstruction is strongly consistent with:

```c
ret = DIRTY_I(sbi)->v_ops->get_victim(
    sbi, &segno, gc_type, NO_CHECK_TYPE, 0, 0);
```

The historical `NO_CHECK_TYPE`/LFS correspondence is a fingerprint, not an unquestioned X683 source definition.

## 10. do_garbage_collect / gc_data_segment

Logical `do_garbage_collect()` begins around `0x350854` and is inlined through approximately `0x35279c`.

Logical `gc_data_segment()` is also inlined. The stock five-phase migration architecture is strongly preserved:

```text
phase 1: liveness / read-ahead
phase 2: parent-node preparation
phase 3: inode/data preparation
phase 4: actual data migration
phase 5: accounting / cleanup
```

The binary shows SSA summary handling, per-segment iteration, node-vs-data dispatch, migration accounting, merged-write submission, GC retry and cleanup.

## 11. Resolved migration helpers

The phase-3 post-read path calls `0x36bf78`. It is high-confidence `ra_data_block()`-type logic.

`0x36bea0` is a repeated generic page/object state operation used around preparation and is **not** currently identified as `move_data_block()` or `move_data_page()`.

Do not assign historical names to these addresses without direct body evidence.

## 12. `0x36b4fc` / block replacement

`0x36b4fc` is very high confidence as the stock block-replacement primitive corresponding to `f2fs_do_replace_block()`.

The observed argument preservation matches:

```c
f2fs_do_replace_block(
    sbi,
    &sum,
    old_blkaddr,
    new_blkaddr,
    recover_curseg,
    recover_newaddr
);
```

This is one of the strongest migration anchors.

## 13. `0x35443c` status

This target was investigated repeatedly.

What is proven:

```text
0x351f54: x0 = sp - 0xd0
0x351f58: *(ctx + 0x14) = 0x0000080000000001
0x351f60: BL 0x35443c
```

The caller then checks a local status byte and has an error path returning `-EAGAIN`.

The body of `0x35443c` was **not present in the retrieved disassembly chunk**. Therefore its exact function identity is unresolved.

Previous identification as `f2fs_submit_page_write()` is downgraded/rejected. It is also not equated with `f2fs_submit_merged_write()`.

Current best label:

```text
0x35443c = unresolved X683 GC/migration orchestration helper
```

Do not source-name it yet.

## 14. `sp-0xd0` migration context

A provisional raw layout was reconstructed from callers, but it is NOT source-ready.

Known/proven access patterns include:

```text
ctx = sp - 0xd0
ctx + 0x14 = 0x0000080000000001 at phase-4 setup
ctx + 0x1c = initially zero
ctx + 0x6c = byte access
ctx + 0x6d = halfword access
ctx + 0xa0 = used as synchronization/lock-like object
ctx + 0xe0 = pointer access
```

Earlier claim that `ctx+0x00` was definitively `sbi` is downgraded; caller evidence was insufficient. Keep it unresolved until the actual `0x35443c` body is recovered.

Do not model this object as ordinary `struct f2fs_io_info` yet. It appears to be a vendor/merged GC migration context containing embedded state/IO-related fields.

## 15. Other migration anchors

```text
0x35443c  unresolved migration orchestration boundary
0x355210  data/dnode block-address update path
0x36be28  f2fs_replace_block()-type wrapper
0x36b4fc  f2fs_do_replace_block()-type primitive, very high confidence
0x373e5c  extent-cache/tree update boundary, very high confidence
```

The exact ordering is branch-dependent; do not force a single linear call chain where the compiler has merged/inlined paths.

## 16. SBI migration fields discovered late

A phase-4 path accesses:

```text
sbi + 0x5f4  lock/synchronization object used around migration
sbi + 0x630  value incremented by 0x1000 under that lock
sbi + 0x660  byte access observed in related context
```

Exact semantics remain unresolved.

## 17. Six unresolved helper functions

The six helper functions around the GC migration path were individually investigated. The safe state is:

- use binary ABI/access patterns;
- preserve unresolved helper addresses;
- use historical `move_data_block()` / `move_data_page()` only as correspondence;
- do not collapse distinct helpers into upstream names merely because the control flow looks similar.

The most valuable remaining helper target is the body of `0x35443c`.

## 18. Historical source correspondence

Closest historical 4.14 F2FS architecture includes:

```text
f2fs_gc
__get_victim
get_victim_by_default
select_policy
get_gc_cost
do_garbage_collect
gc_node_segment
gc_data_segment
move_data_block/move_data_page
f2fs_replace_block
f2fs_do_replace_block
```

X683 strongly resembles this lineage, with Transsion wrapper/policy/accounting additions and compiler inlining.

Public source is not the original X683 source.

## 19. Vendor-specific divergence already proven

Do not copy upstream structure ordering blindly.

The strongest example is:

```text
sbi+0x540 -> stat+0x1a0
sbi+0x548 -> stat+0x1a8
```

Those are `skipped_atomic_files` values, proving vendor divergence from a public structure layout that might otherwise suggest different members there.

Also preserve the possibility that `sbi+0x430/+0x434` form one 64-bit quantity.

## 20. Repository reconstruction state

Repository:

```text
P-3-4/reverse_engineered_x683_kernel
```

Active reconstruction branch:

```text
reconstruction
```

Conceptual tree:

```text
fs/f2fs/
  f2fs.h
  gc.c
  segment.c
  segment.h
  super.c
  checkpoint.c
  node.c
  data.c
  x683_layout.h
  tran_gc.c

docs/reverse-engineering/
  findings.md
  f2fs-layout.md
  f2fs-api-history.md
  f2fs-stat-recovery.md
  f2fs-source-fingerprint.md
  f2fs-baseline-candidates.md
  tran-gc.md
  hardware.md
  build-status.md
```

Existing reconstruction documents include the master findings, F2FS GC reconstruction, `gc_data_segment()`/`do_garbage_collect()` reconstruction, and provisional Transsion GC reconstruction.

## 21. Firmware / evidence inventory

Known hashes from previous project handoff:

```text
boot.img       a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180
kernel_image   96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba
boot_dtb       de123d41bd398f20e97ecc01a21721437ee1698f9c1cbc178096946c4aedf1d6
dtbo           d043a48a7350ef27ea3bd8793342f6112c1ce8b81f0943e28c044ad8929d8b2d6
dtbo_dtb       648289038314d7ab96e818a095c447b8e6dfc82c9a897f7fe27af0d9920ce495
stock_defconfig 7d789b857f2fd7af52ddbfdd5e36fba33d62162536635de15423e80525010f56
```

Firmware build example:

```text
X683-H694EFGHIJU-Q-OP-210301V288
```

Multiple X683/H694 firmware versions were identified in earlier work and can later be binary-diffed.

## 22. Other project findings

The project also established:

- X683/H694 has real stock DTB/DTBO and recovered kernel config evidence.
- Live kernel observations supplied runtime symbols/addresses, hardware bindings and module ABI evidence.
- Vendor modules include WMT/Wi-Fi/Bluetooth/GPS/MET/UDC-related modules; stock vermagic is `4.14.141+ SMP preempt mod_unload modversions aarch64`.
- Public MT6768 trees are useful for common driver/source recovery but are not proof of X683/H694 source ancestry.
- The reconstruction must eventually cover board code, DT, PMIC/power, display, camera, thermal, storage, GPU/M4U, USB, vendor modules and remaining Transsion subsystems.

## 23. Immediate next target

Recover the actual function body for:

```text
0x35443c -> function end
```

from the raw decompressed kernel, not an incomplete disassembly chunk.

Then:

1. trace every read/write of `sp-0xd0` context fields;
2. identify source page/block and destination allocation;
3. identify actual write/I/O construction;
4. locate `f2fs_replace_block()` and `f2fs_do_replace_block()` paths;
5. reconstruct the X683 equivalent of the historical `move_data_block()` / `move_data_page()` logic;
6. only then update source structures.

After that continue exact `stat_info` reconstruction and remaining SBI layout.

## 24. Final project state

The project has progressed from “find the source” to binary-to-source reconstruction. The F2FS GC core, Transsion wrapper, major SBI offsets, statistics relationships, inlined `do_garbage_collect()`/`gc_data_segment()` architecture, and block-replacement endpoint are substantially mapped.

The major unresolved technical blocker is the exact migration implementation around `0x35443c` and the remaining X683-specific structure fields. Do not hallucinate those fields or source names.
