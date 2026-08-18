# X683/H694 — `0x366cd4` vendor GC policy/orchestration

## Authority

Fresh analysis of the stock X683/H694 Image extracted from the supplied boot image. This is binary-derived reconstruction, not recovered proprietary Transsion source.

Boot SHA-256:

`a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`

Image SHA-256:

`96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

## 1. Correct function boundary

```text
0x366cd4 .. 0x366f2c
```

`0x366f2c` is the stack-canary failure tail. The next normal function begins at `0x366f30`.

This supersedes the older `0x366cd4..0x366edc` boundary.

## 2. Entry / first policy ladder

```text
sbi + 0x48 bit3
    set -> return

policy(sbi,4)
    false -> 0x373108(sbi,0x80)

policy(sbi,1)
    false -> 0x35d22c(sbi,455)

policy(sbi,0)
    true  -> 0x362c40(sbi,0,0)
    false -> 0x363288(sbi,0xe38)
```

## 3. Seven-field discriminator

For `gc_mode != 3`:

```text
+0x44c != 0 -> 0x366da4
+0x450 != 0 -> 0x366da4
+0x454 != 0 -> 0x366da4
+0x448 != 0 -> 0x366da4
+0x444 != 0 -> 0x366da4
+0x45c != 0 -> 0x366da4
+0x458 == 0 -> 0x366ee0
+0x458 != 0 -> 0x366da4
```

Therefore these seven fields are **not** an all-zero prerequisite for normal execution. They discriminate between:

```text
any nonzero -> active guarded path
all zero    -> clean/alternate path
```

`gc_mode == 3` bypasses this discriminator and enters at `0x366de4`.

The seven original source-level field names remain unresolved.

## 4. Active guarded path

At `0x366da4` the binary computes:

```text
obj = *(sbi + 0x70)
scaled = obj[0x04] * obj[0x18] * 0x51EB851F >> 37
current = obj[0x80]
```

Then:

```text
current >= scaled
    -> 0x366de4

current < scaled AND
sbi+0x434 < 8*(u32)sbi+0x3dc
    -> return

otherwise
    -> 0x366de4
```

The reciprocal multiply/shift sequence is byte-proven.

## 5. Shared secondary policy

At `0x366de4`:

```text
policy(sbi,1) == false -> terminal
policy(sbi,3) == false -> terminal
```

Then:

```text
obj    = *(sbi + 0x80)
nested = *(obj + 0x10)
left   = *(u32 *)(obj + 0x64)
right  = *(u32 *)(nested + 0x84)

right > left -> terminal
```

A second fixed-point capacity test and `sbi+0x434` versus `8*sbi+0x3dc` comparison follow.

Finally:

```text
value  = 250 * sbi+0x1c8 + sbi+0x198
global = *(u64 *)(Image + 0x16c6980)

value >= global -> return
value < global  -> terminal
```

## 6. Clean/alternate path

The all-zero seven-field case reaches `0x366ee0`.

The object at `sbi+0x80` is checked:

```text
obj = *(sbi + 0x80)
child = *(obj + 0xa0)
child[0x2090] != 0 -> 0x366da4

list = *(obj + 0x98)
list[0x24] != 0 -> 0x366da4
```

Otherwise:

```text
value  = 250 * sbi+0x1d0 + sbi+0x1a0
global = *(u64 *)(Image + 0x16c6980)

value >= global -> 0x366da4
value < global  -> 0x366de4
```

This tail was previously missing from the documented function.

## 7. Terminal path

The terminal path begins at `0x366e7c`.

Bit 7 of `sbi+0x4b9` controls only the TLS/list helpers:

```text
bit7 set:
    0x3e1014(stack_object)
    0x34e224(sbi,1)
    0x3e1558(stack_object)
```

Important correction: `0x34e224` receives the **SBI pointer**, not the stack object.

Then, unconditionally on the terminal path:

```text
0x341250(sbi->sb,1)
stat_info = *(sbi+0x568)
stat_info+0x16c++
```

Thus `0x341250` and the `+0x16c` statistic are not conditional on mount-byte bit7 once the terminal branch is entered.

## 8. Helper classifications

### `0x35cc18`

Selector dispatch is directly established:

```text
0 -> 0x35cc7c
1 -> 0x35ccc8
2 -> 0x35cd14
3 -> 0x35cd2c
4 -> 0x35cd78
5 -> 0x35cdb4
```

The selector names remain anonymous.

### `0x373108`

Tests `sbi+0x4b9` bit5 before entering a larger vendor threshold/accounting routine.

### `0x3e1014`

TLS-associated temporary object/list initialization helper.

### `0x3e1558`

TLS-associated temporary object/list reset helper.

### `0x34e224`

Global/list callback-dispatch helper invoked as `(sbi,1)`.

### `0x341250`

Terminal filesystem synchronization/balance-style helper invoked as `(sbi->sb,1)`. Exact original source name is not promoted.

## 9. Controller mapping sanity check

The independent wrapper bytes prove:

```text
controller 0 -> gc_mode unchanged
controller 1 -> temporary gc_mode 2
controller 2 -> temporary gc_mode 3
```

The vendor mode strings establish:

```text
2 = URGENT
3 = GREEDY
```

Therefore Stop-4/5's raw controller write of `2` causes the temporary **GREEDY** path.

## 10. Final call graph

```text
tran_gc detector
      |
      v
0x366cd4 vendor policy/orchestration
      |
      +-- entry freeze/state guard
      +-- selector 4/1/0 helper ladder
      +-- gc_mode == 3 ? --------------------+
      |                                       |
      | no                                    | yes
      v                                       v
 seven-field discriminator                0x366de4
      |                                    shared stage
      +-- active path 0x366da4
      |       |
      |       +-- fixed-point guard
      |       +-- reservation guard
      |       +-- secondary stage
      |
      +-- clean path 0x366ee0
              |
              +-- manager/list escalation -> 0x366da4
              +-- time gate -> 0x366de4

shared stage
      |
      +-- policy 1/3
      +-- nested reservation comparison
      +-- fixed-point/reservation gates
      +-- time/current gate
      +-- direct return OR terminal

terminal
      |
      +-- optional TLS trio when mount bit7 set
      +-- 0x341250(sbi->sb,1) always
      +-- stat_info+0x16c++
```

## 11. Remaining uncertainty

Still unresolved and intentionally not guessed:

```text
selector symbolic names
0x35d22c original name
0x362c40 original name
0x363288 original name
0x34e224 original name
0x341250 original name
seven guard-field source names
stat_info +0x16c original member name
```

The branch topology and call arguments are now byte-checked against the fresh stock image.
