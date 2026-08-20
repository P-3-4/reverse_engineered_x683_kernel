# X683 BLR Analysis — 2026-08-20

## Fresh authoritative count

The recovered ARM64 Image contains exactly `11,692` `BLR` instructions inside symbol-derived function ranges.

## Conservative target reconstruction

For every BLR, the analysis window records the target register and inspects preceding instructions for:

- `ADRP` page construction
- `ADD/SUB` immediate address formation
- `LDR` unsigned-immediate loads
- literal loads
- static data/table addresses

`922` sites have a recoverable static `ADRP + ADD + LDR` chain feeding the BLR register. No site is labeled as an exact callback merely because a nearby data value or symbol name resembles a function pointer.

## Runtime-structure boundary

Many high-value callbacks load from runtime state such as F2FS `sbi`, MSDC host state, device-private state, scheduler policy state, or ops tables. Those targets cannot be recovered honestly from a single local BLR window when the containing pointer is runtime-initialized. Such sites remain explicit `opaque_ops_table` / unknown-field candidates.

## Priority domains

The next executable resolver should concentrate on BLR sites in functions containing `f2fs`, `gc`, `sit`, `nat`, `checkpoint`, `msdc`, `mmc`, `blk`, `bio`, `dma`, `ion`, `m4u`, `iommu`, `binder`, `sched`, `schedtune`, `ppm`, `cpufreq`, `thermal`, `pm`, `suspend`, `resume`, `wakeup`, `battery`, `charger`, `usb`, `dsi`, `display`, `mali`, `gpu`, `ilitek`, `touch`, `afe`, `snd`, `btif`, `wmt`, and `crypto`.

No indirect edge is marked resolved until executable data-flow supports the target function.
