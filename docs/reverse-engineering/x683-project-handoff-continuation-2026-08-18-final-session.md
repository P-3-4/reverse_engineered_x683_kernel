# X683/H694 Kernel Reverse Engineering — Final Session Continuation Handoff

Date: 2026-08-18

## Session summary

This continuation started from the canonical handoff:

`docs/reverse-engineering/x683-project-handoff.md`

The goal was completing Phase 1 before moving to Phase 2 (kernel tree reconstruction).

Important correction:

Do not treat inferred/reconstructed code as recovered proprietary source. Binary evidence is authoritative.

---

# Current project state

## Completed/high confidence

### Boot/kernel evidence

Stock `boot(8).img` and kernel Image are the binary authority.

Confirmed:

- ARM64 Android kernel
- kernel-4.14 F2FS lineage
- X683/H694 target
- vendor F2FS GC additions exist in the binary

Embedded F2FS paths confirmed:

```
fs/f2fs/f2fs.h
fs/f2fs/gc.c
fs/f2fs/segment.c
fs/f2fs/segment.h
fs/f2fs/data.c
fs/f2fs/node.c
fs/f2fs/checkpoint.c
```

---

# Transsion GC subsystem findings

Confirmed vendor artifacts:

```
tran_gc
tran_f2fs_gc
tran_gc_thread_func
tran_gc loop static detect
tran_gc_debug
tran_gc_wait_q
tran_gc_usb_wakelock

emmc_gc_time
ssr_gc_times
gc_skip_times
gc_to_static_detect_times

.gc_urgent
.gc_urgent_sleep_time
gc_min_sleep_time
gc_max_sleep_time
gc_no_gc_sleep_time
gc_idle_interval

inc_gc_seg_threshold
dec_gc_seg_threshold
percent_of_free_segment
```

Interpretation:

Transsion GC is not only a wrapper around F2FS. It contains:

```
GC kernel thread
static detection loop
eMMC lifetime awareness
threshold control
fragmentation handling
F2FS GC invocation layer
```

---

# Confirmed GC architecture

```
Transsion detector/thread
        |
        v
tran_f2fs_gc()
        |
        v
X683 F2FS GC entry
        |
        v
victim selection
        |
        v
migration/reclaim
        |
        v
vendor policy/statistics
```

---

# Important corrections

Avoid previous overstatements:

Not yet proven:

- exact tran_gc_* xrefs
- exact vendor-added f2fs_sb_info offsets
- exact gc.c/segment.c branch deltas
- exact threshold formulas
- final compilable vendor patch

These require instruction-level binary tracing.

---

# Phase 1 status

```
GC entry mapping              complete
GC wrapper understanding      complete
Vendor GC discovery           complete
GC thread discovery           complete
Vendor parameter discovery    complete

Binary xref mapping           remaining
SBI field recovery            remaining
gc.c delta recovery           remaining
segment.c delta recovery      remaining
Compilable patch generation   remaining
```

---

# Next continuation task

Continue from binary analysis, not assumptions:

1. Extract ARM64 disassembly.
2. Map every tran_gc_* string/object xref.
3. Recover function boundaries.
4. Recover SBI structure offsets from LDR/STR accesses.
5. Diff against Android 4.14 F2FS source.
6. Generate a real vendor F2FS patch set.

Do not create fake source-level recovery.

---

# User workflow preference

For future sessions:

- no progress updates
- no plans before work
- no meta commentary
- output finalized findings only
- clearly separate confirmed facts from inference
