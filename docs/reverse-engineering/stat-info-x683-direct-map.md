# X683/H694 `f2fs_stat_info` Direct Binary Map

Target: X683/H694 stock kernel, Linux 4.14.141+, Android 10.

Evidence source: raw ARM64 Image extracted from the supplied stock `boot.img`.

## Evidence rule

Direct X683 instructions are authoritative. Historical F2FS source is used only to identify semantics where the machine-code operation is unambiguous or to constrain candidates.

## `stat_info` pointer

At Image offset `0x375e1c`:

```asm
ldr x24, [x23, #0x568]
```

with `x23 = struct f2fs_sb_info *`.

Therefore:

```text
sbi + 0x568 = struct f2fs_stat_info *
```

## Direct SBI -> STAT copy map

The stock statistics-copy routine begins at approximately Image offset `0x375e18`.

Confirmed numeric mappings:

```text
SBI offset   STAT offset
---------    ----------
0x5a8        0x038       64-bit
0x5b0        0x040       64-bit
0x5a0        0x048       64-bit
0x598        0x058       64-bit
0x3a4        0x060       32-bit
0x3b8        0x064       32-bit
0x3bc        0x068       32-bit
0x434        0x06c       32-bit
0x428        0x070       32-bit
0x438        0x074       32-bit
0x440        0x078       32-bit
0x42c        0x07c       32-bit
0x430        0x080       32-bit
0x43c        0x084       32-bit
0x5e0        0x088       32-bit
0x5e4        0x08c       32-bit
0x424        0x090       32-bit
0x5e8        0x094       32-bit
```

Additional direct mappings:

```text
SBI 0x458 -> STAT 0x0d8
SBI 0x45c -> STAT 0x0d4
SBI 0x444 -> STAT 0x0c0
SBI 0x448 -> STAT 0x0c4
SBI 0x44c -> STAT 0x0c8
SBI 0x450 -> STAT 0x0cc
SBI 0x454 -> STAT 0x0d0

SBI 0x5c4 -> STAT 0x118
SBI 0x5c8 -> STAT 0x120
SBI 0x5cc -> STAT 0x11c
SBI 0x5d0 -> STAT 0x124

SBI 0x5d4 -> STAT 0x0bc
SBI 0x5d8 -> STAT 0x0dc
SBI 0x5dc -> STAT 0x0e0

SBI 0x5b8 -> STAT 0x100
SBI 0x5bc -> STAT 0x104
SBI 0x5c0 -> STAT 0x108

SBI 0x40c -> STAT 0x128 and 0x130 (derived calculation at site)
SBI 0x410 -> STAT 0x134
SBI 0x3f0 -> STAT 0x12c
SBI 0x250 -> STAT 0x10c
SBI 0x280 -> STAT 0x110
SBI 0x220 -> STAT 0x114
```

The exact semantic field names for the SBI-side `0x3a4`, `0x3b8`, `0x3bc`, `0x434`, `0x42c`, `0x43c`, and `0x424` accesses are not promoted here solely from this copy routine.

## Confirmed `skipped_atomic_files` mapping

At Image offsets `0x376124` onward:

```asm
ldr x8, [x23, #0x540]
str x8, [x24, #0x1a0]
ldr x8, [x23, #0x548]
str x8, [x24, #0x1a8]
```

Therefore:

```text
STAT + 0x1a0 = skipped_atomic_files[0]
STAT + 0x1a8 = skipped_atomic_files[1]
```

This is binary-confirmed.

It also demonstrates that the X683 `stat_info` layout must not be copied blindly from a public F2FS revision whose `0x1a0` position has another semantic field.

## Confirmed SBI background-GC counters

The same routine copies:

```text
SBI + 0x5d4 -> STAT + 0x0bc
SBI + 0x5d8 -> STAT + 0x0dc
SBI + 0x5dc -> STAT + 0x0e0
```

Independent GC call sites increment the SBI fields with the expected background-GC accounting behavior.

Therefore the following names are now binary-confirmed:

```text
sbi + 0x5d4 = bg_gc
sbi + 0x5d8 = io_skip_bggc
sbi + 0x5dc = other_skip_bggc
```

## Dirty-info derived statistics

The routine follows:

```text
sbi + 0x80 -> sm_info
sm_info + 0x10 -> dirty_info
```

and reads six consecutive 32-bit values:

```text
dirty_info + 0x68
+0x6c
+0x70
+0x74
+0x78
+0x7c
```

It sums them and stores the result in:

```text
STAT + 0x154
```

This is direct binary evidence for the numeric relationship. The exact six-entry semantic names remain provisional pending an independent dirty-segment path.

The routine also derives the segment statistics from dirty-info offsets:

```text
+0x5c  -> STAT + 0x1b0, divided by segs_per_sec -> 0x1c8, then secs_per_zone -> 0x1e0
+0xcc  -> STAT + 0x1b4, then 0x1cc, then 0x1e4
+0x13c -> STAT + 0x1b8, then 0x1d0, then 0x1e8
+0x1ac -> STAT + 0x1bc, then 0x1d4, then 0x1ec
+0x21c -> STAT + 0x1c0, then 0x1d8, then 0x1f0
+0x28c -> STAT + 0x1c4, then 0x1dc, then 0x1f4
```

These are direct numeric relationships and should be preserved even before the final member names are reconstructed.

## Historical correlation

Public 4.14-era F2FS contains the same broad statistics concepts: extent-hit counters, GC/skip counters, node/data segment counts, block counts, current-segment statistics, dirty-segment statistics, and `skipped_atomic_files[2]`. The public structure ordering is useful as a fingerprint, but the X683 binary proves that its vendor revision has diverged in the exact member ordering.

Sources:

- Android/common / historical F2FS `f2fs.h`: https://android.googlesource.com/kernel/common/+/e0c24a32aa2b3a240ca9cd6d9a5d6aa8e217f8ad/fs/f2fs/f2fs.h
- Linux F2FS historical `f2fs.h`: https://code.googlesource.com/linux/torvalds/linux/+/18ded910b589839e38a51623a179837ab4cc3789/fs/f2fs/f2fs.h

## Current confidence

### Confirmed

```text
sbi + 0x568 = stat_info *
stat + 0x1a0 = skipped_atomic_files[0]
stat + 0x1a8 = skipped_atomic_files[1]
stat + 0x038 / 0x040 / 0x048 / 0x058 mappings above
stat + 0x0bc = bg_gc copy target
stat + 0x0dc = io_skip_bggc copy target
stat + 0x0e0 = other_skip_bggc copy target
stat + 0x154 = six-entry dirty_info aggregate
stat + 0x1b0..0x1f4 = six dirty-info derived segment metrics
```

### Strong but semantic name still being validated

```text
stat + 0x060..0x094
stat + 0x098..0x0b8
stat + 0x0c0..0x0d8
stat + 0x0e4..0x0fc
stat + 0x100..0x124
stat + 0x128..0x148
stat + 0x158..0x180
stat + 0x188..0x19c
stat + 0x1b0..0x1f4
stat + 0x1f8..0x218
```

## Immediate next binary targets

1. Identify semantic names of `stat + 0x060..0x094` using debug-print/counter call sites.
2. Map `stat + 0x098..0x0b8` from the source fields feeding those copies.
3. Recover the exact meanings of `stat + 0x0e4..0xfc` and the GC-thread/driver-derived fields.
4. Prove `stat + 0x188..0x19c` independently, especially the relationship between `node_segs`, `free_segs`, and `bg_node_segs`.
5. Resolve the six dirty-info categories behind `+0x5c/+0xcc/+0x13c/+0x1ac/+0x21c/+0x28c`.
6. Only after this map stabilizes, translate the numeric reconstruction into a normal C `struct f2fs_stat_info`.
