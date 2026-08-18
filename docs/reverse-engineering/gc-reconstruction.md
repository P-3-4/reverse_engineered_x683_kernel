# X683/H694 F2FS GC reconstruction

## ABI match

The recovered X683 stock call site is equivalent to:

```c
f2fs_gc(sbi, sync, true);
```

Historical F2FS confirms that this three-argument form is a real 4.14-era API. A 2016 upstream change explicitly changed `f2fs_gc(sbi, sync)` to `f2fs_gc(sbi, sync, true)` and added the `background` parameter. The later API added a victim-segment argument, so that later prototype is not used as the X683 target ABI.

## Reconstructed state machine

1. `gc_type = sync ? FG_GC : BG_GC`.
2. Validate `SB_ACTIVE` and checkpoint error state.
3. For BG_GC with insufficient free sections, checkpoint prefree segments first.
4. Promote BG_GC to FG_GC if free sections remain insufficient.
5. Reject BG_GC when `background == false`; this preserves the historical `f2fs_balance_fs()` critical-path behavior.
6. Select a victim through `__get_victim()` under the segment-manager victim-selection path.
7. Migrate the victim with `do_garbage_collect()`.
8. Count a freed section only when FG_GC frees all `segs_per_sec` segments.
9. Clear `cur_victim_sec` after foreground collection.
10. For asynchronous/background operation, repeat GC while free sections remain insufficient.
11. Run a checkpoint after foreground GC in the asynchronous path.
12. Reset `SIT_I(sbi)->last_victim[ALLOC_NEXT]` before returning.
13. Release the GC inode list.
14. For synchronous GC, return success only if at least one complete section was freed; otherwise return `-EAGAIN`.

## X683 layout correlation

The recovered layout already establishes:

- `sbi + 0x3d8`: `log_blocks_per_seg`
- `sbi + 0x3dc`: `blocks_per_seg`
- `sbi + 0x3e0`: `segs_per_sec`
- `sbi + 0x408`: `user_block_count`
- `sbi + 0x428`: `reserved_blocks`
- `sbi + 0x430`: `current_reserved_blocks`
- `sbi + 0x438`: `unusable_block_count`
- `sbi + 0x440`: `nquota_files`
- `sbi + 0x4b8`: `mount_opt.opt`

The unresolved GC field at `0x534` now matches the historical `gc_mode` member with high confidence. Historical `f2fs_sb_info` places `cur_victim_sec` immediately before `gc_mode`, and the X683 binary's GC-state accesses are consistent with this ordering.

## Segment-manager accesses

The recovered `sm_info` correlations remain:

- `0x00`: `sit_info`
- `0x08`: `free_info`
- `0x10`: `dirty_info`
- `0x60`: `reserved_segments`

This explains the stock free-segment path as `sbi -> sm_info -> free_info` and the dirty-victim path through `dirty_info`.

## Transsion-specific boundary

The stock kernel has additional Transsion GC triggering/coupling around charging, USB, framebuffer events, wakelock state, fragmentation and GC mode. Those triggers are intentionally kept outside the reconstructed core `f2fs_gc()` until each binary call site is independently matched. The current source therefore reconstructs the F2FS GC core without inventing vendor-specific predicates.

## Confidence

- Three-argument GC ABI: **high**.
- Core historical state machine: **high**.
- `0x534 == gc_mode`: **high**.
- Dirty/free segment manager relationships: **high**.
- Exact Transsion trigger predicates: **unresolved**.
- Exact X683-era helper implementation revisions: **still requires final source-tree matching**.
