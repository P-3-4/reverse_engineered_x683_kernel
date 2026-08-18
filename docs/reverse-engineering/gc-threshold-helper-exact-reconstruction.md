# X683/H694 GC threshold/helper exact reconstruction

Directly reconstructed from the verified stock Image, SHA-256:
`96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`.

## Range

`0x37b580..0x37b8c0`

This range contains several vendor GC/statistics helpers. Only labels supported by direct behavior are used below.

## 0x37b580 — fragmentation-percentage calculator/log path

Stock arithmetic:

```c
user_segments = sbi->user_block_count >> sbi->log_blocks_per_seg;
sit_blocks = *(u32 *)((char *)SM_I(sbi)->sit_info + 0x10);
sit_segments = sit_blocks >> sbi->log_blocks_per_seg;
span = user_segments - sit_segments;
free_segments = *(u32 *)((char *)SM_I(sbi)->free_info + 0x04);
free_percent = (free_segments * 100) / span;
fragmentation = 100 - free_percent;
```

The routine logs the result with the literal:

```text
"f2fs alloc new segment and fragmentation is %lu"
```

The exact source-level function name is not proven from the binary alone. Reconstruction label:

```text
x683_calc_fragmentation_percent()
```

## 0x37b5d4..0x37b748 — boolean free-space/fragmentation policy helper

This helper is directly called at:

```text
0x377104
0x377de8
```

and returns `1` for either of two predicates; otherwise it reaches the diagnostic logging path and returns `0`.

The first selector inputs are:

```c
sm = SM_I(sbi);
sit = sm->sit_info;
free_i = sm->free_info;

user_segments = sbi->user_block_count >> sbi->log_blocks_per_seg;
sit_segments = sit_blocks >> sbi->log_blocks_per_seg;
free_segments = free_i->field_0x04;
reserved_segments = sm->reserved_segments;
```

### Scale selection — corrected exact form

The stock code does **not** use a single `(user_segments >> 13) & 0x7ffff` bucket formula.

It does:

```c
if ((user_segments >> 15) == 0)
    scale = small_scale[user_segments >> 13];
else
    scale = 0x1800;
```

The small-scale table at Image `+0x4d4` is:

```text
index 0: 0x0800
index 1: 0x0c00
index 2: 0x1000
index 3: 0x1000
```

The `user_segments >> 15` gate guarantees that the table index is only 0..3.

### Vendor selector

Stock loads two vendor bytes at Image:

```text
0x1a13890
0x1a13894
```

and selects:

```c
selector = max(byte_a, byte_b);
```

Then:

```c
factor = selector <= 7 ? factor_table[selector] : 0;
```

Factor table at Image `+0x4e4`:

```text
{ 100, 100, 100, 80, 80, 80, 60, 60 }
```

### First predicate — Stop 2

```c
delta = (s32)(free_segments - reserved_segments);
threshold = (factor * scale * 0x51EB851F) >> 37;

if (delta > (s32)threshold)
    return 1;
```

Important fixed-point correction:

```text
0x51EB851F / 2^37 ~= 0.01
```

Therefore the threshold is approximately:

```text
factor 100 -> 1.0 * scale
factor  80 -> 0.8 * scale
factor  60 -> 0.6 * scale
```

### Second predicate — Stop 3

The binary then computes:

```c
span = (s64)user_segments - (s64)sit_segments;
reference = (s64)delta;
```

It selects the 64-bit table at Image `0xe74610`:

```text
{ 80, 80, 80, 70, 70, 70, 60, 60 }
```

and performs the exact signed fixed-point sequence:

```c
p = factor64 * span;
high = smulh(p, 0xA3D70A3D70A3D70B);
v = (high + p) >> 6;
v += (p < 0);

if (v < reference)
    return 1;
```

The reciprocal constant is:

```text
0xA3D70A3D70A3D70B / 2^64 = 0.64
```

and the compiler sequence implements the corresponding signed fixed-point division/scaling operation.

### Diagnostic fall-through — 0x37b6d8..0x37b748

When neither predicate matches, stock calculates another threshold using the same `factor/scale` machinery and a second 4-byte table at Image `+0x504`:

```text
{ 80, 80, 80, 70, 70, 70, 60, 60 }
```

It then logs:

```text
"free_segment=%d, fix_size=%d, left_space=%d(precent:%d)"
```

with the currently-live free-space/threshold/span/table-derived values and returns 0.

This path is important: `0x37b6d8` is not a separate independent function boundary. It is reached after the early-return predicates and continues into the logging block ending at `0x37b748`.

## Function identity

The previous project notes named the subsystem helpers:

```text
tran_has_enough_free_segment()
is_f2fs_fragmentation()
```

The binary gives a stronger binding for this range:

```text
0x37b580
    = fragmentation percentage/log calculator

0x37b5d4..0x37b748
    = boolean free-space/fragmentation policy helper
    = direct caller of the Stop-2 / Stop-3 logic
```

The exact proprietary source names are still not recoverable from these instructions alone. Use reconstructed names, not claimed original names.

## 0x37b74c and later

The remainder of the requested range transitions into generic vendor control/attribute helpers rather than additional Stop-1/2/3 arithmetic:

```text
0x37b74c -> helper 0x38ba24
0x37b76c -> iterates a descriptor/list and calls 0x37bedc
0x37b7cc -> reads a nested bit and returns it
0x37b7e0 / 0x37b844 -> type-gated operation wrappers
0x37b8a8 -> calls 0x03bb48 with argument 21, returns bit0
0x37b8c4 -> copies a byte from object +0x268 to optional output and returns 1
0x37b8d8 -> modifies object +0x268 after an operation check
```

These should be treated as separate generic control/attribute helpers unless a direct GC semantic use is proven.

## Corrections to earlier notes

1. The single-bucket description `(user_segments >> 13) & 0x7ffff` was too coarse for this helper. The stock code explicitly uses a `>>15` high-bucket test and only indexes the small table when that value is zero.

2. `0x51EB851F` implements an approximately 1% fixed-point scale before the factor is applied; it is not itself a 2.5% constant.

3. The ~2.5% behavior comes from the **signed 64-bit Stop-3 sequence** using `0xA3D70A3D70A3D70B` and the final `>>6`, with the 80/70/60 table adjusting that percentage.
