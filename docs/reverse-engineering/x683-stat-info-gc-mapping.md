# X683/H694 `stat_info` GC mapping

Source authority: stock X683/H694 Image/disassembly.

## Pointer

```text
sbi + 0x568 -> struct f2fs_stat_info *
```

## Directly established GC counters

```text
stat +0x164  call_count: incremented once per GC call
stat +0x18c  tot_segs
stat +0x190  data_segs
stat +0x194  node_segs
stat +0x198  bg_data_segs
stat +0x19c  bg_node_segs
```

### Exact DATA branch

At `0x3521dc..0x352208` the binary performs:

```c
stat->tot_segs++;
stat->data_segs++;
stat->bg_data_segs += local_bg_gc_flag;
```

This branch is reached for segment types `0,1,2` (`w23 <= 2`), which are the DATA segment types in the target F2FS layout.

### Exact NODE branch

At `0x350c1c..0x350c58` the binary performs:

```c
stat->tot_segs++;
stat->node_segs++;
stat->bg_node_segs += local_bg_gc_flag;
```

This branch is reached for segment types `3,4,5` (`w23 > 2`), which are the NODE segment types.

Therefore the X683 mapping is now exact:

```text
+0x18c = tot_segs
+0x190 = data_segs
+0x194 = node_segs
+0x198 = bg_data_segs
+0x19c = bg_node_segs
```

The mapping matches the historical Android/common 4.14 `stat_inc_seg_count()` shape, but the X683 offsets are established independently from stock machine code. citeturn629759search0

### Exact call-count

At `0x35278c..0x352798`:

```c
stat->call_count++;
```

So `+0x164` is the X683 GC call-count field by direct behavior, not merely a candidate.

## GC block/data statistics: still being separated from segment counters

The earlier `+0x170/+0x174/+0x178/+0x184/+0x188` accesses are a **different counter family** from `+0x18c..+0x19c`.

Observed shape:

```text
+0x170  increment once in the shared segment-accounting path
+0x174  increment in NODE path; +0x184 accumulates a local 0/1-like value
+0x178  increment in DATA path; +0x188 accumulates the same local value
```

Do not currently rename these as `tot_blks/data_blks/node_blks`; the increments are segment-level and therefore do not match the ordinary `stat_inc_*_blk_count()` macros directly.

The most plausible remaining historical family is a vendor/GC-specific **GC section/call counter set**, but the exact X683 symbolic names require the surrounding branch/counter data-flow to be resolved.

## Other known stat offsets

```text
stat +0x1a0  corresponds to a counter copied/derived from sbi +0x540
stat +0x1a8  corresponds to a counter copied/derived from sbi +0x548
stat +0x1b0..0x1c4  six dirty-info-derived counters
stat +0x1c8..0x1f4  normalized/derived dirty statistics
stat +0x1f8..0x218  contiguous SBI-statistics copies/derived values
```

Exact widths and semantic names of this secondary region are not yet proven.

## `sp + 0x118` caveat

This stack slot is compiler-reused. It is a task/reference pointer earlier in the function and is later overwritten before the terminal GC-list cleanup path. Therefore the value used as the addend for `+0x184/+0x188` must be named from its local data-flow, not from the stack slot itself.

The addend is consistent with the historical background-GC increment shape, but its final producer should be treated as a separate data-flow target until traced to its source assignment.

## Safe reconstruction

```c
struct x683_stat_info {
    /* unresolved vendor-specific prefix */
    u32 call_count;      /* +0x164 */
    /* unresolved fields */
    u32 gc_counter_170;  /* +0x170, name unresolved */
    u32 gc_node_counter; /* +0x174 */
    u32 gc_data_counter; /* +0x178 */
    u32 gc_node_bg_acc;  /* +0x184 */
    u32 gc_data_bg_acc;  /* +0x188 */
    /* ... */
    u32 tot_segs;        /* +0x18c */
    u32 data_segs;       /* +0x190 */
    u32 node_segs;       /* +0x194 */
    u32 bg_data_segs;    /* +0x198 */
    u32 bg_node_segs;    /* +0x19c */
};
```

`gc_node_counter/gc_data_counter` are intentionally descriptive placeholders, not claims of original member names.
