# X683 Reconstruction Coverage Report — 2026-08-19

| metric | value | interpretation |
|---|---:|---|
| Function mapping coverage | **100.0%** | every unique executable kallsyms address indexed |
| Direct-call control-flow coverage | **84.97%** | 44,852 / 52,784 executable functions have an exact-symbol ARM64 BL edge |
| New reconstructed C artifacts | **13** | binary-backed source models added this pass |
| New unique model functions | **78** | functions in the new source artifacts |
| Embedded source/header paths | **619** | compiler-path artifacts recovered from Image strings |
| F2FS source/header paths | **14** | direct F2FS build-surface evidence |

## Subsystem coverage

**F2FS — deep:** GC/victim/migration was already complete. This pass adds checkpoint, segment allocation, node/NAT, data, recovery, discard, shrinker and sysfs integration.

**Storage — substantial:** X683 is eMMC/MSDC. Probe, MMC request, CMDQ, DMA, IRQ, tuning and PM paths are mapped.

**MM — substantial mapping / partial source reconstruction:** kswapd, reclaim, shrinkers, OOM, PSI, ION and M4U are identified. Vendor low-memory hinting is proven; classic LMK is not.

**Scheduler/power — substantial mapping:** schedtune, PPM, cpufreq, cpuidle, EEM, PBM, SPM and thermal callbacks are identified, including battery/display policy edges.

**Display — integration mapped:** framebuffer notifier registration, Transsion GC wakeup, PPM display-off and GED DVFS callbacks are connected.

**Battery/charger — integration mapped:** MT6358 gauge, charger detection, power-supply updates, USB charger events and Transsion battery probe are connected.

**Binder/ION — integration mapped:** Binder transaction/allocator and ION allocation/shrinker are connected to Android memory pressure.

## Overall

The requested **80–90% meaningful source reconstruction target has not been reached**. No inflated percentage is claimed.

- Function mapping: **100%**
- Direct-call graph coverage: **84.97%**
- Source-path inventory: **619 artifacts**
- New source reconstruction: **13 files / 78 unique model functions**
- Actual source reconstruction: **substantially below 80–90%**

Kallsyms mapping and direct BL coverage do not prove indirect function-pointer calls, exact structure names or source equivalence. Those remain explicit work items.
