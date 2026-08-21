# X683 F2FS Deep Integration — 2026-08-19

## Checkpoint

`f2fs_write_checkpoint()` = `0xffffff92d0dce5d0`, size `0x1400`. Direct callers include `f2fs_put_super`, `f2fs_gc`, `f2fs_sync_fs`, `kill_f2fs_super`, `f2fs_disable_checkpoint`, `f2fs_trim_fs`, and `f2fs_recover_fsync_data`.

Direct call ordering establishes:

```text
f2fs_write_checkpoint
  -> f2fs_flush_nat_entries
  -> f2fs_flush_sit_entries
  -> discard/prefree handling
  -> unblock_operations
  -> checkpoint statistics
```

`f2fs_flush_nat_entries()` = `0xffffff92d0de3f2c`, size `0x85c`.
`f2fs_flush_sit_entries()` = `0xffffff92d0dec660`, size `0xad8`.

## Segment manager

`f2fs_build_segment_manager()` = `0xffffff92d0ded138`, size `0x1cd4`, called by `f2fs_fill_super()`.

`f2fs_destroy_segment_manager()` = `0xffffff92d0deee0c`, size `0x354`, reached from mount cleanup and unmount.

The proven manager graph remains:

```text
sbi +0x80 -> sm_info
  +0x00 sit_info
  +0x08 free_segmap_info
  +0x10 dirty_seglist_info
  +0x18 curseg_info[6]
  +0x60 reserved_segments
  +0x98 flush_cmd_control
  +0xa0 discard_cmd_control
```

## Allocation

| function | address | size |
|---|---|---:|
| `f2fs_allocate_data_block` | `0xffffff92d0dea3c8` | `0x734` |
| `f2fs_allocate_new_segments` | `0xffffff92d0de94fc` | `0xd8` |
| `allocate_segment_by_default` | `0xffffff92d0df0454` | `0x394` |
| `new_curseg` | `0xffffff92d0df07e8` | `0x4e4` |
| `f2fs_submit_page_bio` | `0xffffff92d0dd3b48` | `0x2a8` |
| `f2fs_submit_page_write` | `0xffffff92d0dd443c` | `0x72c` |

Direct disassembly of `allocate_segment_by_default()` proves use of `sbi +0x3d8`, `0x3dc`, `0x3e0`, `0x428`, `0x434`, `0x440`, `gc_mode +0x534`, the segment-allocation operation table, `change_curseg()` and `new_curseg()`.

`new_curseg()` directly mutates free/segment bitmap state through `find_next_zero_bit`, `set_bit` and `test_and_set_bit` and participates in metadata updates.

## Node/NAT

Key X683 functions:

```text
f2fs_build_node_manager  0xffffff92d0de49a8
f2fs_destroy_node_manager 0xffffff92d0de5038
f2fs_get_node_info       0xffffff92d0ddd31c
f2fs_get_dnode_of_data   0xffffff92d0ddd7fc
f2fs_get_node_page       0xffffff92d0dddfb8
f2fs_new_node_page       0xffffff92d0dde1c8
f2fs_fsync_node_pages    0xffffff92d0de19ac
f2fs_sync_node_pages     0xffffff92d0de20dc
f2fs_flush_nat_entries   0xffffff92d0de3f2c
```

Node manager construction is directly called from `f2fs_fill_super()`, while NAT flush is directly coupled to checkpointing.

## Recovery

`f2fs_recover_fsync_data()` = `0xffffff92d0df0d08`, size `0x1bdc`, directly reached from `f2fs_fill_super()`.

The function reaches inode retry, dnode/data truncation, inode accounting, node-page operations and dirty-state handling.

## Discard/flush

`issue_discard_thread()` = `0xffffff92d0df0120`, size `0x334`.

Direct call targets prove a freezable kthread with `kthread_should_stop()`, freezer handling, waitqueue/timed waits, discard-range waits, submission and teardown. The previously established `discard_cmd_control` size remains `0x20B0`.

## Shrinker/sysfs

X683 contains `f2fs_shrink_count`, `f2fs_shrink_scan`, `f2fs_join_shrinker`, `f2fs_leave_shrinker`, plus segment/victim debug views and F2FS sysfs registration.

`f2fs_shrink_scan()` reaches extent-tree, NAT and NID freeing, proving a real F2FS-to-MM pressure path.

## Source added

- `reconstructed/fs/f2fs/x683_segment_reconstructed.c`
- `reconstructed/fs/f2fs/x683_checkpoint_reconstructed.c`
- `reconstructed/fs/f2fs/x683_node_reconstructed.c`
- `reconstructed/fs/f2fs/x683_data_reconstructed.c`
- `reconstructed/fs/f2fs/x683_recovery_reconstructed.c`

These are binary-backed semantic models, not claims of original byte-identical source.
