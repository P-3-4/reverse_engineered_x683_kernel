# X683/H694 full-Image writer/consumer sweep — 2026-08-18

Authority: uploaded stock `boot(8).img` + uploaded standalone compressed kernel.

Verified Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`.

## Seven SBI policy fields

Target fields:

```text
+0x444 +0x448 +0x44c +0x450 +0x454 +0x458 +0x45c
```

### Proven SBI initialization

`0x344efc..0x344f14` is an SBI initialization sequence. The surrounding function writes known `f2fs_sb_info` fields including `+0x3dc`, `+0x3e0`, `+0x530`, `+0x538`, `+0x560`, `+0x1c8`, and `+0x428..+0x440`.

Exact stores:

```text
0x344efc  [sbi+0x444] = 0
0x344f00  [sbi+0x448] = 0
0x344f04  [sbi+0x44c] = 0
0x344f08  [sbi+0x450] = 0
0x344f0c  [sbi+0x454] = 0
0x344f10  [sbi+0x458] = 0
0x344f14  [sbi+0x45c] = 0
```

### Proven non-initialization SBI writers

```text
0x32b0e0  str x8, [sbi,#0x450]
0x338f58  str x8, [sbi,#0x450]
0x338f5c  str x9, [sbi,#0x448]
0x338f70  str x9, [sbi,#0x458]
```

`0x32b0e0` is in a function with `x19=x0`; the function also accesses `[sbi+0x70]` before the store.

The function containing `0x338f58/0x338f5c/0x338f70` establishes `x19=x0` at `0x338be8`, then stores those fields before its return at `0x338f84`.

### Still unresolved

No writer is yet proven, at the `f2fs_sb_info` base, for non-initialization stores to:

```text
+0x444
+0x44c
+0x454
+0x45c
```

Raw stores to the same numeric offsets elsewhere are excluded when the base structure is demonstrably unrelated.

### Explicit false positives

`0x6d5474..0x6d548c` zeroes the same range in another structure.

`0xa77828..0xa77998` writes several of the same offsets inside a battery/thermal configuration object; surrounding literals include `min_charge_temp`, `max_charge_temp`, and `temp_t4_thres_minus_x_degree`. These are not F2FS SBI writers.

## Image+0x16c6980

The GC policy reads the same 64-bit global at:

```text
0x366e64
0x366f10
```

The global is reached through:

```asm
adrp x10, <Image+0x16c6000 page>
ldr  x10, [x10,#0x980]
```

A whole-image ADRP/xref sweep finds roughly 1,000 direct loads of that exact address, so it is not a Transsion-GC-only variable.

The data bytes at the global are:

```text
08 db fe ff 00 00 00 00
```

Low 32 bits: `0xfffedb08` = signed `-75000`.

A decisive early-kernel reference around `0x170c` uses `CNTVCT_EL0` in the same code path that accesses this global. This establishes a time/clock-domain relationship at the kernel level.

Current safe classification:

```text
shared kernel time/clock-related global/reference
```

The exact original source symbol is not yet proven.

### Producer search

No direct `STR [ADRP-base + #0x980]` writer to this exact global was found. A runtime producer may therefore operate through an indirect pointer/helper/atomic path or through a separate initialization mechanism.

Do not rename it to `jiffies`, `sched_clock`, a timeout, or a vendor GC threshold without a producer proof.

## Terminal statistics

The terminal policy path performs:

```text
stat = *(sbi + 0x568)
stat + 0x16c++
```

Historical 4.14 `f2fs_stat_info` layout strongly correlates `+0x16c` with `dirty_count`; this remains a source-level candidate, separate from the seven-field writer investigation.

## Status

The full uploaded Image is now being used as the byte-level authority.

Resolved further:

```text
all-seven initialization writer
+0x450 runtime writer(s)
+0x448 runtime writer
+0x458 runtime writer
shared time/clock character of +0x16c6980
```

Remaining highest-value targets:

```text
+0x444 runtime writer
+0x44c runtime writer
+0x454 runtime writer
+0x45c runtime writer
exact symbol/producer for +0x16c6980
exact semantic names for +0x448/+0x450/+0x458
```
