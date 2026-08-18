# X683/H694 `0x366cd4` vendor GC policy — binary pass 2

Source authority: exact stock X683/H694 Image extracted from the supplied `boot.img`.

## Function boundary

`0x366cd4..0x366edc` is the vendor GC policy/orchestration function called from the detector. It is distinct from:

- `0x37ada8` — one-argument `tran_f2fs_gc()` controller wrapper;
- `0x3503a8` — four-argument X683 `f2fs_gc()` execution core.

## Entry / first policy ladder

The function begins with:

```text
sbi + 0x48, bit 3 set
    -> immediate return path

0x35cc18(sbi, 4)
    false -> 0x373108(sbi, 0x80)

0x35cc18(sbi, 1)
    false -> 0x35d22c(sbi, 455)

0x35cc18(sbi, 0)
    true  -> 0x362c40(sbi, 0, 0)
    false -> 0x363288(sbi, 0xE38)
```

The selector values 4, 1, and 0 are direct immediates in the call sites. Their vendor symbolic names remain unresolved.

## GC-mode branch

The wrapper reads:

```c
u32 mode = *(u32 *)((char *)sbi + 0x534);
```

and branches on `mode == 3`.

The non-mode-3 path checks these fields independently and takes the common guarded path when any is nonzero:

```text
sbi + 0x44c
sbi + 0x450
sbi + 0x454
sbi + 0x448
sbi + 0x444
sbi + 0x45c
sbi + 0x458
```

This is direct machine-code evidence; symbolic identities of these members are not assigned here.

## Fixed-point guard

The common guarded branch repeatedly computes a percentage-like quantity from three fields reached through the object at `sbi + 0x70`:

```text
object + 0x04
object + 0x18
object + 0x80
```

The arithmetic uses the same fixed-point family already recovered in the vendor detector:

```text
100-scale multiplication
0x51EB851F
right shift by 37
```

The calculated value is compared against a byte/word threshold stored in the vendor state around `+0x3DC` relative to the current object used by this function.

The exact symbolic names of these object fields are intentionally not invented.

## Secondary policy ladder

After the first fixed-point guard, the function evaluates:

```text
0x35cc18(sbi, 1)
    false -> skip remaining policy body

0x35cc18(sbi, 3)
    false -> skip remaining policy body
```

When both pass, the function compares a dirty-segment quantity against another reservation/limit quantity and repeats the same percentage guard.

The relevant load chain is:

```text
sbi + 0x80
    -> object + 0x10
    -> scalar + 0x64

sbi + 0x70
    -> object + 0x04
    -> object + 0x18
    -> object + 0x80
```

The comparison is signed and feeds the common early-return branch.

## Time / current-value guard

The later policy block loads:

```text
sbi + 0x1C8
sbi + 0x198
vendor global around Image + 0x16C6000 + 0xC14
```

and performs a multiply/high-word/fixed-point comparison before the final post-GC preparation path.

This is a time/current-capacity style guard, but the exact symbolic source fields remain unresolved.

## Post-GC preparation path

The final path checks byte `sbi + 0x4B9`, bit 7:

```text
bit 7 clear -> return
bit 7 set   ->
    0x3e1014(stack-object)
    0x34e224(stack-object, 1)
    0x3e1558(stack-object)
    0x341250(sbi->sb, 1)
```

Afterward:

```c
stat = *(struct f2fs_stat_info **)((char *)sbi + 0x568);
stat->field_0x16c++;
```

The field at `stat_info + 0x16c` is therefore a vendor policy-path completion counter, but its original symbolic member name is not established.

## Exact control-flow summary

```text
entry
 |
 +-- sbi+0x48 bit3 -> return
 |
 +-- policy(4)
 |     \ false -> gate(0x80)
 |
 +-- policy(1)
 |     \ false -> helper(455)
 |
 +-- policy(0)
 |     + true  -> helper(0x362c40,0,0)
 |     \ false -> helper(0x363288,0xE38)
 |
 +-- mode == 3 ?
 |      |
 |      +-- urgent branch: bypasses the seven-field non-urgent ladder
 |      |
 |      \-- normal branch:
 |           seven SBI field checks at 0x44c..0x45c
 |           -> fixed-point capacity guard
 |
 +-- policy(1)
 +-- policy(3)
 |
 +-- dirty/reservation comparison
 |
 +-- second fixed-point guard
 |
 +-- time/current-value guard
 |
 +-- mount_opt byte 0x4b9 bit7 ?
 |       |
 |       +-- no -> return
 |       \
 |        yes -> 0x3e1014 -> 0x34e224(...,1)
 |                 -> 0x3e1558
 |                 -> 0x341250(sb,1)
 |                 -> stat +0x16c++
 |
 +-- epilogue / return
```

## What is now proven

- `0x366cd4` has a real seven-field SBI guard ladder at `0x44c..0x45c`.
- `gc_mode == 3` selects a distinct branch before those seven fields are tested.
- The function uses the same `0x51EB851F >> 37` fixed-point family as the detector.
- Selector arguments to `0x35cc18` are exactly `4`, `1`, `0`, `1`, and `3` at the observed call sites.
- `0x35d22c` is reached with immediate `455`.
- `0x362c40` is reached with arguments `(sbi, 0, 0)`.
- `0x363288` is reached with `(sbi, 0xE38)`.
- The bit-7 path from `sbi + 0x4b9` leads through `0x3e1014`, `0x34e224`, `0x3e1558`, and `0x341250` before incrementing `stat_info + 0x16c`.

## Not promoted to symbolic names

The following remain intentionally anonymous:

- the original names of the seven `sbi + 0x44x` fields;
- the meaning of selector values 0/1/3/4;
- the exact symbolic identities of `0x35cc18`, `0x373108`, `0x35d22c`, `0x362c40`, `0x363288`, `0x3e1014`, `0x34e224`, `0x3e1558`, `0x341250`;
- the original name of `stat_info + 0x16c`.

This document intentionally records machine-code facts without converting plausible analogies into claimed source identities.
