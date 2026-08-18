# X683/H694 Transsion GC — Stop Conditions 1–5 disassembly

Source image SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`

Offsets below are decompressed-kernel offsets.

## Condition 1

```text
0x377720  3f 03 09 6b   cmp     w25, w9
0x377724  2c 12 00 54   b.gt    0x377968
```

The target block logs `match: Stop condition 1, delta_seg=%d`, passes `w25`, sets `w8 = 1`, and stores `w8` into `controller+0x9fc`.

The threshold in `w9` is produced immediately before the comparison:

```text
w9 = table[index] * w21
w9 = signed-multiply-by-0x51EB851F, shifted by 37 with sign correction
```

`0x51EB851F / 2^37` is approximately 0.025, so the compiled threshold is approximately 2.5% of the selected base quantity.

## Condition 2

```text
0x37776c  6b 0a 01 3f   cmp     w9, w10
0x377770  ac 10 00 54   b.gt    0x377984
```

The target logs `match: Stop condition 2`, sets `w8 = 2`, and stores it into `controller+0x9fc`.

`w10` is the second table-scaled threshold and `w9` is the second calculated delta.

## Condition 3

```text
0x3777c8  08 01 00 eb   cmp     x8, x9
0x3777cc  ed 00 00 54   b.lt    0x377998
```

The target logs `match: Stop condition 3`, sets `w8 = 3`, and stores it into `controller+0x9fc`.

The operands are a 64-bit scaled movement quantity (`x8`) and a signed segment/reference quantity (`x9`).

## Condition 4

The condition is evaluated immediately before the Stop-4 block:

```text
0x3777e8  3f 01 08 eb   cmp     x9, x8
0x3777ec  fa b4 00 90   adrp    x26, ...
0x3777f0  ed 01 00 54   b.le    <continue-without-stop-4>
```

If the comparison is greater-than, execution falls through to the Stop-4 block:

```text
0x3777fc  f0 00 69 60   adrp    x0, ...
0x377800  00 ac 09 91   add     x0, x0, #0x26b
0x377804  82 02 08 cb   sub     x2, x20, x8
0x377808  ac f1 f4 97   bl      printk/log helper

0x37780c  e8 b4 00 90   adrp    x8, controller
0x377810  08 01 67 39   ldrb    w8, [x8, #0x9c0]
0x377814  88 00 00 37   tbnz    w8, #0, <skip-controller-write>
0x377818  e8 b4 00 90   adrp    x8, controller
0x37781c  e9 03 1f 32   mov     w9, #2
0x377820  09 99 09 b9   str     w9, [x8, #0x998]

0x377824  e8 b4 00 90   adrp    x8, controller
0x377828  e9 03 00 32   mov     w9, #1
0x37782c  09 f9 09 b9   str     w9, [x8, #0x9f8]
```

The literal referenced by the logger is:

`match: Stop condition 4, dec_seg=%d, inc_written_seg=%d, switch to SSR`

This is direct binary evidence that **Stop Condition 4 is the SSR-switch trigger**.

## Condition 5

Periodicity is selected first:

```text
0x377830  e8 b4 00 90   adrp    x8, controller
0x377834  08 11 68 39   ldrb    w8, [x8, #0xa04]
0x377838  e9 b4 00 90   adrp    x9, controller
0x37783c  29 c9 44 f9   ldr     x9, [x9, #0x990]
0x377840  4a 06 80 52   mov     w10, #50
0x377844  1f 01 00 71   cmp     w8, #0
0x377848  88 3e 80 52   mov     w8, #500
0x37784c  48 11 88 9a   csel    x8, x10, x8, ne
0x377850  2a a5 c8 9a   udiv    x10, x9, x8
0x377854  28 03 00 9b   msub    x8, x10, x8, x9
0x377858  e8 b4 00 90   adrp    x8, controller
0x37785c  08 0d 4a b9   ldr     w8, [x8, #0xa0c]
0x377860  e9 b4 00 90   adrp    x9, controller
0x377864  29 09 8a b9   ldrsw   x9, [x9, #0xa08]
0x377868  e8 02 08 4b   sub     w8, w23, w8
0x37786c  88 c2 28 8b   add     x8, x20, w8, sxtw
0x377870  1f 01 09 eb   cmp     x8, x9
0x377874  ac 01 00 54   b.gt    <normal-progress-path>
```

When the periodic condition indicates insufficient/no free-segment progress, the path writes controller `2`, sets `+0x9f8 = 2`, and logs:

`match: Stop condition 5,every 400 times gc none free segment inc`

The compiled cadence is **50 or 500**, while the literal says **400**. Both observations are preserved because they are independently present in the stock image.

## Controller consequences

- Stop 1 -> `+0x9fc = 1`
- Stop 2 -> `+0x9fc = 2`
- Stop 3 -> `+0x9fc = 3`
- Stop 4 -> `+0x998 = 2` unless `+0x9c0` blocks it; `+0x9f8 = 1`
- Stop 5 -> `+0x998 = 2` through the same guarded controller-write path; `+0x9f8 = 2`

Controller value `2` is consumed by `tran_f2fs_gc`, which temporarily forces `sbi+0x534 = 3` and invokes `f2fs_gc(sbi, sync, true, -1)`.
