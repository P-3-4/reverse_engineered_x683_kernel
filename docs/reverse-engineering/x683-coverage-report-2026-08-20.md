# X683 Reconstruction Coverage Report — 2026-08-20

## Measured binary inventory

| metric | measured value | meaning |
|---|---:|---|
| Kallsyms entries | 56,975 | all supplied symbol records |
| unique executable Image addresses | 52,784 | kernel functions/symbol addresses in Image range |
| module symbol entries | 3,679 | runtime modules outside supplied Image |
| embedded source/header paths | 540 | cleaned `kernel-4.14/...` paths recovered from Image strings |
| mapped direct BL instructions | 270,108 | BLs landing inside a known executable symbol range |
| exact symbol-start BL edges | 4,133 | strongest direct call edges |
| functions with mapped direct BL | 35,080 / 52,784 | **66.46%** caller-side direct-BL coverage |
| target functions reached by mapped direct BL | 9,429 | containing-symbol target approximation |
| indirect BLR instructions | 11,692 | unresolved indirect dispatch sites requiring data-reference analysis |
| reconstruction units after this pass | 21 | 13 existing + 8 new binary-backed C units |

The earlier 84.97% direct-call figure is retired. It conflated broader branch-target heuristics with exact function-level direct-call coverage. The 66.46% number above is calculated directly from the supplied Image and kallsyms.

## Subsystem inventory

| subsystem | executable functions | direct-BL callers | caller coverage |
|---|---:|---:|---:|
| arch/IRQ | 1,433 | 942 | 65.73% |
| audio | 1,036 | 720 | 69.50% |
| battery/USB | 6,288 | 4,080 | 64.89% |
| display/GPU | 1,529 | 1,163 | 76.06% |
| F2FS | 701 | 452 | 64.48% |
| input/sensors | 525 | 378 | 72.00% |
| MM | 2,290 | 1,615 | 70.52% |
| networking | 1,239 | 543 | 43.83% |
| other | 28,892 | 19,109 | 66.14% |
| power | 2,235 | 1,465 | 65.55% |
| scheduler | 2,061 | 1,356 | 65.79% |
| security/crypto | 1,328 | 896 | 67.47% |
| storage | 2,194 | 1,642 | 74.84% |
| thermal/DVFS | 963 | 672 | 69.78% |

Classification is lexical triage and must not be interpreted as source reconstruction.

## Reconstruction coverage

### F2FS

Deep binary reconstruction already established before this pass is preserved: `f2fs_sm_info`, `sit_info`, `free_segmap_info`, `dirty_seglist_info`, `curseg_info[6]`, flush/discard control, four-argument `f2fs_gc`, stock victim/migration boundary, and Transsion GC policy/state machine.

The previous pass also added binary-backed checkpoint, segment, node/NAT, data, recovery, discard, shrinker and sysfs source models. This pass did not reopen proven GC/victim work.

### Storage

The X683 DT, config, kallsyms and direct disassembly now establish the two enabled MSDC nodes, request path entry points, DMA/IRQ/tuning/CQ/crypto hooks and system/runtime PM entry points. Indirect callback containers and several private host-state fields remain unresolved.

### MM / Android memory

The actual X683 reclaim/kswapd/OOM/PSI symbol surface and ION/M4U/Binder shrinker paths are mapped. Configuration proves the enabled feature boundary. Exact private ownership/state transitions remain partially indirect.

### Scheduler / PPM / thermal / power

schedtune, PPM, cpufreq, cpuidle, EEM/PBM, thermal and GPU PM surfaces are mapped with exact X683 addresses. Callback/container layouts remain partially unresolved.

### Display / battery / USB

The prior binary-backed display/GPU and MT6358/charger/Transsion battery integration remains intact. This pass adds stronger DT binding evidence and exact symbol inventories but does not claim generic source equivalence.

### Input / audio / network / security

The actual X683 Ilitek/TPD, AFE/MT6768 audio, WMT/WLAN module and BTIF surfaces are identified. Security/crypto is constrained by the actual config and source-path fingerprint. Module binaries are missing, so module disassembly is explicitly not claimed.

## Source reconstruction coverage

A trustworthy whole-kernel source-file percentage cannot be derived from the binary because there is no DWARF/source-map association between every symbol and original source path. The recovered source-path list is evidence of build inputs, not proof that a reconstructed C file is equivalent to that path.

Therefore this pass reports:

- **21 reconstruction C units** in the repository.
- **78 model functions** in the 13 pre-existing binary-backed source artifacts.
- **8 additional evidence-backed C units** added in this pass.
- **No inflated whole-kernel source-equivalence percentage.**

## Overall conclusion

The binary/function inventory is now complete for the supplied Image, the direct-BL graph has a reproducible measured baseline, the DT from the boot image has been recovered despite the incomplete standalone DT artifact, and the highest-value storage/MM/scheduler/power/input/audio/security surfaces have been connected.

The project remains a reverse-engineered reconstruction rather than a source-identical rebuild. Indirect dispatch, private structure naming, missing module binaries, and exact historical vendor revision remain the dominant blockers.
