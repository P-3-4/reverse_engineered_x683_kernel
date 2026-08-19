# X683 Kernel Reverse Engineering — Current State Snapshot

Date: 2026-08-19
Repository: P-3-4/reverse_engineered_x683_kernel
Canonical working branch: `kernel-reconstruction-current`

## Completed

The F2FS segment-manager phase is complete enough to move on. Reconstructed against the stock X683 binary: `f2fs_sm_info`, `sit_info`, `free_segmap_info`, `dirty_seglist_info`, `curseg_info[6]`, `flush_cmd_control`, and `discard_cmd_control`.

Established sizes/relationships:

```text
sizeof(f2fs_sm_info)        = 0xA8
sizeof(sit_info)            = 0xA8
sizeof(free_segmap_info)    = 0x20
sizeof(dirty_seglist_info)  = 0x90
sizeof(curseg_info)         = 0x70
curseg_info count           = 6
curseg array size           = 0x2A0
sm_info + 0x98              = fcc_info
sm_info + 0xA0              = dcc_info
sizeof(discard_cmd_control) = 0x20B0
```

The dirty counters at `dirty_info + 0x68..0x7c` are the first six entries of `nr_dirty[8]`, not vendor counters. Earlier hypotheses about `sm_info + 0x40..0x47` and `+0x90..0x97` being vendor insertions were withdrawn during consumer proof.

## Transsion GC framework

Deep reconstruction covered `tran_gc_thread_func`, `tran_urgent_gc_read`, `tran_urgent_gc_write`, `tran_do_f2fs_gc`, `tran_gc_init`, and `tran_gc_stop`.

Conclusion: Transsion did not replace the core F2FS collector. It adds a policy/control layer around stock `f2fs_gc()` with urgent-GC state, GC type/mode control, thresholds, fragmentation/SSR decisions, timing, counters, and telemetry. `tran_do_f2fs_gc()` is a wrapper that selects a GC mode, invokes normal `f2fs_gc()`, and restores the previous mode.

`CONFIG_F2FS_TRAN_GC=y` is enabled in the X683 configuration.

## Next phase

Target the actual algorithmic Transsion delta inside F2FS GC:

1. victim selection
2. victim scoring/cost calculation
3. SSR selection
4. age/mtime use
5. victim filtering
6. migration policy
7. determine whether Transsion changes victim selection itself or only controls when/mode the stock collector runs

Use the stock X683 binary as authority. Use public 4.14 F2FS source only as comparison/naming evidence. Separate binary proof, source correlation, and inference. Do not fill unknowns from newer F2FS structures.

## Repository cleanup status

Many exploratory branches exist. Several GC branches are byte-for-byte identical at their tips; for example `gc-deep-pass`, `gc-deep-pass-final`, `gc-deep-pass-final2`, and `gc-pass-final` all resolve to commit `ac5db07b8b673d5b80aba800efb5a909179d6f32`.

The canonical continuation branch is now:

`kernel-reconstruction-current`

It points at the completed deep-GC state plus this snapshot.

An archival branch preserves the pre-GC reconstruction state:

`archive/reconstruction-f2fs-balance-delta-2026-08-19`

Older exploratory branches should be treated as historical unless a future comparison demonstrates unique evidence in them.
