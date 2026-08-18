# X683/H694 `curseg_array` resolution

Source authority: stock X683/H694 `f2fs_gc()` disassembly.

## `sm_info + 0x18`

The stock binary loads:

```asm
ldr x18, [sm_info, #0x18]
```

Historical 4.14 F2FS independently defines this field as:

```c
struct curseg_info *curseg_array;
```

so the X683 `sm_info + 0x18` member is strongly identified as the active-segment array pointer. citeturn297041search4turn297041search0

## X683 array geometry

The same `f2fs_gc()` path accesses:

```text
curseg_array + 0x1ac
curseg_array + 0x21c
curseg_array + 0x28c
curseg_array + 0x5c
```

The first three offsets differ by exactly `0x70`:

```text
0x1ac = 3 * 0x70 + 0x5c
0x21c = 4 * 0x70 + 0x5c
0x28c = 5 * 0x70 + 0x5c
```

Therefore the stock X683 layout strongly establishes:

```text
curseg_array[3] + 0x5c = word used by GC
curseg_array[4] + 0x5c = word used by GC
curseg_array[5] + 0x5c = word used by GC
curseg_array[0] + 0x5c = word used by GC
```

with a vendor `curseg_info` stride of **0x70 bytes**.

The six array entries correspond to the standard six active logs:

```text
0 = CURSEG_HOT_DATA
1 = CURSEG_WARM_DATA
2 = CURSEG_COLD_DATA
3 = CURSEG_HOT_NODE
4 = CURSEG_WARM_NODE
5 = CURSEG_COLD_NODE
```

Historical F2FS confirms this ordering. citeturn297041search7

## What the `+0x5c` word is

The GC path uses the member value as a per-current-log segment-related scalar and later uses the same member of element 0. The exact source-level X683 name of this `+0x5c` member is **not promoted to proven** yet.

Do not automatically call it `segno` solely from historical structure layout. The historical `struct curseg_info` has several segment-related fields and the X683 vendor structure is demonstrably larger (`0x70` stride), so the exact member must be matched from its writers/readers.

## Correction to old notes

The old description:

```text
dirty_info + 0x1ac
 dirty_info + 0x21c
 dirty_info + 0x28c
```

was incorrect.

These are accesses through:

```text
sm_info + 0x18 -> curseg_array
```

not through `dirty_info`.

The actual dirty-info candidate region remains:

```text
dirty_info +0x68..+0x7c
```

for the six `nr_dirty[]` counters, pending an independent X683 validation.

## Confidence

| Item | Confidence |
|---|---|
| `sm_info+0x18 = curseg_array` | High: binary register flow + historical structure match |
| six-entry active-log ordering | High: standard F2FS ordering + six distinct X683 elements |
| X683 per-entry stride = `0x70` | High: exact offset arithmetic |
| repeated member offset = `+0x5c` | High: direct arithmetic |
| exact symbolic name of `+0x5c` member | Unresolved |
