# X683 Current State

## Verified executable baseline

The canonical branch is `kernel-reconstruction-current`. The supplied boot image remains the executable authority.

Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

Recovered DTB SHA-256: `de123d41bd398f20e97ecc01a21721437ee1698f9c1cbc178096946c4aedf1d6`

Validated metrics remain:
- kallsyms entries: 56,976
- function entries: 56,975
- unique kernel function starts: 52,784
- direct BL sites: 295,805
- mapped BL edges: 270,139
- exact symbol-start BL edges: 1,772
- BLR sites: 11,692

## BLR pass

Current evidence-only BLR state:

- Conservative BLR inventory: 922 candidates.
- ADRP + ADD + LDR -> BLR recheck: 49 chains.
- Exact known-function BLR targets promoted: 0.

Runtime-initialized ops/state pointers remain unresolved. No callbacks or ops tables are promoted without initializer provenance.

## F2FS state

Proven layouts remain:

- f2fs_sm_info = 0xA8
- sit_info = 0xA8
- free_segmap_info = 0x20
- dirty_seglist_info = 0x90
- curseg_info = 0x70
- curseg_info[6] = 0x2A0

Confirmed anchors:

- sm_info + 0x98 = flush_cmd_control
- sm_info + 0xA0 = discard_cmd_control
- discard_cmd_control size = 0x20B0

F2FS GC symbols remain confirmed in the supplied kallsyms including f2fs_gc and Transsion GC wrapper functions.

## Storage state

Confirmed:

- F2FS executable anchors.
- MMC request framework symbols.

Unresolved:

- F2FS -> bio -> block -> MMC exact chain.
- MSDC private structures.
- DMA state.
- IRQ/completion callback graph.

## Source baseline

Target remains:

- Linux 4.14.141+
- MT6768
- Android clang 9.0.3
- X683/H694 2021 firmware baseline

Exact vendor source revision is not proven. No unverified source imported.

## Next continuation target

1. Resolve runtime BLR targets through structure-base provenance.
2. Recover ops tables from registration paths.
3. Continue F2FS/block/MSDC callback graph recovery.
4. Match verified evidence against Transsion 4.14.141 source candidates.

Build integration remains blocked until source baseline and vendor paths are sufficiently recovered.
