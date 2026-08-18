# X683/H694 Transsion GC — Stop-condition producer deep pass

Source: supplied `boot.img`, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.

All offsets below are decompressed-kernel offsets.

## 1. Segment-manager object chain

At `0x377570` the detector obtains:

```text
sbi + 0x80 -> f2fs_sm_info
sm_info + 0x00 -> sit_info
sm_info + 0x08 -> free_info
sm_info + 0x10 -> dirty_info
```

The same three pointers are reloaded at `0x3775dc` / `0x37768c`.

This confirms the previously reconstructed segment-manager topology.

## 2. Exact producers

### `w20` / `x20`

At `0x37757c`:

```asm
ldr  w20, [sbi, #0x408]
```

At `0x377584`:

```asm
ldr  w21, [sbi, #0x3d8]
```

Then:

```asm
0x3775d4  lsr x25, x20, x21
0x3775d8  lsr x20, x23, x21
```

`w23` immediately before this shift is loaded from `sit_info + 0x10` at `0x377588`. In the stock SIT layout this is `sit_blocks`.

Therefore:

```c
user_segments = sbi->user_block_count >> sbi->log_blocks_per_seg;
sit_segments  = sit_info->sit_blocks >> sbi->log_blocks_per_seg;
```

At the Stop-3/Stop-5 region, `x20` is the `sit_segments` value.

### `w23`

The detector reloads `free_info` and `dirty_info` at `0x3775dc` and computes:

```asm
0x3775e4  ldr w9, [free_info, #0x4]
0x3775e8  ldr w8, [dirty_info, #0x84]
0x3775ec  add w23, w8, w9
```

The six 32-bit values at `dirty_info + 0x68..0x7c` are also summed at `0x37769c..0x3776c0`.

The historical `dirty_seglist_info` ordering has six per-log dirty counters followed by `DIRTY` and `PRE`; the binary access pattern fits:

```text
+0x68 .. +0x7c = six per-log dirty counters
+0x80          = nr_dirty[DIRTY]
+0x84          = nr_dirty[PRE]
```

So the detector's `w23` is:

```c
recoverable_segments = free_info->free_segments + dirty_info->nr_dirty[PRE];
```

This is the quantity used as the detector's free/prefree progress metric.

### `w21`

After `x25 = user_segments`, the stock code extracts a capacity bucket:

```asm
0x3775e0  ubfx x21, x25, #13, #19
```

Thus:

```c
bucket = (user_segments >> 13) & 0x7ffff;
```

The branch at `0x377604..0x377658` selects two values:

| Bucket | `w20` base | `w21` selected scale |
|---|---:|---:|
| 0 | `0x800` | `2 * 512 = 1024` |
| 1 | `0xc00` | `3 * 512 = 1536` |
| 2–3 | `0x1000` | `2 * 512 = 1024` |
| >=4 | `0x1800` | `4 * 512 = 2048` |

The `512` base is directly loaded from vendor/global data at `+0xd8c`.

After selection, `w21` is the Stop-1 threshold scale and `w20` is the Stop-2 threshold base.

## 3. Stop 1 — exact inputs

At `0x377660`:

```asm
ldr w2, [global, #0x9f4]
```

At `0x3776a4`:

```asm
sub w25, w23, w2
```

Therefore:

```c
delta1 = recoverable_segments - controller->saved_baseline;
```

The table at image offset `0x10a64e4` contains:

```text
100, 100, 100, 80, 80, 80, 60, 60, ...
```

At `0x3776d4..0x3776fc`, the selector is `max(global+0x890, global+0x894)`, restricted to indices `0..7`, then the table value is multiplied by selected `w21`.

The fixed-point calculation is:

```c
threshold1 = table[bucket] * w21 * 0x51EB851F >> 37;
```

The stock predicate is:

```c
if ((s32)delta1 > (s32)threshold1)
    stop_condition = 1;
```

## 4. Stop 2 — exact inputs

At `0x377728..0x377734`:

```asm
ldr x9, [sbi, #0x80]
ldr w9, [x9, #0x60]
sub w9, w23, w9
```

`sm_info + 0x60` is `reserved_segments`.

Therefore:

```c
delta2 = recoverable_segments - sm_info->reserved_segments;
```

The same capacity table is used. The unsigned fixed-point expression is:

```c
threshold2 = table[bucket] * w20 * 0x51EB851F >> 37;
```

The stock predicate is:

```c
if ((s32)delta2 > (s32)threshold2)
    stop_condition = 2;
```

## 5. Stop 3 — exact inputs

At `0x377774..0x377784`:

```asm
ldr x20, [sp, #0x30]
ldr x10, [sp, #0x18]
sxtw x9, w9
sub x10, x10, x20
```

The stack values were established earlier as:

```text
sp+0x18 = user_segments
sp+0x30 = sit_segments
```

So:

```c
span = user_segments - sit_segments;
reference = recoverable_segments - reserved_segments;
```

The second table at image offset `0xE74610` contains:

```text
80, 80, 80, 70, 70, 70, 60, 60
```

The detector multiplies the selected table value by `span` and applies the signed fixed-point sequence:

```asm
smulh x10, x8, 0xA3D70A3D70A3D70B
add   x8, x10, x8
asr   x10, x8, #6
add   x8, x10, x8, lsr #63
```

Then:

```asm
cmp x8, x9
b.lt Stop3
```

Thus Stop 3 is exactly:

```text
scaled(table2[bucket] * (user_segments - sit_segments))
    < (recoverable_segments - reserved_segments)
```

## 6. Stop 4 — corrected operand chain

At `0x3777d4`:

```asm
sub w1, w26, w23
```

`w26` is the running maximum of `recoverable_segments` maintained at `0x377668..0x377678`.

`x11` at `0x3777bc` is the saved baseline stored at `sp+0x28`; `sp+0x28` was initialized from the same `sit_segments` value at `0x3771c8`.

At `0x3777dc` the threshold is loaded from a separate vendor state object at `+0xd90`; the stock value in the supplied image is `20`.

The exact expression is therefore:

```c
s64 delta4 = (s64)((s32)(w26 - w23) + saved_sit_segments)
             - (s64)sit_segments;

if (delta4 > vendor_state_d90)
    Stop4;
```

The resulting controller transition remains:

```text
Stop4 -> if +0x9c0 == 0: +0x998 = 2
       -> +0x9f8 = 1
```

## 7. Stop 5 — exact operand chain

At `0x37785c..0x377874`:

```asm
ldr  w8, [global, #0xa0c]
ldrsw x9, [global, #0xa08]
sub  w8, w23, w8
add  x8, x20, w8, sxtw
cmp  x8, x9
```

Therefore:

```c
progress = sit_segments +
           (recoverable_segments - baseline_recoverable_segments);

if (progress <= baseline_segment)
    Stop5;
```

where:

```text
+a08 = signed baseline segment
+a0c = baseline recoverable/free-progress value
```

This replaces the earlier generic `current_progress <= 0` reconstruction.

## 8. Important correction to prior reconstruction

`w23` must no longer be described generically as a "written-segment count".

The direct stock producer is:

```c
w23 = free_info->free_segments + dirty_info->nr_dirty[PRE];
```

That makes it a free/prefree recovery-progress metric.

`x20` is likewise not a generic segment baseline:

```c
x20 = SIT_I(sbi)->sit_blocks >> sbi->log_blocks_per_seg;
```

which is the SIT-area segment count used as the reference in Stop 3 and the running expression in Stop 5.

## 9. Immediate next target

The next binary target is the detector's state/control logic around `0x3772e0..0x377494` and the helper calls surrounding `0x3770xx`.

That region already exposes:

- the initial `+0xa00` controller/arming state;
- the `+0xa04` cadence selector write;
- detector state writes 2/3;
- the conditions that enable the static detector;
- the call that produces the initial detector value.

This should be reconstructed before adding further assumptions about charger/USB/display triggers.
