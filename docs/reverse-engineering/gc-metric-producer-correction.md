# X683/H694 GC metric-producer correction

Date: 2026-08-18

## Result

The detector uses **two different segment metrics**. Earlier scaffold code incorrectly treated them as one `recoverable_segments` input.

### Static arming metric

The arming region around `0x377120..0x377494` uses the six per-log dirty counters:

```c
arming_dirty_segments =
    dirty_i->nr_dirty[DIRTY_HOT_DATA] +
    dirty_i->nr_dirty[DIRTY_WARM_DATA] +
    dirty_i->nr_dirty[DIRTY_COLD_DATA] +
    dirty_i->nr_dirty[DIRTY_HOT_NODE] +
    dirty_i->nr_dirty[DIRTY_WARM_NODE] +
    dirty_i->nr_dirty[DIRTY_COLD_NODE];
```

For the X683 binary these counters occupy `dirty_info + 0x68 .. +0x7c`.

The arming ratio is reconstructed as:

```c
ratio = ((free_segments + arming_dirty_segments - main_segments)
         << log_blocks_per_seg)
        + (sit_blocks - user_block_count);

denominator = arming_dirty_segments + free_segments;
w21 = ratio / denominator;
```

The detector also checks `arming_dirty_segments > user_segments / 10`, `w21 >= 0x15f`, the free/main-segment relationship, and the two vendor capacity guards described in `tran-gc-detector-arming-deep-pass.md`.

### Stop-condition metric

The Stop 1..5 region uses `w23`, whose direct producer is:

```c
recoverable_segments =
    free_i->free_segments + dirty_i->nr_dirty[PRE];
```

For the recovered X683 layout, `nr_dirty[PRE]` is at `dirty_info + 0x84`.

This metric is used by:

```c
delta1 = recoverable_segments - saved_baseline;
delta2 = recoverable_segments - reserved_segments;
delta4 = (running_max - recoverable_segments)
       + saved_sit_segments - sit_segments;
progress = sit_segments
         + (recoverable_segments - baseline_recoverable);
```

The Stop-3 reference remains:

```c
reference = recoverable_segments - reserved_segments;
span = user_segments - sit_segments;
```

## Source update

`fs/f2fs/tran_gc_thread_reconstructed.c` now models these metrics separately instead of accepting one ambiguous metric from the caller.

## Confidence

- Six-counter arming metric: high; direct binary access and historical `dirty_seglist_info` layout agree.
- `free_segments + nr_dirty[PRE]` Stop metric: high; direct `w23` producer is recovered at `0x3775e4..0x3775ec`.
- Exact proprietary source-level member names: still treated conservatively because the repository does not contain the original X683 vendor headers.

This correction supersedes the earlier scaffold assumption that the detector had one generic recoverable/written-segment input.
