# F2FS GC API history and X683 constraint

## Public history

Public F2FS history shows the GC prototype changing across 4.14-era and later trees. The X683 binary must be treated as authoritative rather than being coerced to a currently inspected public prototype.

## X683 binary result — supersedes the earlier unresolved/three-argument note

The decompressed X683 Image directly proves the four-argument callable ABI:

```c
f2fs_gc(sbi,
        ((sbi->mount_opt.opt >> 14) & 1),
        true,
        NULL_SEGNO);
```

At `tran_do_f2fs_gc + 0x58`, `+0x94`, and `+0xc4` the registers are prepared as:

```text
x0 = sbi
w1 = bit 14 of sbi + 0x4b8
w2 = 1
w3 = -1 (NULL_SEGNO)
```

The stock symbol is `f2fs_gc` at `0xffffff92d0dd03a8`.

The vendor wrapper is `tran_do_f2fs_gc` at `0xffffff92d0dfada8`.

Therefore the previous statement that the exact X683 prototype was unresolved is obsolete and is intentionally superseded here by direct binary evidence.

## Source-baseline rule

The X683 kernel identifies itself as Linux `4.14.141+`, built with Android clang 9.0.3 on 2021-11-05. Public Android/Linux 4.14 trees are comparison/naming baselines only. Where a public source prototype differs from the X683 call-site, the X683 binary wins.

The exact Transsion vendor source git revision is not proven by the supplied image metadata.
