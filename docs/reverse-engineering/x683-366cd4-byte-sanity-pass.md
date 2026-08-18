# X683/H694 `0x366cd4` byte-level sanity pass

Date: 2026-08-18

## Authority

Fresh re-analysis of the supplied stock `boot(8).img`.

- Boot SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- Boot size: `33,554,432` bytes
- Kernel compressed offset: `0x800`
- Decompressed Image size: `26,615,820` bytes
- Decompressed Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

The compressed kernel was decompressed from the gzip member beginning at `0x800` and stopped at gzip EOF; trailing boot-slot bytes were not fed to the decompressor.

This pass exists to supersede earlier prose whenever the branch-level interpretation differs.

## 1. Correct function boundary

`0x366cd4` is not a complete function ending at `0x366edc`.

The live function continues through:

```text
0x366ee0 .. 0x366f28
```

with the stack-canary failure call at `0x366f2c`, followed by the next function beginning at `0x366f30`.

Thus the safe source-level boundary is:

```text
0x366cd4 .. 0x366f2c
```

where `0x366f2c` is the failure tail rather than ordinary fall-through code.

## 2. First policy ladder

```text
sbi + 0x48 bit 3 set
    -> return

policy(sbi, 4)
    false -> 0x373108(sbi, 0x80)

policy(sbi, 1)
    false -> 0x35d22c(sbi, 455)

policy(sbi, 0)
    true  -> 0x362c40(sbi, 0, 0)
    false -> 0x363288(sbi, 0xE38)
```

Selector meanings remain anonymous. The branch polarity and call arguments are byte-proven.

## 3. Seven-field discriminator — corrected

For `gc_mode != 3`, the binary evaluates:

```text
+0x44c -> nonzero -> 0x366da4
+0x450 -> nonzero -> 0x366da4
+0x454 -> nonzero -> 0x366da4
+0x448 -> nonzero -> 0x366da4
+0x444 -> nonzero -> 0x366da4
+0x45c -> nonzero -> 0x366da4
+0x458 -> zero    -> 0x366ee0
          nonzero -> 0x366da4
```

Therefore this is **not** an all-zero guard that permits the normal path.

It is a branch discriminator:

```text
any of the seven fields nonzero -> active guarded path (0x366da4)
all seven zero                 -> clean/alternate path (0x366ee0)
```

`gc_mode == 3` skips this discriminator and enters at `0x366de4`.

## 4. Active guarded path (`0x366da4`)

The first fixed-point predicate is exactly:

```text
obj = *(sbi + 0x70)
a = obj + 0x04
b = obj + 0x18
current = obj + 0x80
scaled = a * b * 0x51EB851F >> 37
```

Then:

```text
current >= scaled
    -> secondary stage (0x366de4)

current < scaled
    and sbi+0x434 < 8*(u32 sbi+0x3dc)
    -> immediate return

otherwise -> secondary stage
```

The constant `0x51EB851F` and the shift are byte-proven here.

## 5. Secondary stage (`0x366de4`)

```text
policy(sbi,1)
    false -> terminal path 0x366e7c

policy(sbi,3)
    false -> terminal path 0x366e7c
```

Then the nested comparison:

```text
obj = *(sbi + 0x80)
nested = *(obj + 0x10)
left = *(u32 *)(obj + 0x64)
right = *(u32 *)(nested + 0x84)

right > left
    -> terminal path
```

The fixed-point predicate is repeated, followed by the same `sbi+0x434` versus `8*sbi+0x3dc` comparison.

The final time/current comparison is:

```text
value = 250 * sbi+0x1c8 + sbi+0x198
global = *(u64 *)(Image + 0x16c6980)

value >= global
    -> direct return
```

If the comparison is below the global, control reaches the terminal path.

## 6. Clean/alternate path (`0x366ee0`)

This path is reached only when the seven-field discriminator is all zero.

The binary examines the object at `sbi + 0x80`:

```text
obj = *(sbi + 0x80)
child = *(obj + 0xa0)
child[0x2090] != 0 -> 0x366da4

list = *(obj + 0x98)
list[0x24] != 0 -> 0x366da4
```

If neither escalation condition fires, it evaluates:

```text
value = 250 * sbi+0x1d0 + sbi+0x1a0
global = *(u64 *)(Image + 0x16c6980)

value >= global
    -> 0x366da4
value < global
    -> 0x366de4
```

This is a genuine second policy branch and was previously omitted from the documented function boundary.

## 7. Terminal path (`0x366e7c`)

The terminal path begins regardless of whether `sbi + 0x4b9` bit 7 is set.

Bit 7 controls only the TLS/list helper trio:

```text
if (sbi + 0x4b9) bit7:
    0x3e1014(stack_object)
    0x34e224(sbi, 1)
    0x3e1558(stack_object)
```

Important correction:

```text
0x34e224 receives x0 = sbi, not stack_object.
```

Then, unconditionally on the terminal path:

```text
0x341250(sbi->sb, 1)
stat_info = *(sbi + 0x568)
stat_info + 0x16c++
```

So `0x341250` and `stat_info +0x16c` are **not** conditional on mount-byte bit 7 once the terminal path is reached.

## 8. Helper classifications from fresh bytes

### `0x35cc18`

Verified selector dispatch table for values `0..5`:

```text
0 -> 0x35cc7c
1 -> 0x35ccc8
2 -> 0x35cd14
3 -> 0x35cd2c
4 -> 0x35cd78
5 -> 0x35cdb4
```

Selectors are retained as numeric policy modes. Their proprietary names are not invented.

### `0x373108`

Begins by testing `sbi + 0x4b9` bit 5; the remainder is a substantial vendor accounting/threshold routine. It is not merely a byte accessor.

### `0x3e1014`

Initializes a temporary TLS-associated object when the current SP_EL0 slot is empty.

### `0x3e1558`

Matches/resets the TLS-associated object and clears the SP_EL0 slot through the paired reset helper.

### `0x34e224`

Called with `(sbi, 1)` from this policy function and dispatches through a callback/list structure. Exact source identity remains unresolved.

### `0x341250`

Called with `(sbi->sb, 1)`. Its body treats the first argument as a superblock and later reaches F2FS state/write/balance machinery. It is therefore retained as an anonymous terminal filesystem synchronization/balance helper; no source symbol is promoted from argument shape alone.

## 9. Controller sanity check

Fresh wrapper bytes reconfirm:

```text
controller 0 -> f2fs_gc(...), mode unchanged
controller 1 -> temporary gc_mode = 2
controller 2 -> temporary gc_mode = 3
```

Therefore Stop-4/Stop-5 controller writes of `2` produce the temporary `gc_mode=3` path.

## 10. Sanity-check conclusions

Corrected:

- function boundary extended through `0x366f2c`;
- seven-field logic changed from “all-zero requirement” to a true active-vs-clean discriminator;
- clean tail `0x366ee0..0x366f28` recovered;
- `0x34e224` argument corrected to `sbi`;
- `0x341250(sbi->sb,1)` made unconditional within the terminal path;
- `stat_info+0x16c++` likewise made unconditional within the terminal path;
- policy global corrected to Image `+0x16c6980`.

Still intentionally unresolved:

```text
original names for selector 0/1/2/3/4/5
0x35d22c source identity
0x362c40 source identity
0x363288 source identity
0x34e224 source identity
0x341250 source identity
stat_info +0x16c source member name
seven guard field source names
```

These require source-tree correlation or stronger symbol/data-flow evidence, not guesswork.
