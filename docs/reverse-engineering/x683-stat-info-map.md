# X683/H694 `f2fs_stat_info` binary map

Target: stock X683/H694 Linux 4.14.141+ kernel, build date 2021-11-05.

## Evidence rule

The stock ARM64 Image is authoritative. Historical Android/common F2FS source is used only to attach names where the X683 access semantics and structure ordering support the name.

## 1. `stat_info` pointer

Stock `f2fs_gc()` loads:

```asm
ldr x8, [x20, #0x568]
```

Therefore:

```text
sbi + 0x568 -> struct f2fs_stat_info *
```

Confidence: confirmed.

## 2. Direct segment-accounting members

At image offset `0x350c18` the loaded pointer is used directly:

```asm
ldr x8, [x20, #0x568]
ldr w10, [x8, #0x18c]
ldr w11, [x8, #0x194]
ldr w12, [x8, #0x19c]
...
add w9,  w10, #1
add w10, w11, #1
...
add w11, w12, w11
str w9,  [x8, #0x18c]
str w10, [x8, #0x194]
str w11, [x8, #0x19c]
```

The operations prove these are mutable segment statistics:

```text
stat_info + 0x18c  node_segs       confirmed
stat_info + 0x194  free_segs       confirmed
stat_info + 0x19c  bg_node_segs    confirmed as a background-node segment counter
```

The exact surrounding fields `0x190`, `0x198`, and `0x1a0` must not be inferred merely from a public structure because the X683 layout has vendor-specific differences.

## 3. Critical correction: `0x1a0`

The statistics-copy routine at image offset `0x376124` contains:

```asm
ldr x8, [x23, #0x540]
str x8, [x24, #0x1a0]
ldr x8, [x23, #0x548]
str x8, [x24, #0x1a8]
```

Here `x24` is the `stat_info` object and `x23` is the `f2fs_sb_info` object.

Therefore the X683 mapping is:

```text
stat_info + 0x1a0 = sbi->skipped_atomic_files[0]
stat_info + 0x1a8 = sbi->skipped_atomic_files[1]
```

This is direct binary evidence and overrides the tempting historical assumption that `0x1a0` must be `bg_data_segs`.

## 4. Direct SBI -> `stat_info` copy map

The stock statistics-copy routine begins at image offset `0x375e18` with:

```asm
ldr x23, [x28, #0x10]
ldr x24, [x23, #0x568]
```

Selected direct mappings are:

```text
SBI offset   STAT offset   status / interpretation
-----------------------------------------------------------
0x5a8        0x038         direct copy; extent statistic
0x5b0        0x040         direct copy; extent statistic
0x5a0        0x048         direct copy; extent statistic
0x598        0x058         direct copy; extent statistic
0x3a4        0x060         direct copy
0x3b8        0x064         direct copy
0x3bc        0x068         direct copy
0x434        0x06c         direct copy
0x428        0x070         direct copy
0x438        0x074         direct copy
0x440        0x078         direct copy
0x42c        0x07c         direct copy
0x430        0x080         direct copy
0x5e0        0x088         direct copy
0x5e4        0x08c         direct copy
0x424        0x090         direct copy
0x5e8        0x094         direct copy
0x45c        0x0d4         direct copy
0x458        0x0d8         direct copy
0x444        0x0c0         direct copy
0x448        0x0c4         direct copy
0x44c        0x0c8         direct copy
0x450        0x0cc         direct copy
0x454        0x0d0         direct copy
0x5c4        0x118         direct copy
0x5c8        0x120         direct copy
0x5cc        0x11c         direct copy
0x5d0        0x124         direct copy
0x408        0x0b4         derived value
0x40c        0x128         direct copy
0x410        0x134         direct copy
0x3f0        0x12c         direct copy
0x5b8        0x100         direct copy
0x5bc        0x104         direct copy
0x5c0        0x108         direct copy
0x250        0x10c         direct copy
0x280        0x110         direct copy
0x220        0x114         direct copy
```

These are binary mappings; field names should be attached only after semantic correlation.

## 5. SBI GC counters copied into `stat_info`

The copy routine directly performs:

```asm
ldr w8, [x23, #0x5d4]
str w8, [x24, #0xbc]

ldr w8, [x23, #0x5d8]
str w8, [x24, #0xdc]

ldr w8, [x23, #0x5dc]
str w8, [x24, #0xe0]
```

Combined with the GC increment paths, this gives:

```text
sbi + 0x5d4 = bg_gc             confirmed
sbi + 0x5d8 = io_skip_bggc      confirmed by semantic path
sbi + 0x5dc = other_skip_bggc   confirmed by semantic path
```

and their statistics-object mirrors:

```text
stat_info + 0x0bc = bg_gc
stat_info + 0x0dc = io_skip_bggc
stat_info + 0x0e0 = other_skip_bggc
```

## 6. Statistics derived from `dirty_info`

The copy routine also reaches:

```text
sbi + 0x80 -> sm_info
sm_info + 0x10 -> dirty_info
```

and reads:

```asm
ldp w9,  w10, [x8, #0x68]
ldp w11, w12, [x8, #0x70]
ldp w10, w8,  [x8, #0x78]
```

It sums all six values and stores the result at:

```text
stat_info + 0x154
```

This proves `0x154` is a derived dirty-segment statistic, while the exact historical `nr_dirty[]` naming remains dependent on the corresponding source revision.

## 7. Segment/section derived statistics

The stock copy routine generates repeated values from `sm_info` and the segment geometry. It writes:

```text
stat + 0x1b0 / 0x1b4 / 0x1b8 / 0x1bc / 0x1c0 / 0x1c4
```

and normalized values at:

```text
stat + 0x1c8 / 0x1cc / 0x1d0 / 0x1d4 / 0x1d8 / 0x1dc
stat + 0x1e0 / 0x1e4 / 0x1e8 / 0x1ec / 0x1f0 / 0x1f4
```

The source operands are `dirty_info` counters at offsets:

```text
+0x5c
+0xcc
+0x13c
+0x1ac
+0x21c
+0x28c
```

with normalization using `sbi->segs_per_sec` and `sbi->log_blocks_per_seg`/related geometry.

These are retained as numeric mappings until the exact vendor `dirty_info` structure is reconstructed.

## 8. SBI post-statistics region

The same copy routine directly maps:

```text
sbi + 0x570 -> stat + 0x1f8
sbi + 0x574 -> stat + 0x1fc
sbi + 0x578 -> stat + 0x200
sbi + 0x57c -> stat + 0x204
sbi + 0x580 -> stat + 0x208
sbi + 0x584 -> stat + 0x20c
sbi + 0x588 -> stat + 0x210
sbi + 0x58c -> stat + 0x214
sbi + 0x590 -> stat + 0x218
```

This is strong binary evidence that the SBI statistics region immediately after `stat_info *` is real and separately copied into the `stat_info` object.

Historical 4.14 F2FS has a closely related `meta_count[]`, `segment_count[]`, `block_count[]`, and `inplace_count` sequence, but the X683 mapping above is preferred over a source-only offset guess.

## 9. Historical correlation

Public Android/common F2FS source contains the same conceptual statistics families: `node_segs`, `data_segs`, `free_segs`, `free_secs`, `bg_node_segs`, `bg_data_segs`, `tot_blks`, `data_blks`, `node_blks`, background block counters, and `skipped_atomic_files[2]`. It also places `bg_gc`, `io_skip_bggc`, and `other_skip_bggc` in `f2fs_sb_info` rather than inside the pointed-to object. This supports the semantic interpretation but does not override the X683 offsets.

## 10. Current confidence rules

Confirmed by direct X683 machine code:

```text
sbi + 0x568 -> stat_info *
stat + 0x18c -> node_segs
stat + 0x194 -> free_segs
stat + 0x19c -> bg_node_segs
stat + 0x1a0 -> mirror of sbi->skipped_atomic_files[0]
stat + 0x1a8 -> mirror of sbi->skipped_atomic_files[1]
stat + 0x0bc -> mirror of sbi->bg_gc
stat + 0x0dc -> mirror of sbi->io_skip_bggc
stat + 0x0e0 -> mirror of sbi->other_skip_bggc
```

Do not promote unverified historical names merely because the numeric offset matches an upstream 4.14 structure.

## Next targets

1. Recover the complete `stat_info` field map from `0x098` through `0x1f4`.
2. Identify the exact vendor modifications responsible for the `0x1a0/0x1a8` placement.
3. Reconstruct the six `dirty_info` counters at `+0x68..+0x7c` and their later counterparts at `+0x5c`, `+0xcc`, `+0x13c`, `+0x1ac`, `+0x21c`, and `+0x28c`.
4. Map the remaining SBI statistics region through `0x5e8`.
5. Only then begin translating the recovered offsets into a compilable X683 F2FS structure definition.
