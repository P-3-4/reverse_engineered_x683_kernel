# X683 Kernel Reverse Engineering — Current State Snapshot

Date: 2026-08-19
Repository: `P-3-4/reverse_engineered_x683_kernel`
Canonical working branch: `kernel-reconstruction-current`

## Current phase status

The X683 F2FS segment-manager reconstruction is complete enough for integration, including:

- `f2fs_sm_info`
- `sit_info`
- `free_segmap_info`
- `dirty_seglist_info`
- `curseg_info[6]`
- `flush_cmd_control`
- `discard_cmd_control`

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

The dirty counters at `dirty_info + 0x68..0x7c` are the first six entries of `nr_dirty[8]`; earlier vendor-counter hypotheses were withdrawn during consumer proof.

## Stock GC reconstruction

The X683 collector uses the proven four-argument ABI:

```c
f2fs_gc(sbi,
        (sbi->mount_opt.opt >> 14) & 1,
        true,
        NULL_SEGNO);
```

`f2fs_gc` is at `0xffffff92d0dd03a8`.

The completed victim-selection/migration phase established that X683 preserves the stock 4.14-era algorithmic core:

- `get_victim_by_default()` remains stock-like at `0xffffff92d0dd2e74`;
- victim filtering remains stock-like;
- SSR/greedy/CB scoring remains stock-like;
- age/mtime cost-benefit remains stock-like;
- `f2fs_need_SSR()` remains stock-like, including `gc_mode == 3` returning true;
- node/data migration remains stock-like;
- no vendor replacement migration engine was found.

Detailed prior phase:

`docs/reverse-engineering/x683-f2fs-victim-selection-migration-delta-2026-08-19.md`

## Transsion GC policy/state-machine phase

The vendor subsystem is a controller around the stock collector:

```text
external event / explicit request
        -> vendor GC worker state
        -> free-space / threshold / wakelock admission
        -> persistent gc_type (0..2)
        -> temporary sbi->gc_mode override
        -> tran_do_f2fs_gc()
        -> stock f2fs_gc(sbi, sync, true, NULL_SEGNO)
        -> stock victim selection / scoring / migration
```

Detailed phase:

`docs/reverse-engineering/x683-transsion-gc-policy-state-machine-2026-08-19.md`

### `tran_do_f2fs_gc()`

Address: `0xffffff92d0dfada8`.

Only direct caller: `tran_gc_thread_func +0x544` at `0xffffff92d0df7414`.

```text
global +0x990 = GC invocation counter

global +0x998 = persistent gc_type

0 -> preserve sbi+0x534
1 -> temporary sbi+0x534 = 2
2 -> temporary sbi+0x534 = 3
```

For `gc_type != 0`, the old `gc_mode` is saved and restored after `f2fs_gc()` returns. The stock call uses `sync = bit14(sbi+0x4b8)`, `background = true`, `segno = -1`.

`gc_type_write()` accepts only values `0..2`.

### Worker

`tran_gc_thread_func()` is at `0xffffff92d0df6ed0`.

Direct vendor calls:

```text
+0x234 -> tran_has_enough_free_segment
+0x544 -> tran_do_f2fs_gc
```

The worker uses a waitqueue at `+0x978`, a 250 ms normal polling timeout, urgent state at `+0x9d0`, charger gating, free-segment policy, dirty/pressure thresholds, wakelock checks, `mutex_trylock(sbi+0x508)`, and post-GC `f2fs_balance_fs_bg()` paths.

`tran_has_enough_free_segment()` is at `0xffffff92d0dfb5d4` and uses:

```text
selector = max(global+0x890, global+0x894)
A = [2048, 3072, 4096, 4096, 100, 100, 100, 80]
B = [80, 80, 80, 70, 70, 70, 60, 60]
```

It uses user-segment arithmetic from `sbi+0x408` and `sbi+0x3d8`, free segments, reserved segments, and an unresolved SIT quantity at `sit_info+0x10`.

The worker's direct threshold ladder contains approximately 40%, 351, 25%, 13%, and 27% tests. Passing all writes `+0xa00=1` and `+0xa04=1`; failure writes `+0xa00=2`.

`is_f2fs_fragmentation()` at `0xffffff92d0dfb580` computes/logs fragmentation and returns zero. No direct BL caller was found; it is not proven to be an active urgency gate.

## Complete integration result — 2026-08-19 deep pass

The complete direct ARM64 `BL` scan establishes:

```text
f2fs_start_gc_thread +0xd8 -> tran_gc_init
f2fs_stop_gc_thread  +0x18 -> tran_gc_stop
tran_gc_thread_func  +0x234 -> tran_has_enough_free_segment
tran_gc_thread_func  +0x544 -> tran_do_f2fs_gc
has_enough_free_seg_read +0x40 -> tran_has_enough_free_segment
tran_do_f2fs_gc +0x58/+0x94/+0xc4 -> f2fs_gc
```

`gc_thread_create` creates `tran_gc_thread_func` through the kthread API; event/proc handlers such as `usb_charge_event`, `fb_event`, `tran_urgent_gc_write`, `gc_type_write`, and `need_switch_ssr_write` are callback/proc-op paths rather than ordinary direct BL callers.

### Hidden downstream vendor modification check

From `tran_do_f2fs_gc()` into `f2fs_gc()`, victim selection, node/data migration, segment freeing, retry and checkpoint cleanup, no downstream `tran_*` call target was found.

Therefore the previously stated vendor/stock boundary is strengthened:

```text
Transsion policy/state
        -> tran_do_f2fs_gc
        -> stock f2fs_gc
        -> stock victim selection
        -> stock migration/accounting
```

Logical helpers such as `__get_victim()`, `do_garbage_collect()`, `gc_data_segment()`, and `gc_node_segment()` are not preserved as standalone kallsyms functions in this image and must not be assigned fabricated addresses.

## `f2fs_sb_info` integration

High-confidence fields used by this phase:

```text
+0x80   sm_info *
+0x3d8  log_blocks_per_seg
+0x3dc  blocks_per_seg
+0x3e0  segs_per_sec
+0x408  user_block_count
+0x428  reserved_blocks
+0x430  current_reserved_blocks
+0x438  unusable_block_count
+0x440  nquota_files
+0x4b8  mount_opt.opt
+0x508  gc_mutex
+0x534  gc_mode
+0x538  next_victim_seg[0]
+0x53c  next_victim_seg[1]
+0x560  max_victim_search
+0x564  migration_granularity
+0x568  stat_info *
```

Runtime pointer graph:

```text
sbi + 0x80
   -> sm_info
      +0x00 -> sit_info
      +0x08 -> free_info
      +0x10 -> dirty_info
      +0x60 -> reserved_segments
      +0x98 -> flush_cmd_control
      +0xa0 -> discard_cmd_control
```

## Vendor global-state map

No formal C struct is asserted. Proven/offset-backed fields include:

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
+0x9d4 worker phase
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

## Genuine vendor delta

Directly proven vendor-owned behavior:

- worker lifetime and scheduling;
- urgent GC control;
- charger/USB and framebuffer integration;
- wakelock admission;
- free-segment threshold policy;
- dirty/segment pressure ladder;
- persistent `gc_type` configuration;
- temporary `gc_mode` override;
- vendor proc/state telemetry.

Directly retained stock behavior:

- victim filtering;
- SSR/greedy/cost-benefit scoring;
- age/mtime scoring;
- node/data migration;
- stock retry/checkpoint architecture;
- four-argument `f2fs_gc()` ABI.

Disproven/not found:

- separate vendor victim scorer;
- separate vendor migration engine;
- direct fragmentation urgency gate;
- permanent vendor `gc_mode` mutation;
- downstream vendor GC helper between wrapper and stock migration.

## Source baseline

The supplied Image identifies:

```text
Linux version 4.14.141+
Android clang 9.0.3
Fri Nov 5 15:56:25 CST 2021
CONFIG_MTK_PLATFORM="mt6768"
CONFIG_F2FS_TRAN_GC=y
```

The exact Transsion source git revision is not proven by the supplied binary. Public Android/Linux 4.14 F2FS sources are comparison/naming baselines only; binary evidence wins where prototypes or structure details differ.

## Remaining genuinely unresolved questions

1. Formal vendor global-state C declaration and total size.
2. Exact source member names for several vendor offsets.
3. Exact historical C member at `sit_info +0x10` used by the vendor free-space helper.
4. Exact indirect proc-op/event callback container layouts.
5. Exact vendor source git revision behind `4.14.141+`.
6. Exact standalone addresses for logical helpers that are inlined.

These unresolved items do not currently weaken the proven Transsion-to-stock GC boundary.

## Next phase

Proceed to the broader remaining X683 F2FS integration/layout reconstruction: adjacent `f2fs_sb_info` fields, checkpointing, segment allocation, discard/flush paths, and remaining pointer/lock relationships. Do not reopen the completed GC victim-selection/migration or Transsion policy phases unless new binary evidence contradicts them.
