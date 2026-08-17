# X683/H694 — `0x373e5c` Deep Phase-4 Migration Reconstruction

## Scope

Target: exact stock X683/H694 ARM64 Image extracted from `boot.img`.

Primary function under analysis:

    0x373e5c

This pass treats the stock binary as authoritative and uses historical 4.14 F2FS `gc.c` only as a semantic/control-flow comparison.

## Directly established

`0x373e5c` begins with a normal AArch64 function prologue, saves a large register set, and constructs a substantial local context. It is a real migration helper, not a thin veneer.

Direct branch/call targets observed in the analyzed range:

    0x373f5c -> BL 0x00e0a490
    0x373fcc -> BL 0x00372ce8
    0x3740b4 -> BL 0x00374700
    0x3740cc -> BL 0x00df8568
    0x374118 -> BL 0x00e09ebc

Local forward branch:

    0x374110 -> 0x3741a8

The targets outside the local GC cluster are shared kernel helpers until their own bodies are independently identified.

## Function role

The call-site context established in the X683 phase-4 path makes `0x373e5c` a high-confidence component of the `move_data_block()` migration machinery. It is reached after dnode/data context preparation and data-page writeback handling.

It is not yet declared byte-for-byte identical to upstream `move_data_block()`.

## Historical fingerprint

Historical 4.14 F2FS `move_data_block()` follows this logical sequence:

    build f2fs_io_info
    prepare/lock source page
    validate source block
    initialize/check dnode
    obtain summary information
    serialize I/O in LFS mode when required
    allocate destination block
    obtain target META_MAPPING page
    wait on target-page writeback
    copy source data to target page
    invalidate old mapping/cache as required
    mark target dirty/writeback
    submit write
    handle retry/error
    update dnode's data block address
    update inode flags when applicable
    release pages/context
    recover destination allocation on failure
    release ordering lock/dnode

The X683 surroundings and deep helper structure are consistent with this migration pipeline. Historical F2FS source confirms the same destination-allocation, target-page, write, dnode-update, and recovery sequence. citeturn642185search2turn642185search5

## X683 call-target conclusions

### `0x00e0a490`

Shared kernel helper called from inside `0x373e5c`. Its role is consistent with destination data-block allocation/allocation-related work, but the exact source identity is not yet proven from this call site alone.

Status: **HIGH-CONFIDENCE ROLE, EXACT NAME UNRESOLVED**.

### `0x00372ce8`

Local helper in the nearby GC/migration cluster. It participates in the same migration path.

Status: **HIGH-CONFIDENCE GC/MIGRATION HELPER, EXACT NAME UNRESOLVED**.

### `0x00374700`

Nearby local helper reached late in the routine. It may be completion/cleanup or a local migration subroutine; its exact role remains unresolved.

Status: **UNRESOLVED**.

### `0x00df8568`

Shared helper outside the local GC cluster. Historical `move_data_block()` uses shared helpers for page I/O, data mapping, dnode updates and block replacement; this target requires independent body matching.

Status: **UNRESOLVED**.

### `0x00e09ebc`

Shared helper reached late in the routine. It may belong to write/submission/cache/recovery handling, but the call site alone does not establish its identity.

Status: **UNRESOLVED**.

## What remains hypothesis

Do not yet promote these names to exact X683 identities solely from historical correspondence:

    f2fs_allocate_data_block(...)
    f2fs_pagecache_get_page(META_MAPPING(...), ...)
    f2fs_submit_page_write(...)
    f2fs_do_replace_block(...)
    f2fs_update_data_blkaddr(...)

The historical source contains these logical operations in this migration path, but their precise X683 call boundaries must be proven by reversing the five target helpers.

## Vendor-delta status

No direct evidence from `0x373e5c` alone proves that Transsion rewrote the fundamental physical block-replacement algorithm. Current evidence still favors:

    standard 4.14 F2FS migration engine
        + X683 structure/layout changes
        + X683 GC statistics/accounting changes
        + Transsion GC wrapper/policy changes

A vendor delta inside `0x373e5c` remains possible around allocation mode, recovery, I/O accounting, or destination-cache handling, and will be tested by the next helper-level pass.

## Next exact targets

Reverse independently:

    0x00e0a490
    0x00372ce8
    0x00374700
    0x00df8568
    0x00e09ebc

Priority:

1. `0x00e0a490` — likely destination allocation path
2. `0x00df8568` — likely write/cache/block helper
3. `0x00e09ebc` — likely late write/recovery helper
4. `0x00372ce8` — local migration helper
5. `0x00374700` — local completion/cleanup helper

## Confidence rule

No helper above receives an exact C function name until its own binary body and ABI independently support the identity. Historical F2FS remains a fingerprint, not proof.
