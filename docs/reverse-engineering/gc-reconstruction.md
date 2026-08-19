# X683/H694 F2FS GC reconstruction

## ABI match

Direct X683 binary evidence proves that the stock GC call uses the four-argument form:

```c
f2fs_gc(
    sbi,
    (sbi->mount_opt.opt >> 14) & 1,
    true,
    NULL_SEGNO);
```

The wrapper at `tran_do_f2fs_gc()` supplies the fourth argument as `NULL_SEGNO` (`-1`). The obsolete three-argument description is superseded.

## Stock state machine

The X683 `f2fs_gc()` preserves the historical 4.14 F2FS control structure:

1. validate filesystem/checkpoint state;
2. determine foreground/background mode;
3. handle free/prefree section pressure;
4. promote background GC where required;
5. select a victim through the stock `__get_victim()` / `get_victim_by_default()` path;
6. migrate the selected victim with the stock section/node/data engine;
7. account freed segments/sections;
8. perform retry/checkpoint/cleanup logic;
9. reset victim bookkeeping and release the GC inode list.

The logical `do_garbage_collect()` and `gc_data_segment()` boundaries are heavily inlined in the X683 `f2fs_gc()` body, but the recovered structure matches the historical 4.14 implementation.

## Victim-selection result

The stock X683 victim selector itself is not replaced by a Transsion algorithm.

`get_victim_by_default()` remains standalone at `0xffffff92d0dd2e74` and retains the historical victim-selection structure:

- dirty-list locking and policy setup;
- SSR/LFS distinction;
- `max_victim_search` limiting;
- current-victim and next-victim handling;
- SIT validity/checkpoint filtering;
- section-level candidate filtering;
- minimum-cost selection.

`gc_mode` at `sbi + 0x534` has the stock urgent-mode consequences already present in the selector.

## Victim scoring result

The X683 scoring path preserves the historical cases:

```text
SSR      -> ckpt_valid_blocks
GREEDY   -> valid-block cost / segment-type weighting
CB       -> age/mtime cost-benefit
```

The CB path directly uses:

```text
SIT entry +0x20 = mtime
sit_info +0x88  = min_mtime
sit_info +0x90  = max_mtime
sbi +0x3d8      = log_blocks_per_seg
```

and computes the same utilization, age, and final cost formula as the closest public 4.14 implementation.

## SSR result

`f2fs_need_SSR()` at `0xffffff92d0de58f8` retains the historical 4.14 threshold structure. It directly returns true for:

```text
sbi + 0x534 == 3
```

and otherwise performs the normal stock free-section threshold logic.

## Transsion policy/state-machine boundary

The remaining vendor-specific GC layer has now been reconstructed in detail in:

`docs/reverse-engineering/x683-transsion-gc-policy-state-machine-2026-08-19.md`

### `tran_do_f2fs_gc()`

Address:

`0xffffff92d0dfada8`

Exact persistent vendor configuration:

```text
global + 0x998 = gc_type
```

Exact mapping:

```text
gc_type 0 -> do not change sbi->gc_mode

gc_type 1 -> temporarily set sbi->gc_mode = 2

gc_type 2 -> temporarily set sbi->gc_mode = 3
```

The old `gc_mode` is saved before the override and restored after `f2fs_gc()` returns.

This proves that Transsion changes **when and under which existing stock mode the collector runs**, rather than replacing victim scoring or migration.

### `tran_gc_thread_func()`

Address:

`0xffffff92d0df6ed0`

Direct vendor calls proven in the worker:

```text
tran_gc_thread_func + 0x234 -> tran_has_enough_free_segment
tran_gc_thread_func + 0x544 -> tran_do_f2fs_gc
```

The worker uses:

- a waitqueue at global `+0x978`;
- a 250 ms scheduling timeout in the main wait loop;
- urgent state at `+0x9d0`;
- charger-type gating;
- free-segment policy;
- dirty/segment threshold tests;
- wakelock admission checks;
- `mutex_trylock(sbi + 0x508)` before entering vendor GC;
- post-call `f2fs_balance_fs_bg()` and superblock write-section cleanup;
- phase/counter/telemetry state spanning `+0x990..+0xa20`.

### `tran_has_enough_free_segment()`

Address:

`0xffffff92d0dfb5d4`

The vendor helper is a two-stage threshold predicate based on:

```text
user_block_count >> log_blocks_per_seg
free_segments - reserved_segments
unresolved sit_info + 0x10 field
threshold selector max(global+0x890, global+0x894)
```

with the exact static tables:

```text
[2048, 3072, 4096, 4096, 100, 100, 100, 80]
[80, 80, 80, 70, 70, 70, 60, 60]
```

This is not a replacement for the stock free-section test.

### `is_f2fs_fragmentation()`

Address:

`0xffffff92d0dfb580`

The function computes/logs fragmentation but returns zero. A full direct-BL scan found no direct caller. It is therefore not currently proven to be an active boolean GC urgency gate.

### Urgent GC and external events

`tran_urgent_gc_read/write()` expose/control global `+0x9d0`. A nonzero write enables urgent state and creates the worker if it is inactive; zero stops the worker.

`usb_charge_event()` can create/stop the worker based on charger events and charger type.

`fb_event()` handles framebuffer blank/unblank and wakes the worker through `+0x978`.

Wakelock status is checked by the worker before admission to one of the GC paths.

## Caller/callee graph

```text
f2fs_start_gc_thread
    -> tran_gc_init

f2fs_stop_gc_thread
    -> tran_gc_stop

tran_gc_thread_func
    -> tran_has_enough_free_segment
    -> tran_do_f2fs_gc

has_enough_free_seg_read
    -> tran_has_enough_free_segment

gc_thread_create / tran_urgent_gc_write / usb_charge_event
    -> tran_gc_thread_func through kthread creation

fb_event
    -> wake vendor waitqueue

stock gc_thread_func
    -> should_do_origin_gc
```

No direct vendor migration helper appears between `tran_do_f2fs_gc()` and the stock `f2fs_gc()` call.

## Vendor state context

The binary proves a global vendor state context containing at least:

```text
+0x898  worker active flag
+0x8a0  sbi pointer
+0x8a8  task pointer
+0x8b8  wakeup source
+0x970  charger state/control
+0x974  framebuffer state
+0x978  waitqueue
+0x990  GC call counter
+0x998  gc_type 0..2
+0x9a0  post-GC counter
+0x9c0  inverse SSR flag
+0x9d0  urgent-GC flag
+0x9d4  worker phase
+0x9e0  thread-create count
+0x9e8  thread-destroy count
+0x9f0  free-segment metric
+0x9f4  startup metric
+0x9f8  telemetry/type state
+0x9fc  status state
+0xa00  capacity/fragmentation state
+0xa04  positive threshold-trigger byte
+0xa05  wakelock/detect gate
+0xa06  continuation flag
+0xa08/+0xa0c remembered metrics
+0xa10  last metric
+0xa18  remembered GC/delta value
+0xa20  proc directory pointer
```

The formal C struct name/size remains unresolved and should not be guessed.

## Migration result

The binary retains the historical F2FS migration sequence:

```text
victim
  -> summary/SSA preparation
  -> segment iteration
  -> node/data GC
  -> merged-write submission
  -> accounting
  -> retry/checkpoint/cleanup
```

No Transsion `tran_*` migration helper was found in the core migration loop by direct branch-target inspection.

## Final conclusion

The complete X683 GC architecture is:

```text
Transsion events / state
        -> vendor worker admission
        -> free-space / threshold / wakelock policy
        -> persistent gc_type
        -> temporary sbi->gc_mode override
        -> tran_do_f2fs_gc()
        -> stock f2fs_gc()
             -> stock victim filtering
             -> stock SSR/greedy/CB scoring
             -> stock age/mtime scoring
             -> stock node/data migration
```

The remaining uncertainty is concentrated in exact vendor field names/struct layout and a small number of indirect callback relationships, not in the existence of a hidden vendor replacement collector.
