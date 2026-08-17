# Source reconstruction manifest

## Target

Stock X683/H694 kernel fingerprint:

- Linux 4.14.141+
- Android clang 9.0.3
- Build date: 2021-11-05
- F2FS compiled into the kernel
- `CONFIG_F2FS_TRAN_GC=y`

## Public references

The reconstruction uses public Linux/Android Common 4.14 F2FS and public MT6768 kernel trees as comparison material. These are references, not claimed to be the original Transsion source.

Primary F2FS comparison reference used during layout work:

- Android Common `upstream-f2fs-stable-linux-4.14.y`
- MT6768-Lab `android_kernel_xiaomi_mt6768`

## Rule

Do not replace a recovered X683 field/member with a newer upstream name merely because the semantic meaning looks similar. First establish structural correspondence using offsets, access patterns, symbol boundaries, and source history.

## Reconstruction status

- F2FS structure mapping: in progress
- `f2fs_gc()`: core control flow reconstructed; binary/vendor delta still being mapped
- `do_garbage_collect()`: inlined structure reconstructed
- `gc_data_segment()`: five-phase structure reconstructed
- Phase-4 data migration: high-confidence control flow reconstructed; deepest write/replace engine still under instruction-level analysis
- Transsion GC: behaviorally reconstructed; not verbatim vendor source
- MT6768 hardware drivers: pending integration
- X683/H694 DTS: pending reconstruction
- Exact stock-equivalent build: not yet achieved

## New phase-4 reconstruction

See `f2fs-gc-phase4-migration.md` for the binary-backed phase-4 migration map, helper identities, confidence levels, and remaining work around `0x373e5c`.
