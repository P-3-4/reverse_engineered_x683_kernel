# X683 Persistent Call-Graph Model — 2026-08-19

The executable inventory was generated from X683 kallsyms and a direct ARM64 `BL` scan. 44,852 of 52,784 executable functions have at least one exact-symbol direct call target.

## Core runtime graph

```text
f2fs_fill_super
 -> f2fs_build_segment_manager
 -> f2fs_build_node_manager
 -> f2fs_recover_fsync_data

f2fs_gc
 -> f2fs_write_checkpoint
 -> f2fs_allocate_data_block
 -> f2fs_submit_page_bio
 -> f2fs_submit_page_write

f2fs_write_checkpoint
 -> f2fs_flush_nat_entries
 -> f2fs_flush_sit_entries
 -> f2fs_release_discard_addrs / f2fs_clear_prefree_segments

f2fs_build_segment_manager
 -> SIT/free/dirty/curseg initialization
 -> discard/flush workers

f2fs_allocate_data_block
 -> update_sit_entry
 -> locate_dirty_segment
 -> discard/write paths

f2fs_recover_fsync_data
 -> f2fs_allocate_new_segments
 -> inode/node/data recovery

tran_gc_thread_func
 -> tran_has_enough_free_segment
 -> tran_do_f2fs_gc

tran_do_f2fs_gc
 -> f2fs_gc

msdc_drv_probe
 -> mmc_alloc_host
 -> msdc_dt_init
 -> CMDQ initialization

msdc_ops_request
 -> msdc_do_request_prepare
 -> msdc_pre_crypto
 -> command/DMA path
 -> error tuning

msdc_irq
 -> controller completion/error handling

schedtune_enqueue_task
 <- enqueue_task_fair
 <- enqueue_task_rt

battery_update
 -> power_supply_changed

fb_register_client
 <- tran_gc_init
 <- ppm_lcmoff_policy_init
 <- ged_hal_init

fb_event
 -> Transsion GC wakeup

binder_ioctl_write_read
 -> binder_transaction
 -> binder allocator

ion_alloc
 <- display/ISP/vendor multimedia allocation
```

## Key function records

| function | address | size | major callers |
|---|---|---:|---|
| `f2fs_write_checkpoint` | `0xffffff92d0dce5d0` | `0x1400` | GC, sync, unmount, recovery |
| `f2fs_build_segment_manager` | `0xffffff92d0ded138` | `0x1cd4` | fill_super |
| `f2fs_allocate_data_block` | `0xffffff92d0dea3c8` | `0x734` | GC, allocation, write |
| `f2fs_build_node_manager` | `0xffffff92d0de49a8` | `0x690` | fill_super |
| `f2fs_recover_fsync_data` | `0xffffff92d0df0d08` | `0x1bdc` | fill_super |
| `issue_discard_thread` | `0xffffff92d0df0120` | `0x334` | worker lifecycle |
| `msdc_drv_probe` | `0xffffff92d1564ba0` | `0x808` | platform registration |
| `msdc_ops_request` | `0xffffff92d1566448` | `0x764` | MMC request path |
| `msdc_irq` | `0xffffff92d1565aa0` | `0x69c` | controller IRQ |
| `binder_transaction` | `0xffffff92d15e7d1c` | `0x3aa0` | binder ioctl |
| `ion_alloc` | `0xffffff92d15adb68` | `0x84c` | multimedia/display/ISP |
| `schedtune_enqueue_task` | `0xffffff92d0b26944` | `0x1c8` | CFS/RT enqueue |
| `tran_do_f2fs_gc` | `0xffffff92d0dfada8` | binary-mapped | Transsion GC worker |

Indirect function-pointer calls, notifier container layouts and inline helpers are kept explicitly separate from this direct-BL model.
