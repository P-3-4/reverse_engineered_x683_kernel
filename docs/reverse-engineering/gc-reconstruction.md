# X683/H694 F2FS GC reconstruction

## ABI match

Direct X683 AArch64 disassembly supersedes the earlier historical three-argument reconstruction. The stock entry at `0x3503a8` uses:

```c
f2fs_gc(sbi, sync, background, segno);
```

The function saves/uses `w1`, `w2`, and `w3`, and stock call sites supply either a real segment number or `-1` (`NULL_SEGNO`). The Transsion wrapper calls the stock entry with `segno = -1`.

The previously documented three-argument form was based on historical F2FS API chronology and is retained only as historical context, not as the X683 ABI.

## Reconstructed state machine

1. `gc_type = sync ? FG_GC : BG_GC`.
2. Validate `SB_ACTIVE` and checkpoint error state.
3. For BG_GC with insufficient free sections, checkpoint prefree segments first.
4. Promote BG_GC to FG_GC if free sections remain insufficient.
5. Reject BG_GC when `background == false`; this preserves the historical `f2fs_balance_fs()` critical-path behavior.
6. Select a victim through the segment-manager victim-selection path when `segno == NULL_SEGNO`.
7. Migrate the victim with `do_garbage_collect()`.
8. Count a freed section only when FG_GC frees all `segs_per_sec` segments.
9. Clear `cur_victim_sec` after foreground collection.
10. For asynchronous/background operation, repeat GC while free sections remain insufficient.
11. Run a checkpoint after foreground GC in the asynchronous path.
12. Release the GC inode list.
13. For synchronous GC, return success only if at least one complete section was freed; otherwise return `-EAGAIN`.

The exact use of a non-`NULL_SEGNO` fourth argument still requires direct call-site correlation before being promoted beyond this interface-level reconstruction.

## X683 layout correlation

The recovered layout establishes:

- `sbi + 0x3d8`: `log_blocks_per_seg`
- `sbi + 0x3dc`: `blocks_per_seg`
- `sbi + 0x3e0`: `segs_per_sec`
- `sbi + 0x408`: `user_block_count`
- `sbi + 0x428`: `reserved_blocks`
- `sbi + 0x430`: `current_reserved_blocks`
- `sbi + 0x438`: `unusable_block_count`
- `sbi + 0x440`: `nquota_files`
- `sbi + 0x4b8`: `mount_opt.opt`
- `sbi + 0x534`: `gc_mode`

Historical `f2fs_sb_info` placement and direct X683 accesses establish `0x534` as the GC policy field with high confidence.

## Segment-manager accesses

The recovered `sm_info` correlations remain:

- `0x00`: `sit_info`
- `0x08`: `free_info`
- `0x10`: `dirty_info`
- `0x60`: `reserved_segments`

This explains the stock free-segment path as `sbi -> sm_info -> free_info` and the dirty-victim path through `dirty_info`.

## Transsion-specific boundary

The stock kernel has additional Transsion GC triggering/coupling around charging, USB, framebuffer events, wakelock state, fragmentation and GC mode. Those triggers are intentionally kept outside the reconstructed core until each binary call site is independently matched. No vendor-specific predicate is inferred from strings alone.

## Confidence

- Four-argument X683 GC ABI: **high / direct disassembly**.
- Core historical state machine: **high, pending exact vendor-revision matching**.
- `0x534 == gc_mode`: **high**.
- Dirty/free segment manager relationships: **high**.
- Exact Transsion trigger predicates: **unresolved**.
- Exact X683-era helper implementation revisions: **still requires final source-tree matching**.
