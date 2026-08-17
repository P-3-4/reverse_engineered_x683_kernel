# X683/H694 `f2fs_stat_info` model

Target stock kernel: Linux 4.14.141+ / X683-H694.

## Evidence boundary

The stock X683 kernel extracted from the supplied `boot.img` was disassembled directly. This closes the previous evidence gap: `sbi + 0x568` is loaded as a pointer and the pointed-to object is accessed at multiple direct member offsets inside stock `f2fs_gc()`.

The stock binary remains authoritative. Historical F2FS source is used only to assign names after the machine-code access pattern matches.

## Binary-proven pointer

```text
struct f2fs_sb_info *sbi
        + 0x568
            -> struct f2fs_stat_info *
```

In stock `f2fs_gc()` at image offset `0x350c18`:

```asm
ldr x8, [x20, #0x568]
```

where `x20` is the saved `sbi` pointer.

Confidence: **confirmed**.

## Directly recovered `f2fs_stat_info` members

The same stock function immediately performs:

```asm
ldr w10, [x8, #0x18c]
ldr w11, [x8, #0x194]
ldr w12, [x8, #0x19c]
...
add w9, w10, #1
add w10, w11, #1
...
str w9,  [x8, #0x18c]
str w10, [x8, #0x194]
...
add w11, w12, w11
str w11, [x8, #0x19c]
```

This proves that the object reached through `sbi + 0x568` has live 32-bit members at:

```text
STAT + 0x18c
STAT + 0x194
STAT + 0x19c
```

Historical 4.14 F2FS structure ordering gives the corresponding names:

```text
+0x188  tot_segs
+0x18c  node_segs
+0x190  data_segs
+0x194  free_segs
+0x198  free_secs
+0x19c  bg_node_segs
+0x1a0  bg_data_segs
```

The X683 access semantics match this ordering: the function increments `node_segs`, increments `free_segs`, and adds a calculated count to `bg_node_segs`.

Therefore these three names are now **binary-confirmed by direct pointer-relative accesses plus historical structural correlation**:

| `f2fs_stat_info` offset | Field | Confidence |
|---:|---|---|
| `0x18c` | `node_segs` | **confirmed** |
| `0x194` | `free_segs` | **confirmed** |
| `0x19c` | `bg_node_segs` | **confirmed** |

This is the first direct recovery of actual `f2fs_stat_info` members rather than SBI fields adjacent to the pointer.

## Historical structure fingerprint

The surrounding historical 4.14 sequence is:

```text
+0x174  prefree_count / related prefree statistics
+0x188  tot_segs
+0x18c  node_segs
+0x190  data_segs
+0x194  free_segs
+0x198  free_secs
+0x19c  bg_node_segs
+0x1a0  bg_data_segs
+0x1a4  curseg[]
+0x1d4  dirty_seg[]
+0x204  meta_count[]
+0x214  segment_count[]
+0x21c  block_count[]
+0x224  inplace_count
```

The exact complete structure must still be validated against additional X683 accesses; only the members directly exercised by the binary should be promoted.

## SBI statistics region remains separate

The following addresses are within `struct f2fs_sb_info`, not the pointed-to statistics object:

```text
sbi + 0x568  stat_info pointer
sbi + 0x570  meta_count[4]        candidate
sbi + 0x580  segment_count[2]     candidate
sbi + 0x588  block_count[2]       candidate
sbi + 0x590  inplace_count        candidate
...
sbi + 0x5d4  bg_gc
sbi + 0x5d8  io_skip_bggc
sbi + 0x5dc  other_skip_bggc
```

## Direct X683 evidence for `0x5d4..0x5dc`

The stock GC-thread function contains all three counter updates.

### `sbi + 0x5d4`

At image offset `0x3500bc`:

```asm
ldr w8, [x19, #0x5d4]
add w8, w8, #1
...
str w8, [x19, #0x5d4]
...
bl  0x3503a8
```

The immediately following call is the stock `f2fs_gc()` entry at `0x3503a8`. This directly confirms `0x5d4` as the background-GC call counter:

```text
sbi + 0x5d4 = bg_gc
```

Confidence: **confirmed**.

### `sbi + 0x5d8`

At image offset `0x350284` the GC thread releases the GC mutex through the stock unlock primitive at `0xe06684`, then increments `sbi + 0x5d8`:

```asm
mov x0, x21
bl  0xe06684
ldr w8, [x19, #0x5d8]
add w8, w8, #1
str w8, [x19, #0x5d8]
```

The control-flow position matches the historical background-GC path that unlocks the GC mutex after the idle/IO check and accounts an IO-related background-GC skip.

```text
sbi + 0x5d8 = io_skip_bggc
```

Confidence: **very high / direct semantic match**.

### `sbi + 0x5dc`

The GC-thread also contains direct increment sites:

```asm
ldr w8, [x19, #0x5dc]
add w8, w8, #1
str w8, [x19, #0x5dc]
```

The corresponding control-flow branch is the non-IO skip path in the background-GC decision logic. This matches the historical `other_skip_bggc` accounting path.

```text
sbi + 0x5dc = other_skip_bggc
```

Confidence: **high**, with the historical semantic distinction retained as a sanity check.

## Historical source correlation

The matching Android/common 4.14 generation contains:

```c
#define stat_inc_bggc_count(sbi) ((sbi)->bg_gc++)
#define stat_io_skip_bggc_count(sbi) ((sbi)->io_skip_bggc++)
#define stat_other_skip_bggc_count(sbi) ((sbi)->other_skip_bggc++)
```

and the corresponding `f2fs_stat_info` ordering contains `tot_segs`, `node_segs`, `data_segs`, `free_segs`, `free_secs`, `bg_node_segs`, and `bg_data_segs`.

The source match is therefore now supported by direct X683 machine-code semantics rather than structure ordering alone.

## Current X683 status

| Item | Status |
|---|---|
| `sbi + 0x568` is `stat_info *` | **Confirmed** |
| `STAT + 0x18c = node_segs` | **Confirmed** |
| `STAT + 0x194 = free_segs` | **Confirmed** |
| `STAT + 0x19c = bg_node_segs` | **Confirmed** |
| `sbi + 0x5d4 = bg_gc` | **Confirmed** |
| `sbi + 0x5d8 = io_skip_bggc` | **Very high confidence / direct semantic match** |
| `sbi + 0x5dc = other_skip_bggc` | **High confidence / direct semantic match** |
| Complete X683 `f2fs_stat_info` layout | **Partially recovered** |
| Exact vendor source revision | **Still unresolved** |

## Next binary targets

1. Recover additional `STAT + offset` accesses from the stock GC/statistics functions.
2. Map `+0x188..0x1a0` completely and verify `tot_segs`, `data_segs`, `free_secs`, and `bg_data_segs` independently.
3. Recover the `dirty_seg[]`, `full_seg[]`, `valid_blks[]`, and `meta_count[]` portions of the statistics object.
4. Compare the resulting structure against the closest 4.14 source revision and Transsion/MediaTek modifications.
5. Continue recovering the remaining `f2fs_gc()` victim-selection and accounting control flow.

## Sanity rule

Never treat `sbi + 0x570..` as `stat_info` members. The pointer at `sbi + 0x568` is the boundary. Actual `f2fs_stat_info` members must be expressed relative to the loaded STAT pointer.
