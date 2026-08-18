# X683 / H694 Reverse Engineering Continuation Handoff — GC Deep Pass

Date: 2026-08-18

Parent handoff:
`docs/reverse-engineering/x683-project-handoff.md`

## Current target

Deep reconstruction of:

```text
0x3503a8  X683 four-argument f2fs_gc()
0x362c40  vendor GC execution wrapper
0x35cc18  selector engine
SBI GC policy fields
```

## Confirmed

### 0x3503a8 ABI

Recovered prototype:

```c
int f2fs_gc_x683(
    struct f2fs_sb_info *sbi,
    int sync,
    int background,
    int type
);
```

ARM64:

```text
x0 = sbi
w1 = sync
w2 = background
w3 = GC type
```

The fourth argument is a Transsion extension.

Current mapping:

```text
0 = normal/background
1 = forced foreground
2 = urgent
3 = greedy
```

## Important correction

`0x3503a8` is not pure historical F2FS GC entry.

It is:

```text
Transsion GC admission/front-end
        |
        v
victim preparation
        |
        v
historical F2FS GC core
        |
        v
vendor cleanup/accounting
```

## 0x3503a8 entry findings

Early execution performs vendor state/admission work before victim selection.

Observed GC-related SBI inputs:

```text
+0x428
+0x434
+0x440
+0x3d8
+0x3dc
+0x3e0
```

Do NOT assign final names yet.

Current safe names:

```c
gc_policy_428
gc_policy_434
gc_policy_440
gc_threshold_3d8
gc_threshold_3dc
gc_threshold_3e0
```

## Selector engine final semantic names

0x35cc18:

```c
bool tran_gc_selector(struct f2fs_sb_info *sbi, int selector);
```

Recovered selectors:

```text
0 = age/activity pressure
1 = GC pressure threshold
2 = disable/state gate
3 = resource pressure
4 = IO pressure
5 = free segment pressure
```

Used by 0x366cd4:

```text
selector 4 -> IO pressure gate
selector 1 -> GC pressure gate
selector 0 -> age pressure gate
selector 3 -> resource pressure gate
selector 5 -> free segment pressure gate
```

## Sanity corrections

Do not claim:

```text
0x34e5d0 = confirmed get_victim()
```

Current label:

```text
0x34e5d0 = victim-related GC preparation helper
```

Do not claim exact pressure formula yet.

Only confirmed:

```text
multiple GC policy fields
fixed-point/scaled comparisons
threshold admission logic
```

## Current architecture

```text
Transsion detector/thread
        |
        v
tran_f2fs_gc()
        |
        v
0x366cd4 policy
        |
        v
0x362c40 execution wrapper
        |
        v
0x3503a8 X683 f2fs_gc()
        |
        v
victim preparation
        |
        v
historical F2FS migration core
        |
        v
vendor cleanup/statistics
```

## Next work order

1. Recover 0x34e5d0 completely.
2. Recover exact victim selection.
3. Recover do_garbage_collect/migration helpers.
4. Recover Transsion controller thread/state machine.
5. Complete SBI structure names.

## Confidence

```text
0x3503a8 ABI              95%
selector engine           95%
GC wrapper architecture   90%
SBI GC field region       85%
victim helper identity    50%
exact pressure formula    55%
```
