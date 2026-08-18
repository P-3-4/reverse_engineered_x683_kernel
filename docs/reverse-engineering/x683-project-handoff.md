# X683 / H694 Kernel Reverse-Engineering — Canonical Project Handoff

Last consolidated: 2026-08-18

## Continuation rule

Work from `main` in `P-3-4/reverse_engineered_x683_kernel`.

Read this file first before continuing.

**User preference:** do the work, keep explanations minimal, do not ask unnecessary confirmation questions, and do not present inference as proof.

**Binary authority:** stock X683/H694 `boot.img` and its decompressed kernel Image. Public Android/Linux/F2FS sources are comparison references only.

## Current critical binary identity

Boot SHA-256:
`a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`

Decompressed Image SHA-256:
`96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

## Current architecture

```text
Transsion detector/controller
        |
        v
0x366cd4..0x366f2c vendor GC policy/orchestration
        |
        +--> controller wrapper 0x37ada8..0x37ae94
        |
        +--> actual X683 four-argument f2fs_gc() 0x3503a8
        |
        +--> terminal filesystem synchronization/balance path
```

## Correct `0x366cd4` boundary and policy

Live function:

```text
0x366cd4 .. 0x366f2c
```

`0x366f2c` is the stack-canary failure tail; next normal function begins at `0x366f30`.

First policy ladder:

```text
sbi+0x48 bit3 set -> return
policy(sbi,4) false -> 0x373108(sbi,0x80)
policy(sbi,1) false -> 0x35d22c(sbi,455)
policy(sbi,0) true  -> 0x362c40(sbi,0,0)
policy(sbi,0) false -> 0x363288(sbi,0xe38)
```

For `gc_mode != 3`, the seven-field discriminator is:

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

Thus:

```text
any nonzero -> active path 0x366da4
all zero    -> clean/alternate path 0x366ee0
gc_mode=3   -> bypass discriminator -> shared stage 0x366de4
```

The clean path can escalate back to `0x366da4` through nested manager/list state, then compares the second SBI quantity pair against the shared global at `Image+0x16c6980`.

## Seven-field writer status

The same seven fields are read both by `0x366cd4` and by the detector runtime guard after state-2 arming. Exact writer addresses are **not yet established** because the currently committed raw/disassembly artifacts do not cover enough of the Image for a complete store-reference sweep.

Keep them unnamed:

```text
sbi+0x444
sbi+0x448
sbi+0x44c
sbi+0x450
sbi+0x454
sbi+0x458
sbi+0x45c
```

Do not rename them from historical 4.14 structure layouts without exact offset matching.

Detailed producer/consumer record:
`docs/reverse-engineering/x683-policy-field-producer-consumer-trace.md`

## `Image+0x16c6980`

One 64-bit vendor policy global is consumed in both `0x366cd4` branches:

```text
active/shared:
    250*(sbi+0x1c8) + (sbi+0x198) vs global

clean/alternate:
    250*(sbi+0x1d0) + (sbi+0x1a0) vs global
```

It is therefore a shared policy threshold/reference. Exact producer/write location and source symbol remain unresolved.

Neutral label:

```text
x683_gc_policy_global_16c6980
```

Do not call it timeout/jiffies/charger/segment threshold without producer evidence.

## Terminal path

At `0x366e7c`:

```text
if (sbi+0x4b9) bit7:
    0x3e1014(stack_object)
    0x34e224(sbi,1)
    0x3e1558(stack_object)

always once terminal path reached:
    0x341250(sbi->sb,1)
    stat_info = *(sbi+0x568)
    stat_info+0x16c++
```

Historical 4.14 structure correlation strongly supports `stat_info+0x16c == dirty_count`, but X683 source-level confirmation is still pending.

`0x341250` remains an anonymous terminal filesystem synchronization/balance helper; call shape is consistent with `f2fs_sync_fs(sb,1)` but the stock X683 body must be used for final symbol promotion.

## Controller mapping

The one-argument Transsion wrapper proves:

```text
controller 0 -> f2fs_gc(), gc_mode unchanged
controller 1 -> temporary gc_mode=2
controller 2 -> temporary gc_mode=3
```

Vendor mode strings establish `2=URGENT`, `3=GREEDY`; therefore Stop-4/5's raw controller write `2` selects the temporary GREEDY path.

## Current honest status

High confidence:

```text
X683 f2fs_gc() four-argument boundary
tran_f2fs_gc() controller mapping
0x366cd4 full live branch topology
seven-field consumer role
Image+0x16c6980 consumer role
terminal stat_info access
```

Still unresolved:

```text
writers of the seven 0x444..0x45c fields
producer of Image+0x16c6980
exact source names for 0x35d22c/0x362c40/0x363288/0x34e224/0x341250
original names of seven fields
exact X683-vs-stock gc.c delta
buildability against exact X683/H694 source tree
```

The stock kernel payload currently committed is gzip-compressed; the connected binary fetch path cannot decode it as UTF-8. A byte-capable copy of the decompressed Image is required to finish the producer/writer sweep without guessing.
