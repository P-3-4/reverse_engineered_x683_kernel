# X683/H694 Kernel Reverse Engineering — Phase 2 Kernel Reconstruction Handoff

## Project objective

Recover the Transsion/Infinix X683/H694 vendor kernel modifications from the stock boot image and reconstruct a compilable vendor patch against the matching MediaTek Android 4.14 kernel tree.

Binary authority:

- X683/H694 boot(8).img
- compressed kernel image
- decompressed ARM64 Image

Public Linux/F2FS source is comparison material only.

---

# Binary baseline

Confirmed:

- Linux 4.14.141+
- ARM64
- MediaTek MT6768 platform
- Android clang 9.0.3 (r353983c)

Target source family:

```
kernel-4.14/
    fs/f2fs/
    drivers/misc/mediatek/
    arch/arm64/
```

---

# Phase 1 completed

Recovered:

```
tran_f2fs_gc()       0x37ada8
f2fs_gc()            0x3503a8
vendor predicate     0x35cc18
post-GC policy       0x366cd4
```

Recovered:

- tran_gc subsystem relationships
- GC controller model
- threshold formulas
- segment policy branch locations
- post-GC policy flow
- F2FS statistics usage

---

# Important correction

Do not add fake vendor fields to struct f2fs_sb_info.

The following are existing F2FS accounting fields:

```
sbi+0x444
sbi+0x448
sbi+0x44c
sbi+0x450
sbi+0x454
sbi+0x458
sbi+0x45c
```

The vendor code consumes these counters but they are not Transsion-added fields.

---

# tran_gc controller reconstruction

Semantic layout:

```
+0x990 cycle counter
+0x998 state
+0x9d4 stop state
+0x9f8 result
+0x9fc result flags
+0xa04 cadence
+0xa08 progress baseline A
+0xa0c progress baseline B
```

Known objects:

```
tran_gc_wait_q
tran_gc_usb_wakelock
tran_gc_debug
```

---

# GC mode mapping

Recovered:

```
controller 0:
    normal f2fs_gc

controller 1:
    temporary gc_mode = 2

controller 2:
    temporary gc_mode = 3
```

---

# Current Phase 2 state

Completed:

- kernel version identification
- compiler/toolchain identification
- MediaTek BSP narrowing
- tran_f2fs_gc architecture recovery
- segment.c branch mapping
- gc.c vendor hook model

In progress:

## Source baseline matching

Need exact MediaTek/Transsion kernel-4.14 source tree.

Search anchors:

```
tran_gc_thread_func
tran_gc_wait_q
gc_urgent
gc_idle_interval
percent_of_free_segment
inc_gc_seg_threshold
dec_gc_seg_threshold
```

---

# Remaining work

## 1. Finish tran_f2fs_gc()

Need:

- instruction-level pseudocode
- exact f2fs_gc arguments
- locking behaviour
- restore path
- error handling

## 2. Recover controller lifecycle

Need:

- allocation
- initialization
- thread creation
- wake conditions
- teardown

## 3. Match source tree

Map recovered binary functions into:

```
fs/f2fs/gc.c
fs/f2fs/segment.c
fs/f2fs/f2fs.h
fs/f2fs/stat.c
```

## 4. Generate reconstruction patches

Target:

```
patches/
    tran-gc-controller.patch
    gc-policy.patch
    segment-policy.patch
    stat-control.patch
```

---

# Strict rules

- Binary evidence first.
- No invented recovered code.
- No invented struct names.
- Semantic reconstruction must be labelled.
- Source-identical claims require matching source proof.

---

# Current phase

```
Phase 1: COMPLETE
Phase 2: IN PROGRESS

Next target:
exact MediaTek/Transsion kernel tree matching.
```
