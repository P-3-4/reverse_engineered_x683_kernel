# X683 Current State

## Verified executable baseline

The canonical branch is `kernel-reconstruction-current`. Its current tip before this pass was `0d2d768c3b515341b011d66ec6f2a0f273031422`, parent `d5f9390a93f87f5df4db87609d57de2632b3c612`.

The supplied boot image was independently reprocessed in this pass. The decompressed Image SHA-256 is `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`, size `26,615,820`; the recovered DTB SHA-256 is `de123d41bd398f20e97ecc01a21721437ee1698f9c1cbc178096946c4aedf1d6`, size `114,696`.

Recomputed executable metrics exactly reproduce the existing authoritative counts: 56,976 kallsyms entries, 56,975 function entries, 52,784 unique kernel function starts, 295,805 direct BL sites, 270,139 mapped BL edges, 1,772 exact symbol-start BL edges, 35,034 direct-call callers, 66.3723856% caller coverage, and 11,692 BLR sites.

## BLR pass

A conservative simple `ADRP + ADD + LDR -> BLR` recheck found 49 directly reconstructable chains in the supplied Image. None yielded an exact known kernel function address. Therefore no new indirect callback was promoted to exact/high-confidence status by this pass.

The broader existing conservative inventory remains 922 candidate static BLR sites. Runtime-initialized ops/state pointers remain unresolved.

## Structures

The six proven F2FS layout values remain unchanged: `f2fs_sm_info=0xA8`, `sit_info=0xA8`, `free_segmap_info=0x20`, `dirty_seglist_info=0x90`, `curseg_info=0x70`, and `curseg_info[6]=0x2A0`. `sm_info+0x98` and `sm_info+0xA0` remain the proven flush/discard control locations, with discard control size `0x20B0`.

## Build gate

No complete X683/Transsion 4.14.141 source baseline has been proven. Consequently the ARM64 build gates remain intentionally unrun rather than populated with an unverified source tree.

## Highest-value next evidence

1. Resolve runtime-initialized BLR targets by following structure-base provenance across callers and initializers.
2. Match the exact Transsion vendor source revision associated with the 2021-11-05 X683 build.
3. Recover missing vendor/module source or binaries outside the built-in Image.
4. Only then transition the verified baseline into a complete kernel build tree.
