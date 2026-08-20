# X683 Kernel Reverse Engineering — Project Handoff

## Canonical state

- Repository: `P-3-4/reverse_engineered_x683_kernel`
- Branch: `kernel-reconstruction-current`
- Current pre-pass commit: `98bfea9bd077cb80abe54fbb8d6dd971d5ea2086`
- Its parent: `75273606d654df536f61f5879ae981d4dcf1e7f2`
- Target: Infinix X683 / MT6768 / ARM64 / Linux `4.14.141+`

## This pass

The supplied boot image, kallsyms and config were re-analyzed from the actual local artifacts. The standalone DT archive was verified incomplete, and the full DT table was recovered from the boot image instead of being inferred.

### New persistent analysis

- 52,784 unique executable Image addresses inventoried.
- 56,975 kallsyms entries classified; 3,679 are runtime module symbols outside the Image.
- 270,108 direct BL instructions mapped into executable symbol ranges.
- 4,133 exact symbol-start BL edges.
- 35,080 / 52,784 functions have at least one mapped direct BL: 66.46%.
- 11,692 indirect BLR sites identified.
- 540 cleaned `kernel-4.14/...` source/header paths recovered from Image strings.
- Boot-image DT parsed: 542 nodes, 382 with `compatible` and/or `reg`.
- 8 additional binary-backed C evidence units added.

### Existing F2FS work preserved

The proven F2FS layouts and Transsion GC boundary remain canonical. The four-argument `f2fs_gc` ABI and temporary `gc_mode` behavior are not reopened.

### New source units

- `reconstructed/drivers/mmc/host/mediatek/ComboA/mt6768/x683_msdc_state.c`
- `reconstructed/mm/x683_reclaim_shrinkers.c`
- `reconstructed/drivers/android/x683_ion_m4u_binder.c`
- `reconstructed/kernel/sched/x683_sched_ppm_thermal.c`
- `reconstructed/drivers/misc/mediatek/base/power/x683_power_pm_reconstructed.c`
- `reconstructed/arch/arm64/kernel/x683_dt_driver_init.c`
- `reconstructed/drivers/input/touchscreen/x683_touch_audio_net.c`
- `reconstructed/security/x683_security_crypto.c`

These are evidence-backed reconstruction units, not fake compile-complete replacements. Unknown structures and indirect calls remain explicit.

## Persistent analysis artifacts

The repository keeps compact reproducible summaries and the analysis generator. The full generated JSONL inventory/callgraph can be regenerated from the supplied Image and kallsyms rather than being silently truncated into Git.

- `analysis/x683-measured-coverage.json`
- `analysis/x683-selected-callgraph-summary.json`
- `analysis/x683-source-paths.txt`
- `analysis/x683-config-selected.txt`
- `analysis/x683-dt-nodes.json`
- `analysis/x683-dt-summary.json`
- `analysis/x683-module-symbol-inventory.tsv`
- `tools/rebuild_x683_analysis.py`

## Current high-value remaining work

1. Resolve indirect callback/ops tables by data-reference analysis.
2. Recover exact private MSDC host/request/CQ state fields and error transitions.
3. Resolve ION/M4U/Binder ownership and callback structures.
4. Recover exact PPM/schedtune client structures and thermal limit callback data.
5. Correlate initcall tables to DT probe ordering.
6. Recover missing runtime module binaries for WLAN/WMT/FPSGO and disassemble them.
7. Determine the exact historical vendor source revision; `kernel-4.14` + `4.14.141+` + clang 9.0.3 is strong but not an exact git revision.
8. Continue exact F2FS adjacent `f2fs_sb_info`/vendor-state field naming only where new binary evidence is available.

## Evidence discipline

HIGH = direct binary/DT/kallsyms/config proof.
MEDIUM = binary plus historical-source correlation.
LOW = inference.

Binary wins over historical source. Unknowns stay offset-backed. Module symbols without module binaries are inventory-only.
