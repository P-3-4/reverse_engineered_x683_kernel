# X683 Kernel Reverse Engineering — Project Handoff

## Canonical state

- Repository: `P-3-4/reverse_engineered_x683_kernel`
- Branch: `kernel-reconstruction-current`
- Previous tip: `0d2d768c3b515341b011d66ec6f2a0f273031422`
- Previous parent: `d5f9390a93f87f5df4db87609d57de2632b3c612`
- Target: Infinix X683 / MT6768 / ARM64 / Linux `4.14.141+`.
- Authoritative artifacts: supplied X683 boot image, kallsyms, config and recovered DTB.

## Executable authority revalidated

This pass independently decompressed the supplied boot image and recomputed the executable metrics against the actual recovered Image.

- decompressed Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`
- Image size: `26,615,820`
- recovered DTB SHA-256: `de123d41bd398f20e97ecc01a21721437ee1698f9c1cbc178096946c4aedf1d6`
- kallsyms entries: `56,976`
- function entries: `56,975`
- unique kernel function starts: `52,784`
- direct BL sites: `295,805`
- direct BL mapped edges: `270,139`
- exact symbol-start BL edges: `1,772`
- direct-call callers: `35,034`
- direct-call caller coverage: `66.3723856%`
- BLR sites: `11,692`

These values reproduce the previous executable-level measurements; they are not carried forward as unverified historical numbers.

## BLR pass

The existing conservative static inventory contains `922` BLR candidates. An independent simple `ADRP + ADD + LDR -> BLR` recheck found `49` such chains in the recovered Image and `0` exact known-function targets. No indirect callback was therefore promoted by this pass.

Runtime-initialized ops/state pointers remain explicit unresolved evidence.

## F2FS state

The proven layout anchors remain:

- `f2fs_sm_info = 0xA8`
- `sit_info = 0xA8`
- `free_segmap_info = 0x20`
- `dirty_seglist_info = 0x90`
- `curseg_info = 0x70`
- `curseg_info[6] = 0x2A0`
- `sm_info + 0x98 = flush_cmd_control`
- `sm_info + 0xA0 = discard_cmd_control`
- `discard_cmd_control = 0x20B0`

The proven four-argument `f2fs_gc` ABI remains authoritative. Known Transsion GC symbols are present in the supplied kallsyms.

## Build identity and source baseline

The Image contains Linux `4.14.141+`, Android clang `9.0.3`, LLVM `9.0.3svn`, and build timestamp `Fri Nov 5 15:56:25 CST 2021`. The `X683-H694EFGHIJUW-Q-OP-211105V361` firmware release is a high-confidence release target, but the exact vendor Git revision is not proven.

Public MT6768 4.14 trees remain correlation references only. No unverified source has been imported.

## Build gate

- Complete exact X683/Transsion 4.14.141 source tree: **not recovered**.
- `make olddefconfig`: **not run**.
- `make prepare`: **not run**.
- `make modules_prepare`: **not run**.
- `make Image`: **not run**.
- `make dtbs`: **not run**.
- replacement boot: **not tested**.
- Android userspace boot: **not verified**.
- hardware functionality: **not verified**.

## Immediate blockers

1. Resolve runtime BLR targets through structure-base and initializer provenance.
2. Match the exact Transsion/X683 vendor source revision.
3. Recover vendor/module code outside the built-in Image.
4. Only after those gates, import a complete source baseline and begin ARM64 build integration.

Unknowns remain explicit and are never promoted from inference to proof.
