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

The dirty counters at `dirty_info + 0x68..0x7c` are the first six entries of `nr_dirty[8]`. Earlier hypotheses that these were vendor counters were withdrawn during consumer proof.

## Stock X683 GC reconstruction already completed

The stock X683 collector uses the four-argument ABI:

```c
f2fs_gc(sbi,
        (sbi->mount_opt.opt >> 14) & 1,
        true,
        NULL_SEGNO);
```

The completed victim-selection/migration phase established that X683 preserves the stock 4.14-era algorithmic core:

- `get_victim_by_default()` remains stock-like;
- victim filtering remains stock-like;
- SSR/greedy/cost-benefit scoring remains stock-like;
- age/mtime cost-benefit remains stock-like;
- `f2fs_need_SSR()` remains stock-like, including `gc_mode == 3` returning true;
- node/data migration remains stock-like;
- no vendor replacement migration engine was found.

See:

`docs/reverse-engineering/x683-f2fs-victim-selection-migration-delta-2026-08-19.md`

## Transsion GC policy/state-machine phase — completed 2026-08-19

The remaining directly visible Transsion policy layer is now reconstructed to the level supported by the X683 binary.

Detailed phase document:

`docs/reverse-engineering/x683-transsion-gc-policy-state-machine-2026-08-19.md`

### Core architectural result

The vendor delta is a controller around the stock collector:

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

### `tran_do_f2fs_gc()` — exact policy

Address:

`0xffffff92d0dfada8`

Only direct caller:

`tran_gc_thread_func + 0x544` at `0xffffff92d0df7414`.

Persistent vendor configuration:

```text
global + 0x998 = gc_type
```

`gc_type_write()` accepts only values `0..2`.

Exact mapping:

```text
gc_type 0 -> call f2fs_gc() with existing sbi->gc_mode

gc_type 1 -> save sbi->gc_mode; set sbi->gc_mode = 2;
              call f2fs_gc(); restore old mode

gc_type 2 -> save sbi->gc_mode; set sbi->gc_mode = 3;
              call f2fs_gc(); restore old mode
```

The stock call remains four-argument and uses `background=true`, `segno=NULL_SEGNO`, and mount-option bit 14 as `sync`.

### Transsion worker

`tran_gc_thread_func()` at `0xffffff92d0df6ed0` is a genuine vendor control worker. Directly proven behavior includes:

- `set_freezable()` and kthread-stop/freezer checks;
- waitqueue use through `global + 0x978`;
- a 250 ms polling timeout in the normal wait loop;
- urgent-GC state at `global + 0x9d0`;
- charger-type gating on the urgent/special path;
- the vendor free-segment predicate;
- dirty/segment-pressure threshold tests;
- wakelock admission checks;
- GC-mutex `mutex_trylock(sbi + 0x508)` before calling `tran_do_f2fs_gc()`;
- `f2fs_balance_fs_bg()` after the vendor wrapper returns;
- worker lifecycle counters and phase/status telemetry.

### `tran_has_enough_free_segment()`

Address:

`0xffffff92d0dfb5d4`

This is a genuine vendor free-space predicate, not the stock `has_not_enough_free_secs()` check.

Directly proven elements:

- `U = user_block_count >> log_blocks_per_seg`;
- unresolved SIT field at `sit + 0x10` is converted to segment units;
- `F = free_info + 0x04`;
- `R = sm_info + 0x60` (`reserved_segments`);
- row selector is `max(global+0x890, global+0x894)`;
- first table:
  `[2048, 3072, 4096, 4096, 100, 100, 100, 80]`;
- second table:
  `[80, 80, 80, 70, 70, 70, 60, 60]`;
- first and second threshold gates both compare `F-R` against percentage-derived thresholds;
- divide-by-100 reciprocal arithmetic is visible in the binary.

Direct callers:

```text
tran_gc_thread_func + 0x234
has_enough_free_seg_read + 0x40
```

### `is_f2fs_fragmentation()` — corrected conclusion

Address:

`0xffffff92d0dfb580`

The binary computes a fragmentation percentage from free segments and the user segment population, logs it, and then returns `0`.

A full direct-BL scan found no direct caller. Therefore this function is currently proven as a diagnostic/proc-oriented fragmentation calculation, not as a direct boolean urgency gate.

No evidence currently shows fragmentation changing the stock victim score.

### Event/control inputs

USB/charger and display events are genuinely involved in worker policy:

- `usb_charge_event()` can create or stop the Transsion worker;
- `fb_event()` handles `FB_EVENT_BLANK` and wakes the worker on blank/unblank transitions;
- charger type can gate the urgent/special worker path;
- wakelock state can gate admission before GC.

These event callbacks do not directly replace victim selection and do not directly prove a permanent `gc_mode` write.

### Urgent GC controls

`tran_urgent_gc_read()` / `tran_urgent_gc_write()` expose and control `global + 0x9d0`.

A nonzero write enables the urgent state and creates the worker if inactive. A zero write clears urgent state and stops the worker if active.

`need_switch_ssr_read()` / `need_switch_ssr_write()` expose the inverse of `global + 0x9c0`; their binary behavior is documented in the phase document without assigning an unproven internal enum name.

## Vendor state context

The exact formal C type and total size of the vendor global state are not proven. The binary nevertheless establishes a live state context containing at least:

```text
+0x898  worker active
+0x8a0  f2fs_sb_info *
+0x8a8  task_struct *
+0x8b8  wakeup source
+0x970  charger-detection state
+0x974  framebuffer state
+0x978  waitqueue
+0x990  GC invocation counter
+0x998  gc_type (0..2)
+0x9a0  post-GC counter
+0x9c0  inverse need_switch_ssr flag
+0x9d0  urgent-GC flag
+0x9d4  worker phase
+0x9e0  thread-create count
+0x9e8  thread-destroy count
+0x9f0  free-segment metric
+0x9f4  startup segment/dirty metric
+0x9f8  vendor telemetry/type state
+0x9fc  vendor status state
+0xa00  capacity/fragmentation state
+0xa04  positive threshold-trigger byte
+0xa05  wakelock/detect gate
+0xa06  post-threshold continuation flag
+0xa08  remembered metric
+0xa0c  remembered metric
+0xa10  last segment metric
+0xa18  remembered GC/delta value
+0xa20  proc directory pointer
```

No guessed formal struct declaration should replace this offset table until more binary evidence appears.

## Policy model

The final reconstructed model is:

```text
external subsystem/event
        |
        v
Transsion worker state
        |
        +-- urgent flag
        +-- charger / USB state
        +-- framebuffer wakeups
        +-- free-segment predicate
        +-- dirty/segment threshold ladder
        +-- wakelock admission
        +-- retry/timing/delta state
        |
        v
GC admission / worker scheduling
        |
        v
persistent gc_type 0..2
        |
        v
tran_do_f2fs_gc()
        |
        +-- temporary gc_mode 2 or 3 when requested
        |
        v
stock f2fs_gc()
        |
        +-- stock victim filtering
        +-- stock SSR/greedy/CB scoring
        +-- stock age/mtime
        +-- stock migration
```

## Genuine vendor delta versus stock behavior

### Vendor-owned / directly proven

- worker lifetime and wake/sleep policy;
- urgent-GC control;
- charger/USB and framebuffer integration;
- wakelock admission;
- free-segment threshold policy;
- dirty/segment trigger thresholds;
- `gc_type` configuration;
- temporary `gc_mode` override;
- vendor state/telemetry/proc surface.

### Stock / directly retained

- victim filtering;
- victim scoring;
- SSR selector;
- age/mtime cost-benefit;
- node/data migration;
- stock retry/checkpoint architecture;
- four-argument `f2fs_gc()` ABI.

### Unresolved

- exact formal vendor state-structure declaration/size;
- exact source-level names for some internal telemetry fields;
- exact enum/string names for every proc-backed state field;
- exact source member name of the SIT field at `+0x10` used by the vendor free/fragmentation helpers;
- possible indirect callback usage of `is_f2fs_fragmentation()`.

## Repository / branch state

Canonical branch:

`kernel-reconstruction-current`

The active reconstruction is kept on this branch. No new exploratory branch was created for this phase.

Historical/archive branch retained by project policy:

`archive/reconstruction-f2fs-balance-delta-2026-08-19`

`main` remains the repository baseline.

The prior exploratory GC/reconstruction branches are not part of the active reconstruction path.

## Next phase

Do not reopen the completed victim-selection/migration phase unless new binary evidence contradicts it.

Proceed to the remaining X683 F2FS integration/layout reconstruction, using the now-completed Transsion policy/state-machine model as the boundary between vendor scheduling policy and stock collector internals.
