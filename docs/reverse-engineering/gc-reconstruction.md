# X683/H694 F2FS GC reconstruction

## ABI match

Direct X683 binary evidence proves that the stock GC call uses the four-argument form:

```c
f2fs_gc(sbi,
        (sbi->mount_opt.opt >> 14) & 1,
        true,
        NULL_SEGNO);
```

The wrapper at `tran_do_f2fs_gc()` supplies the fourth argument as `NULL_SEGNO` (`-1`). The obsolete three-argument description is superseded.

## Stock state machine

The stock X683 `f2fs_gc()` preserves the historical 4.14 F2FS control structure:

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

The remaining vendor-specific GC layer is reconstructed in:

`docs/reverse-engineering/x683-transsion-gc-policy-state-machine-2026-08-19.md`

The vendor controller performs scheduling/admission, free-space and threshold policy, wakelock/charger/framebuffer integration, and temporary `gc_mode` selection. It does not replace victim scoring or migration.

## Exact `tran_do_f2fs_gc()` boundary

Address: `0xffffff92d0dfada8`.

Direct caller: `tran_gc_thread_func +0x544` at `0xffffff92d0df7414`.

The wrapper performs:

```text
global +0x990++
read global +0x998 (gc_type)

0: f2fs_gc(sbi, bit14(sbi+0x4b8), true, -1)

1: save sbi+0x534; set 2; f2fs_gc(...); restore
2: save sbi+0x534; set 3; f2fs_gc(...); restore

global +0x9a0++
```

The `gc_type` write path accepts only `0..2`.

## Caller/callee graph

```text
f2fs_start_gc_thread +0xd8
    -> tran_gc_init

f2fs_stop_gc_thread +0x18
    -> tran_gc_stop

tran_gc_thread_func +0x234
    -> tran_has_enough_free_segment

tran_gc_thread_func +0x544
    -> tran_do_f2fs_gc

has_enough_free_seg_read +0x40
    -> tran_has_enough_free_segment

tran_do_f2fs_gc +0x58/+0x94/+0xc4
    -> f2fs_gc
```

Callback/event handlers such as `usb_charge_event`, `fb_event`, and `tran_urgent_gc_write` are reached through registration/event paths rather than ordinary direct `BL` calls.

## Hidden downstream modification result

A complete direct ARM64 `BL` target scan of the relevant X683 Image region found no `tran_*` target in the downstream path from `f2fs_gc()` to victim selection, node/data migration, freeing, retry, or checkpoint handling.

Therefore no downstream vendor replacement collector is proven.

The following are logical/source-correlated boundaries rather than standalone X683 symbols:

```text
__get_victim
do_garbage_collect
gc_data_segment
gc_node_segment
```

They are inlined or otherwise not preserved as independent kallsyms symbols in this image.

## Vendor state context

The binary proves a vendor state context containing at least:

```text
+0x890/+0x894 free-policy selectors
+0x898 worker active
+0x8a0 sbi pointer
+0x8a8 task pointer
+0x8b0/+0x8b8 wakeup-source context
+0x968 wakelock/detect control
+0x970 charger control
+0x974 framebuffer state
+0x978 waitqueue
+0x990 GC count
+0x998 gc_type
+0x9a0 post-GC count
+0x9b0/+0x9b8 retry/special-path counters
+0x9c0 inverse SSR flag
+0x9c8 remembered retry/state
+0x9d0 urgent GC
+0x9d4 phase
+0x9d8 remembered state
+0x9e0 create count
+0x9e8 destroy count
+0x9f0 free metric
+0x9f4 startup metric
+0x9f8 type state
+0x9fc status state
+0xa00 capacity/fragmentation decision state
+0xa04 threshold-hit byte
+0xa05 wakelock/detect gate
+0xa06 continuation
+0xa08/+0xa0c remembered metrics
+0xa10 last metric
+0xa18 remembered GC/delta
+0xa20 proc directory
```

The formal vendor C structure and total size remain unresolved.

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
             -> stock retry/checkpoint/cleanup
```

The remaining uncertainty is concentrated in exact vendor field names/struct layout, some indirect callback container layouts, and the exact vendor source git revision. The existence of a hidden vendor replacement collector is not supported by the X683 evidence.
