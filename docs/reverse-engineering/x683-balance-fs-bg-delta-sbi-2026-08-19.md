# X683 `f2fs_balance_fs_bg()` Delta and `f2fs_sb_info` Recovery — 2026-08-19

## Scope

This document separates the stock 4.14-era F2FS `f2fs_balance_fs_bg()` path from X683/Transsion-specific logic using the shipped X683 Image and `x683_kallsyms.txt`.

Binary evidence remains authoritative. Public F2FS source is comparison evidence only.

## Exact X683 symbol facts

```text
Image+0x366cd4 = f2fs_balance_fs_bg
Image+0x3503a8 = f2fs_gc
Image+0x37ada8 = tran_do_f2fs_gc
Image+0x35cc18 = f2fs_available_free_memory
Image+0x35d22c = f2fs_try_to_free_nats
Image+0x362c40 = f2fs_build_free_nids
Image+0x363288 = f2fs_try_to_free_nids
Image+0x373108 = f2fs_shrink_extent_tree
Image+0x34e224 = f2fs_sync_dirty_inodes
Image+0x341250 = f2fs_sync_fs
```

## Stock-equivalent portion

The following X683 instructions map directly onto the normal 4.14 F2FS balance/background-cleanup sequence:

```text
0x366cf4  SBI/POR guard: sbi+0x48 bit3
0x366cfc  f2fs_available_free_memory(sbi, 4)
0x366d0c  f2fs_shrink_extent_tree(sbi, 0x80)
0x366d18  f2fs_available_free_memory(sbi, 1)
0x366d2c  f2fs_try_to_free_nats(sbi, 455)
0x366d34  f2fs_available_free_memory(sbi, 0)
0x366d4c  f2fs_build_free_nids(sbi, false, false)
0x366d5c  f2fs_try_to_free_nids(sbi, 0xe38)
0x366e84  blk_start_plug()
0x366e94  f2fs_sync_dirty_inodes(sbi, 1)
0x366e9c  blk_finish_plug()
0x366ea8  f2fs_sync_fs(sbi->sb, true)
0x366eac  increment background-checkpoint statistic
```

This sequence agrees with the historical 4.14 `f2fs_balance_fs_bg()` architecture: extent-cache pressure handling, NAT/free-NID cleanup, decision whether checkpoint work is needed, optional dirty-inode writeout, `f2fs_sync_fs()`, then background checkpoint accounting. Public 4.14-era source shows the same helper ordering. citeturn371638search12turn446071search0

## Genuine X683 / Transsion delta inside `f2fs_balance_fs_bg()`

The vendor-specific block begins after free-NID handling and is controlled by `sbi+0x534` plus seven X683 IO-accounting fields.

### 1. `gc_mode == 3` bypass

```asm
ldr w8, [sbi, #0x534]
cmp w8, #3
b.eq shared_stage
```

Thus Transsion urgent/greedy mode 3 bypasses the normal non-urgent background-balance discriminator.

### 2. Seven-field IO discriminator

For `gc_mode != 3`, X683 tests all seven fields:

```text
sbi + 0x444
sbi + 0x448
sbi + 0x44c
sbi + 0x450
sbi + 0x454
sbi + 0x458
sbi + 0x45c
```

Any nonzero value enters the active-path guard. All seven zero enters the clean-path branch.

The statistics-copy routine at `0x375ed8..0x375f0c` independently copies these exact fields to `stat_info`, proving that this is one coherent X683 IO-accounting region.

Current binary-derived semantic labels, kept separate from historical names:

```text
0x444 = nr_wb_cp_data-like counter
0x448 = nr_wb_data-like counter
0x44c = nr_rd_data-like counter
0x450 = nr_rd_node-like counter
0x454 = nr_rd_meta-like counter
0x458 = nr_dio_write-like counter
0x45c = nr_dio_read-like counter
```

The names above describe semantics, not claimed original source member names.

### 3. Active-path fixed-point / reservation gate

At `0x366da4`:

```text
obj = *(sbi + 0x70)
A   = obj[0x04]
B   = obj[0x18]
C   = obj[0x80]
scaled = (A * B * 0x51EB851F) >> 37
```

The function then requires:

```text
C >= scaled
```

or it falls through to the terminal balance path. If that passes, it evaluates:

```text
s64(sbi + 0x434) >= (sbi + 0x3dc) << 3
```

with the opposite result returning through the stack-canary/exit path.

`0x434` is not a standalone 32-bit field: it is the upper half of the 64-bit `block_t` at `sbi+0x430`.

Therefore the exact SBI facts used here are:

```text
sbi + 0x430 = current_reserved_blocks (64-bit)
sbi + 0x434 = upper 32 bits of current_reserved_blocks
sbi + 0x3dc = blocks_per_seg
```

### 4. Shared secondary stage

The common stage at `0x366de4` performs:

```text
f2fs_available_free_memory(sbi, 1)
f2fs_available_free_memory(sbi, 3)
```

then a nested manager/dirty-state comparison:

```text
obj0 = *(sbi + 0x80)
obj1 = obj0 + 0x10
A = *(obj1 + 0x84)
B = *(obj0 + 0x64)
```

followed by another copy of the fixed-point/reservation test.

This establishes exact X683 accesses but does not yet justify original C member names for the nested fields.

### 5. Jiffies-domain gate

The shared stage computes:

```text
value = sbi + 0x1c8
       + 250 * (sbi + 0x198)
```

and compares it with the kernel `jiffies_64` backing storage.

The clean path repeats the same pattern with:

```text
sbi + 0x1d0
sbi + 0x1a0
```

The factor `250` matches HZ=250.

Because historical F2FS defines `last_time[]` as jiffies-domain values and `interval_time[]` as seconds multiplied by HZ in `f2fs_time_over()`, the strongest current interpretation is:

```text
sbi + 0x198 / 0x1c8 = one last_time[] / interval_time[] pair
sbi + 0x1a0 / 0x1d0 = adjacent pair
```

Exact enum slots are deliberately not promoted yet; the binary proves the pair relationship but not the original enum labels by itself. citeturn974178search0turn371638search1

## X683 SBI field map advanced by this function

### Confirmed / high confidence

```text
0x00  sb pointer
0x48  s_flag region; bit3 is tested by f2fs_balance_fs_bg
0x70  pointer to a nested X683 object used by the vendor fixed-point test
0x80  sm_info
0x198 timestamp-like field used with a 250x interval computation
0x1a0 timestamp-like field used with the clean-branch 250x computation
0x1c8 interval-like field paired with +0x198
0x1d0 interval-like field paired with +0x1a0
0x3dc blocks_per_seg
0x430 current_reserved_blocks (64-bit)
0x434 upper half of current_reserved_blocks
0x444..0x45c seven-field X683 IO-accounting region
0x4b8 mount_opt.opt
0x4b9 byte within mount options used for the balance path's bit7 test
0x534 gc_mode
0x568 stat_info pointer
0x5d4 background-GC counter
0x5d8 IO-skip background-GC counter
0x5dc other-skip background-GC counter
```

### Newly strengthened statistics fields

`gc_thread_func()` directly increments:

```text
sbi + 0x5d4 immediately before f2fs_gc()
sbi + 0x5dc on the explicit skip path
sbi + 0x5d8 on the later skip/accounting path
```

This promotes the earlier candidates to binary-proven counter roles. Historical naming correspondence is:

```text
0x5d4  bg_gc
0x5d8  io_skip_bggc
0x5dc  other_skip_bggc
```

The mapping is supported by both the runtime update sites and the `stat_show()` export path at `0x37610c..0x376120`, which copies the three SBI counters into the stats object. Public 4.14 F2FS source also contains these background-GC/skip statistics concepts. citeturn446071search0

## Important correction to the previous reconstruction

`0x366cd4` must be treated as a modified **`f2fs_balance_fs_bg()` implementation**, not as a separate vendor policy function.

Therefore:

```text
obsolete model:
    vendor policy function
        -> opaque F2FS helpers

correct model:
    historical f2fs_balance_fs_bg()
        + Transsion/X683 IO discriminator
        + reservation/fixed-point guard
        + jiffies-domain policy gate
        + X683 statistics/counters
```

The earlier semantic scaffold should be retained only as analysis history; it is not the source reconstruction target.

## Next exact SBI reconstruction targets

1. Identify the original semantic object at `sbi+0x70` by tracing its allocation and all constructor/consumer accesses.
2. Resolve the nested fields used through `sbi+0x80` in the clean branch, especially `sm_info+0x98`, `sm_info+0xa0`, and the nested `+0x2090` object.
3. Resolve the enum slots for the `last_time[]` / `interval_time[]` pairs at `0x198/0x1c8` and `0x1a0/0x1d0` using independent writers.
4. Continue proving every access in the `0x444..0x45c` IO-accounting region; names remain semantic until a second independent source path pins them.
5. Integrate the proven `0x5d4..0x5dc` counters into the X683 `f2fs_sb_info` reconstruction.

## Evidence status

```text
DIRECT X683 BINARY        = authoritative
KALLSYMS SYMBOL ID        = exact
HISTORICAL F2FS MATCH     = comparison evidence
SEMANTIC MEMBER NAME     = only promoted when independently supported
```
