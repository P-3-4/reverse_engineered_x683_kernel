# X683/H694 `stat_info` reconstruction

Reconstructed/inferred from the verified stock X683/H694 kernel Image. This is not proprietary Transsion source.

## Binary authority

Boot SHA-256:
`a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`

Decompressed Image SHA-256:
`96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

## 1. `stat_info` pointer

Direct stock GC code repeatedly loads:

```asm
ldr x8, [x20, #0x568]
```

with `x20 = struct f2fs_sb_info *`.

Therefore:

```text
sbi + 0x568 = struct f2fs_stat_info *
```

The pointed-to object must be analyzed separately from the `f2fs_sb_info` fields at `0x570+`.

## 2. Confirmed member accesses

### `+0x164`

At `0x35278c..0x352798`:

```asm
ldr x8, [x20, #0x568]
ldr w9, [x8, #0x164]
add w9, w9, #1
str w9, [x8, #0x164]
```

This is a direct monotonically incremented GC-path counter.

Semantic label:

```text
stat_info + 0x164 = GC call/accounting counter candidate
```

Historical `call_count` correspondence is plausible, but the exact X683 member name remains deferred until independently traced.

### `+0x170/+0x174/+0x178` and `+0x184/+0x188`

The GC completion path performs type-dependent accounting.

For one branch (`0x35267c..0x352698`):

```asm
ldr w9,  [x8, #0x174]
ldr w10, [x8, #0x184]
ldr w11, [sp, #0x118]
add w9,  w9, #1
add w10, w10, w11
str w9,  [x8, #0x174]
str w10, [x8, #0x184]
```

For the alternate branch (`0x35269c..0x3526b4`):

```asm
ldr w9,  [x8, #0x178]
ldr w10, [x8, #0x188]
ldr w11, [sp, #0x118]
add w9,  w9, #1
add w10, w10, w11
str w9,  [x8, #0x178]
str w10, [x8, #0x188]
```

Thus the object contains two clearly paired count/aggregate sets:

```text
+0x174  count A (incremented)
+0x184  aggregate A (+= w11)

+0x178  count B (incremented)
+0x188  aggregate B (+= w11)
```

The neighboring `+0x170` member is also written by the same statistics routine in the preceding basic block and belongs to the same vendor statistics family.

These are the machine-level equivalents of the historical segment/data GC accounting family, but exact historical member names must not be assigned until the X683 object layout is independently recovered.

### `+0x18c/+0x190/+0x198`

At `0x3521dc..0x352204`:

```asm
ldr x8,  [x20, #0x568]
ldr w12, [sp, #0x118]
ldr w9,  [x8, #0x18c]
ldr w10, [x8, #0x190]
ldr w11, [x8, #0x198]
add w9,  w9, #1
add w10, w10, #1
add w11, w11, w12
str w9,  [x8, #0x18c]
str w10, [x8, #0x190]
str w11, [x8, #0x198]
```

This is a second independent GC-completion accounting block.

Proven semantic shape:

```text
+0x18c = counter incremented once per qualifying completion
+0x190 = second counter incremented once per qualifying completion
+0x198 = aggregate accumulator, += local quantity w12
```

Exact names remain unresolved.

## 3. What this proves

The X683 `f2fs_stat_info` object is materially vendor-divergent from a public historical 4.14 structure. The binary contains several directly accessed counters around `0x164`, `0x170..0x198` that are not safe to map by historical offset alone.

The correct source reconstruction should therefore use explicit X683 layout definitions first, for example:

```c
struct x683_f2fs_stat_info {
    /* ... earlier members unresolved ... */
    u32 gc_call_count_candidate;     /* +0x164 */
    /* ... */
    u32 segment_count_a;             /* +0x170, semantic label pending */
    u32 segment_count_b;             /* +0x174 */
    u32 segment_count_c;             /* +0x178 */
    /* ... */
    u32 segment_aggregate_a;         /* +0x184 */
    u32 segment_aggregate_b;         /* +0x188 */
    u32 completion_count_a;          /* +0x18c */
    u32 completion_count_b;          /* +0x190 */
    /* +0x194 unresolved */
    u32 completion_aggregate;        /* +0x198 */
};
```

Those labels are reconstruction labels only, not claimed original names.

## 4. Still unresolved

- exact semantic names for `+0x164`, `+0x170..0x198`;
- all stat_info members before `+0x164`;
- all remaining members after `+0x198`;
- correlation of these counters with vendor debug controls (`gc_times`, `gc_segment_info`, `written_data`, etc.);
- whether any `sbi + 0x570..0x5dc` candidates duplicate historical background-GC counters.

## 5. Next target

Trace reads/logging of these exact stat_info offsets and correlate them with the vendor debug-control names. That should convert the current binary-defined counter map into named X683 fields and provide the final vendor statistics delta for `gc.c`.
