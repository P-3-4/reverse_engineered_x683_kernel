# X683/H694 GC ABI correction

Direct disassembly of the supplied stock boot.img proves the X683 GC entry point at kernel-image offset `0x3503a8` uses **four arguments**:

```c
f2fs_gc(sbi, sync, background, segno);
```

## Proof

At `0x3503a8` the function immediately saves:

```text
3503dc  str w2, [sp, #0xac]
3503e4  str w3, [sp, #0x16c]
350430  str w1, [sp, #0xfc]
```

Later it takes the fourth argument's address:

```text
350820  add x1, sp, #0x16c
```

and reads the saved value:

```text
350858  ldr w27, [sp, #0x16c]
```

This is the exact shape of the historical F2FS `f2fs_gc(sbi, bool sync, bool background, unsigned int segno)` implementation.

## Call-site proof

The stock image contains several four-argument calls.

A caller at `0x333084` computes a real segment value into `w3` and calls:

```text
333074  cmp  w28, #0
333078  cset w1, ne
33307c  mov  w2, #1
333080  mov  x0, x20
333084  bl   0x3503a8
```

Another caller explicitly sets:

```text
3343f8  mov w3, #-1
3343fc  mov x0, x20
334400  bl  0x3503a8
```

The Transsion wrapper at `0x37ada8` likewise supplies:

```text
37adec  mov  w2, #1
37adf0  mov  w3, #-1
37adf4  ubfx w1, w8, #14, #1
37adf8  mov  x0, x19
37ae00  bl   0x3503a8
```

and the alternate policy path repeats the same four arguments.

`-1` is `NULL_SEGNO`; callers that have a preselected victim pass a real segment number.

## Revision boundary

This is consistent with the historical F2FS transition to:

```c
int f2fs_gc(struct f2fs_sb_info *sbi, bool sync,
            bool background, unsigned int segno)
```

The later five-argument `force` form is not present in the X683 call sites. The older three-argument form is also not the stock X683 ABI.

Therefore all reconstructed X683 GC code must use the four-argument ABI until direct evidence proves otherwise.

## Consequence

Earlier project notes that described the stock X683 entry point as:

```c
f2fs_gc(sbi, sync, background)
```

are superseded by this direct binary evidence.
