# X683 Reconstruction Coverage Report — 2026-08-20

## Fresh authoritative executable measurements

| metric | measured value | meaning |
|---|---:|---|
| Kallsyms entries | 56,976 | all supplied records parsed from `x683_kallsyms.txt` |
| function-symbol entries | 56,975 | kallsyms records of function-bearing types |
| unique kernel function starts | 52,784 | unique function starts inside recovered Image range |
| direct BL instruction sites | 295,805 | direct branch instructions inside function ranges |
| mapped direct BL edges | 270,139 | BL targets landing inside known function ranges |
| exact symbol-start BL edges | 1,772 | strongest exact-start direct edges under the current definition |
| functions with mapped direct BL | 35,034 / 52,784 | **66.3724%** caller-side direct-BL coverage |
| indirect BLR instructions | 11,692 | indirect dispatch sites |
| static ADRP+ADD+LDR BLR candidates | 922 | conservative static/table-backed candidates |
| exact targets resolved from that static pattern | 0 | none promoted without stronger evidence |
| reconstruction C units | 21 | existing evidence-backed C models retained |

The fresh BLR count exactly matches the prior executable-level count. The older exact-start/direct-call figures are not mixed with this measurement definition; the current report is based on a single reproducible scan of the recovered Image.

## Executable recovery

The authoritative Image is now directly available from the supplied boot container:

- boot header v2, 2048-byte pages
- gzip member length `9,640,652`
- appended DTB size `114,696`
- decompressed Image size `26,615,820`
- Image SHA-256 `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

## Subsystem inventory

The repository's subsystem counts remain lexical triage rather than source-equivalence. Existing F2FS/storage/MM/ION/Binder/scheduler/PPM/thermal/PM/display/battery/input/audio/network/security mappings are preserved and can now be cross-checked against the recovered executable.

## Reconstruction coverage

F2FS private layouts, Transsion GC policy/state machine, victim/migration boundary, checkpoint/segment/node/data/recovery models, MSDC/storage state model, reclaim/shrinker model, ION/M4U/Binder model, scheduler/PPM/thermal model, PM/display/battery/input/audio/network/security evidence models remain present.

No whole-kernel source-equivalence percentage is reported. Binary source-path strings are evidence of original build inputs, not proof that a reconstructed file is source-identical.

## Build gate

- complete 4.14.141 vendor source tree: **NO**
- `make olddefconfig`: **NOT RUN**
- `make prepare`: **NOT RUN**
- `make modules_prepare`: **NOT RUN**
- `make Image`: **NOT RUN**
- modules build: **NOT RUN**

## Boot/functionality gate

- replacement kernel boot: **NOT TESTED**
- Android userspace boot: **NOT VERIFIED**
- storage/display/touch/USB/audio/Wi-Fi/Bluetooth/battery/suspend functional tests: **NOT VERIFIED**

## Current reconstruction blockers

1. Runtime BLR/ops-table resolution and private structure field recovery.
2. Exact historical Transsion/X683 4.14.141 vendor source revision.
3. Missing runtime module binaries, especially WLAN/WMT/FPSGO.
4. Genuine source-tree integration and ARM64 build transition.
