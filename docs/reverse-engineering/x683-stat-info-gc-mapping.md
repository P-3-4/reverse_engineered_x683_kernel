# X683/H694 `stat_info` GC mapping

Source authority: stock X683/H694 Image/disassembly.

## Pointer

```text
sbi + 0x568 -> struct f2fs_stat_info *
```

## Directly established members

The GC core loads the pointer from `sbi + 0x568` into `x8`, then accesses these fields directly.

```text
stat +0x164  GC call-count candidate; incremented once per GC call
stat +0x18c  total GC segment count
stat +0x190  per-type GC segment count A
stat +0x194  per-type GC segment count B
stat +0x198  background-GC counterpart to type A
stat +0x19c  background-GC counterpart to type B
stat +0x1a0  copy of sbi +0x540 skipped-atomic counter
stat +0x1a8  copy of sbi +0x548 skipped-atomic counter
stat +0x1b0..0x1c4  six dirty-info-derived counters
stat +0x1c8..0x1f4  normalized/derived dirty statistics
stat +0x1f8..0x218  contiguous SBI-statistics copies
```

## Exact arithmetic at the segment-accounting sites

At `0x350c1c..0x350c58` the binary performs:

```c
stat->field_18c++;
stat->field_194++;
stat->field_19c += local_gc_type_increment;
```

A parallel type branch at `0x3521dc..0x352208` performs:

```c
stat->field_18c++;
stat->field_190++;
stat->field_198 += local_gc_type_increment;
```

Therefore `0x18c` is the common total-segment counter, while `0x190/0x194` are the two mutually exclusive segment-type counters and `0x198/0x19c` are their background counters.

The exact DATA/NODE assignment of the A/B pair remains unresolved from the currently extracted snippets; it must be assigned only after the branch predicate immediately selecting each block is matched to `SUM_TYPE_DATA`/`SUM_TYPE_NODE` in the stock flow.

## Historical correspondence

Android/common 4.14 defines `stat_inc_seg_count()` in the following shape:

```c
si->tot_segs++;
if (type == SUM_TYPE_DATA) {
    si->data_segs++;
    si->bg_data_segs += (gc_type == BG_GC) ? 1 : 0;
} else {
    si->node_segs++;
    si->bg_node_segs += (gc_type == BG_GC) ? 1 : 0;
}
```

That macro shape matches the X683 access pattern exactly, but historical field ordering is used only as a correspondence aid; X683 offsets remain binary-defined. citeturn629759search0

The historical header also defines `stat_inc_call_count(si)` as the `call_count++` operation, matching the X683 increment at `stat + 0x164` in role. citeturn629759search9

## Safe reconstruction

Until the remaining branch predicate is resolved, use:

```c
struct x683_stat_info {
    /* ... unresolved prefix ... */
    u32 call_count;            /* +0x164, high-confidence role */
    /* ... */
    u32 tot_segs;              /* +0x18c */
    u32 seg_type_a;            /* +0x190 */
    u32 seg_type_b;            /* +0x194 */
    u32 bg_seg_type_a;         /* +0x198 */
    u32 bg_seg_type_b;         /* +0x19c */
    u64 skipped_atomic0;       /* +0x1a0, source width to be validated */
    u64 skipped_atomic1;       /* +0x1a8, source width to be validated */
};
```

Do not rename `seg_type_a/b` to DATA/NODE yet.
