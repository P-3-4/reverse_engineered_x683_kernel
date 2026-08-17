# X683/H694 F2FS GC Phase-4 Migration Reconstruction

Date: 2026-08-17
Branch: `reconstruction`

## Evidence policy

The stock X683/H694 `boot.img` / decompressed kernel is authoritative. Historical Linux 4.14 F2FS source is used only as a correspondence baseline. This document deliberately separates binary-backed conclusions from source-level reconstruction.

## Scope

This pass reconstructs the phase-4 data-migration portion of the stock `f2fs_gc()` path and its relationship to the historical `gc_data_segment()` / `move_data_block()` / `move_data_page()` machinery.

The compiler has inlined or partially merged several historical helper boundaries into the surrounding GC function. Therefore machine-code block boundaries must not be treated as proof of original source function boundaries.

## Confirmed overall architecture

The X683 data-GC path retains the canonical five-phase design:

```text
phase 0  NAT metadata preparation / readahead
phase 1  victim node readahead
phase 2  parent-node readahead / liveness preparation
phase 3  inode and data-page preparation
phase 4  actual data migration
```

The phase counter is incremented and compared against five in the stock binary. The five-phase structure is therefore directly established.

## Phase-4 control flow

The reconstructed logical flow is:

```text
GC inode list
    |
    +-- locate inode corresponding to summary entry
    |
    +-- regular-file GC locking
    |      |
    |      +-- acquire i_gc_rwsem[READ]
    |      +-- acquire i_gc_rwsem[WRITE]
    |      +-- inode_dio_wait()
    |
    +-- calculate start_bidx
    |
    +-- post-read required?
    |       |
    |       +-- yes -> block-level migration path
    |       |
    |       +-- no  -> page-level migration path
    |
    +-- migration result / submitted accounting
    |
    +-- release GC locks
    |
    +-- data statistics
```

This structure strongly corresponds to the 4.14 F2FS `gc_data_segment()` implementation.

## `ra_data_block()` helper

Target address:

```text
0x36bf78
```

Current classification: **HIGH confidence `ra_data_block()`-type helper**.

The call ABI is consistent with an inode plus block-index operation. It is used from the data-GC preparation path and performs the expected data-page/read preparation and writeback-related handling.

Do not yet claim byte-for-byte identity with a particular upstream revision. Keep the reconstructed source name as a provisional X683 mapping until the complete helper body is compared against the selected historical baseline.

## Data readahead loop

A nearby helper around `0x36c0d8` operates as an inode/block-index/count loop and invokes the data-block readahead operation for successive block indices. This reinforces the interpretation of `0x36bf78` as the data-block readahead primitive.

## Dnode / migration context

The phase-4 path constructs and carries a local dnode-like context containing information equivalent to:

```text
inode
node page
nid
node offset
block index
source/destination block information
```

The exact X683 layout must be reconstructed from all accesses rather than copied from an upstream structure.

## `0x355210`

Current classification: **HIGH confidence `move_data_block()`-type path**.

The call context supplies the data-migration context and block index. The function performs page writeback synchronization and continues into lower-level block replacement/allocation machinery.

Safest reconstructed naming:

```c
x683_move_data_block(...)
```

until the entire call graph is matched to the exact historical revision.

## `0x373dc4`

This helper consumes the migration context and reconstructs the actual data block position from the inode/node context. It then enters a larger write/replacement routine.

Its role is therefore part of the actual data-block migration engine rather than GC policy.

## `0x373e5c`

This is currently the deepest identified data-migration routine. It receives inode/block/migration state and enters the allocation/write/replace path.

Current interpretation:

```text
source data block
      |
      +-- validate mapping/state
      |
      +-- derive filesystem/block geometry
      |
      +-- prepare destination
      |
      +-- allocate/write/copy
      |
      +-- update parent node/dnode
      |
      +-- error/rollback cleanup
```

Status: **HIGH confidence role; exact source function identity and vendor delta still unresolved**.

The next binary pass should fully map this routine before assigning an exact upstream name.

## Writeback helper

Target:

```text
0x36bea0
```

Current classification: **CONFIRMED `f2fs_wait_on_page_writeback()` role**.

The argument pattern corresponds to:

```c
f2fs_wait_on_page_writeback(page, type, ordered, locked);
```

The GC migration path uses this before modifying/migrating relevant data pages.

## Post-read split

The X683 binary retains the historical conceptual split:

```c
if (f2fs_post_read_required(inode))
    err = x683_move_data_block(...);
else
    err = x683_move_data_page(...);
```

The exact source boundaries of `move_data_page()` are not yet proven because compiler inlining/merging obscures them.

## GC rwsem / skip behavior

For regular files, the phase-4 path contains GC-specific inode locking and waits for in-flight direct I/O before migration.

If the required GC semaphore cannot be obtained, the migration is skipped and the X683 skip-accounting path is reached.

This provides a direct behavioral connection between phase-4 migration and the previously recovered `skipped_gc_rwsem` SBI field at `sbi + 0x550`.

## Submission accounting

The X683 path tests migration success and updates the logical submitted count under the applicable GC mode/post-read conditions. It also updates data-block statistics.

The exact X683 `stat_info` member names must remain binary-defined; do not blindly apply upstream `stat_inc_data_blk_count()` field ordering.

## Vendor delta assessment

The current evidence does **not** support the claim that Transsion replaced the fundamental F2FS physical data-migration algorithm.

The stronger interpretation is:

```text
historical 4.14 F2FS migration engine
        +
X683/vendor statistics and state layout
        +
Transsion GC policy/wrapper
        +
additional accounting/skip handling
```

The core concepts still visible are:

- dnode-based migration
- inode GC semaphores
- direct-I/O waiting
- data-page writeback handling
- post-read aware migration
- destination block allocation/write
- parent-node update
- migration result accounting

## Current function map

| Address | Current mapping | Confidence |
|---|---|---|
| `0x35d31c` | `f2fs_get_node_info()` role | CONFIRMED/HIGH |
| `0x35dfb8` | `f2fs_get_node_page()` role | HIGH |
| `0x360c9c` | `f2fs_ra_node_page()` | CONFIRMED |
| `0x36130c` | `f2fs_move_node_page()` | CONFIRMED |
| `0x36bea0` | `f2fs_wait_on_page_writeback()` | CONFIRMED |
| `0x36bf78` | `ra_data_block()`-type helper | HIGH |
| `0x36c0d8` | data-block readahead loop | HIGH |
| `0x355210` | `move_data_block()`-type path | HIGH |
| `0x373dc4` | data-block migration preparation | HIGH role |
| `0x373e5c` | deep data allocation/write/replace engine | HIGH role; name unresolved |

## Reconstructed source-level skeleton

```c
/* Reconstructed behavior, not original vendor source. */

/* phase 4 */
inode = find_gc_inode(gc_list, dni.ino);
if (!inode)
    continue;

if (S_ISREG(inode->i_mode)) {
    if (!down_write_trylock(&F2FS_I(inode)->i_gc_rwsem[READ]))
        continue;

    if (!down_write_trylock(&F2FS_I(inode)->i_gc_rwsem[WRITE])) {
        sbi->skipped_gc_rwsem++;
        up_write(&F2FS_I(inode)->i_gc_rwsem[READ]);
        continue;
    }

    inode_dio_wait(inode);
    locked = true;
}

start_bidx = f2fs_start_bidx_of_node(nofs, inode) + ofs_in_node;

if (f2fs_post_read_required(inode))
    err = x683_move_data_block(inode, start_bidx,
                               gc_type, segno, off);
else
    err = x683_move_data_page(inode, start_bidx,
                              gc_type, segno, off);

if (!err && (gc_type == FG_GC ||
             f2fs_post_read_required(inode)))
    submitted++;

if (locked) {
    up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
    up_write(&F2FS_I(inode)->i_gc_rwsem[READ]);
}

/* X683-specific statistics follow the migration result. */
```

This is a behavioral reconstruction only. It is not claimed to be the original Transsion source text.

## Sanity checks passed

1. The phase-4 path is reached only after the phase-3 inode/data preparation path.
2. The same inode/NID relationship used by the earlier `is_alive()` logic feeds phase 4.
3. The GC rwsem skip path is consistent with the independently recovered `sbi + 0x550` accounting field.
4. `0x36bea0` has the expected page/writeback helper role.
5. `0x36bf78` is called from the data preparation path, consistent with data-block readahead.
6. The migration path remains compatible with the historical five-phase F2FS design.
7. No unresolved structure offset is promoted to an exact C member solely because an upstream 4.14 structure has a similar layout.

## Remaining work

The next target is a complete instruction-level reconstruction of `0x373e5c`, followed by the lower-level destination-block allocation/write and parent-node update calls it invokes. That pass must answer:

1. exact source-block validation;
2. exact destination allocation helper;
3. exact page copy/write operation;
4. exact node/dnode update;
5. error/rollback behavior;
6. any Transsion-specific branches;
7. exact statistics updates caused by migration.

Only after those are mapped should `x683_move_data_block()` be promoted from a type/role reconstruction to an exact source-level reconstruction.
