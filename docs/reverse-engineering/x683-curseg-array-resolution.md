# X683/H694 `curseg_array` resolution

Source authority: stock X683/H694 `f2fs_gc()` disassembly.

## `sm_info + 0x18`

The stock binary loads:

```asm
ldr x18, [sm_info, #0x18]
```

Historical 4.14 F2FS independently identifies this member as:

```c
struct curseg_info *curseg_array;
```

so the X683 `sm_info + 0x18` member is strongly identified as the active-segment array pointer. citeturn297041search4turn297041search0

## X683 array geometry

The stock GC path accesses these words:

```text
curseg_array +0x5c
curseg_array +0x1ac
curseg_array +0x21c
curseg_array +0x28c
```

The offsets satisfy:

```text
0x1ac = 0x5c + 3*0x70
0x21c = 0x5c + 4*0x70
0x28c = 0x5c + 5*0x70
```

Therefore the X683 active-log array has a **0x70-byte per-entry stride**, and `+0x5c` is the same member position in every entry.

The six entries use the standard F2FS active-log ordering:

```text
0 = CURSEG_HOT_DATA
1 = CURSEG_WARM_DATA
2 = CURSEG_COLD_DATA
3 = CURSEG_HOT_NODE
4 = CURSEG_WARM_NODE
5 = CURSEG_COLD_NODE
```

Historical F2FS confirms this ordering. citeturn297041search7

## `curseg_info +0x5c` is now proven `segno`

At the GC selector, the binary performs:

```asm
ldr w18, [x18, #0x5c]
...
madd x18, w18, x26, x0
ldrh w18, [x18, #0x2]
```

where `x0` is the SIT/sentries base and `w18` is the value loaded from the active-segment entry. The value is therefore a **segment number used to index the SIT entry array**.

The same pattern is repeated for the node-log entries at `+0x1ac`, `+0x21c`, and `+0x28c`.

Therefore:

```text
curseg_array[0].segno = +0x5c
curseg_array[3].segno = +0x1ac
curseg_array[4].segno = +0x21c
curseg_array[5].segno = +0x28c
```

and the same `+0x5c` member exists in entries 1 and 2 by the recovered 0x70 stride.

## Correction to old notes

These offsets are **not** members of `dirty_info`:

```text
dirty_info +0x1ac
 dirty_info +0x21c
 dirty_info +0x28c
```

The actual object flow is:

```text
sbi +0x80
   -> sm_info
      +0x18
         -> curseg_array
            +0x5c / +0x1ac / +0x21c / +0x28c
               -> current segment numbers
```

The dirty-info candidate region remains separate:

```text
dirty_info +0x68..+0x7c
```

for the six `nr_dirty[]` counters, pending independent X683 validation.

## Source-level reconstruction boundary

Only the proven member is promoted into the X683 structure model:

```c
struct x683_curseg_info {
    /* vendor-specific fields/padding */
    u8 _x683_unknown[0x5c];
    u32 segno; /* +0x5c, proven */
    /* remaining vendor-specific tail to 0x70 */
};
```

Do not substitute the compact public `struct curseg_info` layout wholesale: the X683 active-log stride is demonstrably `0x70` bytes.

## Confidence

| Item | Confidence |
|---|---|
| `sm_info+0x18 = curseg_array` | High |
| six active-log ordering | High |
| X683 per-entry stride = `0x70` | High |
| `curseg_info +0x5c = segno` | **High / direct SIT-index data flow** |
| exact remaining X683 `curseg_info` members | Unresolved |
