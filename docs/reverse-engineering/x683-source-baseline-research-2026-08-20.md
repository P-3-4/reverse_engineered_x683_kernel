# X683 Source Baseline Research — 2026-08-20

## Authoritative compiler/source identity

The recovered X683 Image contains source path strings under `kernel-4.14/` and the complete build identity `4.14.141+`, Android clang 9.0.3 / LLVM 9.0.3svn, build date `Fri Nov 5 15:56:25 CST 2021`.

At least `862` `kernel-4.14` path occurrences are present in the Image. Examples include `init/main.c`, `arch/arm64/kernel/suspend.c`, `arch/arm64/mm/dma-mapping.c`, `kernel/sched/fair.c`, `kernel/sched/tune.c`, `kernel/power/process.c`, and `drivers/mmc/core/core.c`.

## External source candidates

Public MT6768 4.14 repositories exist and are useful correlation baselines, but no exact Transsion/X683 vendor Git revision has been proven from the current evidence. Public MT6768 downstream trees can support API and structure comparison, but must not be imported as the X683 source baseline without binary correlation.

## Current decision

Status: **reference baseline only; exact vendor revision unresolved**.

Required proof for a true import remains a combination of matching source paths, function signatures, structure layout, vendor macros/deltas, DT bindings, symbol ordering, strings, compiler artifacts, and X683-specific modifications.
