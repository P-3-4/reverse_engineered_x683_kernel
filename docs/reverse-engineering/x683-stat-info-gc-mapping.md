# X683/H694 `stat_info` GC mapping

Source authority: stock X683/H694 Image/disassembly.

## Pointer

```text
sbi + 0x568 -> struct f2fs_stat_info *
```

## Directly established standard GC counters

```text
stat +0x164  call_count
stat +0x18c  tot_segs
stat +0x190  data_segs
stat +0x194  node_segs
stat +0x198  bg_data_segs
stat +0x19c  bg_node_segs
```

### DATA branch

At `0x3521dc..0x352208`:

```c
stat->tot_segs++;
stat->data_segs++;
stat->bg_data_segs += background;
```

The segment-type branch is `w23 <= 2`, corresponding to DATA segment types in the target F2FS layout.

### NODE branch

At `0x350c1c..0x350c58`:

```c
stat->tot_segs++;
stat->node_segs++;
stat->bg_node_segs += background;
```

The segment-type branch is `w23 > 2`, corresponding to NODE types.

### Background flag is now directly proven

At function entry, the X683 four-argument `f2fs_gc()` stores argument `w2` at `sp+0xac`. The fourth-argument ABI is:

```c
w0 = sbi
w1 = sync
w2 = background
w3 = requested_segno
```

At `0x3507f4` the binary loads `sp+0xac`, tests bit 0, and on the background path writes literal `1` to `sp+0x118`. The later statistics sites at `0x35267c..0x3526b4` and `0x3521dc..0x352208` consume `sp+0x118` as the BG increment.

Therefore:

```text
sp+0x118 at these stat sites = background ? 1 : 0
```

This is binary-proven, not inferred from historical source.

## Vendor GC counter family: `+0x170..0x188`

This is distinct from the standard segment counters at `+0x18c..+0x19c`.

At `0x352654..0x3526b4`:

```c
stat->field_170++;

if (segment_type > 2) {
    stat->field_174++;
    stat->field_184 += background;
} else {
    stat->field_178++;
    stat->field_188 += background;
}
```

Thus the exact machine-level roles are:

```text
+0x170 = total vendor GC segment-event counter
+0x174 = NODE vendor GC segment-event counter
+0x178 = DATA vendor GC segment-event counter
+0x184 = NODE background-GC accumulation
+0x188 = DATA background-GC accumulation
```

The original proprietary member names are still not proven. Do not rename these to upstream `tot_blks`, `data_blks`, `node_blks`, `bg_*_blks`; the increment is once per segment/event, not per block.

The structure is conceptually similar to historical/modern F2FS `gc_secs/gc_segs` accounting families, but X683-specific member names remain vendor-defined and are intentionally left descriptive.

## Supporting flow

`sp+0xf0` is a separate per-call migration/freed counter. At `0x352654` it is combined with another local count into `w19`, then the vendor `+0x170` counter is incremented. `sp+0xf0` must not be confused with the background flag at `sp+0x118`.

## Other known stat offsets

```text
stat +0x1a0  derived/correlated with sbi +0x540
stat +0x1a8  derived/correlated with sbi +0x548
stat +0x1b0..0x1c4  six dirty-info-derived counters
stat +0x1c8..0x1f4  normalized/derived dirty statistics
stat +0x1f8..0x218  contiguous SBI-derived statistics
```

Exact widths and proprietary names remain unresolved.

## Safe reconstruction

```c
struct x683_stat_info {
    /* unresolved vendor-specific prefix */
    u32 call_count;                /* +0x164 */
    /* ... */
    u32 gc_seg_events_total;       /* +0x170 */
    u32 gc_node_seg_events;        /* +0x174 */
    u32 gc_data_seg_events;        /* +0x178 */
    u32 gc_node_bg_events;         /* +0x184 */
    u32 gc_data_bg_events;         /* +0x188 */
    u32 tot_segs;                  /* +0x18c */
    u32 data_segs;                 /* +0x190 */
    u32 node_segs;                 /* +0x194 */
    u32 bg_data_segs;              /* +0x198 */
    u32 bg_node_segs;              /* +0x19c */
};
```

The descriptive names above are reverse-engineering labels, not claims of original Transsion source names.
