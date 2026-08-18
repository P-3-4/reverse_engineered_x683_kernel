# X683/H694 GC reconstruction — full sanity check

Date: 2026-08-18

## Verdict

The reverse-engineering findings are substantially useful, but the previous `gc-five-pass-final.md` wording overstated completion. The repository contains a strong binary-derived reconstruction/scaffold, **not yet a buildable or byte-accurate replacement**.

## Confirmed after re-check

- Boot SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.
- Decompressed Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`.
- Stop-1/2/3 condition slots and Stop-4/5 result/controller writes remain the strongest recovered state-machine facts.
- Controller 0/1/2 maps to normal / temporary `gc_mode` 2 / temporary `gc_mode` 3 in the wrapper reconstruction.
- The historical F2FS `f2fs_gc(sbi, sync, background, segno)` ABI and caller-owned GC mutex model remain valid.
- The three named vendor controls are proven registrations/descriptors; they are not proven standalone implementation symbols.

## Problems found and corrected

### 1. Fake `msecs_to_jiffies()` arithmetic

`tran_gc_thread_reconstructed.c` previously contained `(timeout_ms + 3) >> 2` as a placeholder for `0xce58c`. That is not a valid general implementation of `msecs_to_jiffies()` and was removed. The reconstructed scheduler/wait path now explicitly leaves the kernel conversion/integration unresolved.

### 2. Unproven dirty-counter sum

The thread scaffold previously summed six words at `dirty_info + 0x68`. The exact semantic mapping of that region was not strong enough to make this a source-level fact. The scaffold now accepts `recoverable_segments` explicitly instead of baking in the uncertain sum.

### 3. Thread reconstruction was overstated

The thread source is a detector-step scaffold, not the complete proprietary kthread. The exact wait/re-entry/abort branches and vendor predicate producers are still unresolved.

### 4. Wrapper/source integration is not build-proven

The reconstructed files use vendor-divergent structure offsets and helper names. They have not been compiled against the exact X683 4.14 kernel headers/source tree. They must therefore not be treated as drop-in kernel source yet.

### 5. Historical delta is architectural, not exhaustive

The stock/vendor separation is reliable at the architecture level, but the repository has not yet produced a complete line-by-line X683-vs-stock `gc.c` patch.

## Remaining blockers

1. Exact callback bodies/backing fields for `need_switch_ssr`, `tran_urgent_gc`, and `detect_charger_type`.
2. `tran_gc_usb_wakelock` implementation path.
3. Exact semantics of `0xcc774`, `+0x974`, and controller-object `+0x20`.
4. Exact state-3 scheduler/wait/re-entry sequence.
5. Exact mapping of the vendor `stat_info` members.
6. Full exact stock-X683 `gc.c` differential.
7. Compilation against the real X683/H694 4.14 source tree.

## Correct project status

- Binary understanding: high confidence for the recovered GC state machine and wrapper.
- Source reconstruction: substantial scaffold, not final.
- Buildability: **not established**.
- Replacement-kernel readiness: **not established**.

This document supersedes any wording that calls the five-pass reconstruction "complete" without these qualifications.
