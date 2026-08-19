# X683 Vendor Delta Index — 2026-08-19

## Proven vendor-specific classes

- Transsion F2FS GC worker/policy state machine.
- Persistent `gc_type` and temporary `gc_mode` policy.
- Charger/USB and framebuffer GC admission/wakeup.
- Wakelock/pressure/free-segment admission.
- Transsion battery probe and DT-backed attributes.
- Transsion display/LCD shutdown hooks.
- MediaTek/Transsion scheduler/PPM power policy.
- Vendor MMC telemetry/proc surfaces.
- Vendor low-memory hinting.

## Important exact vendor symbols

```text
tran_gc_thread_func        0xffffff92d0df6ed0
tran_do_f2fs_gc            0xffffff92d0dfada8
tran_has_enough_free_segment 0xffffff92d0dfb5d4
is_f2fs_fragmentation      0xffffff92d0dfb580
tran_battery_probe         0xffffff92d150cb90
fb_event                   0xffffff92d0dfacf8
trigger_lowmem_hint        0xffffff92d138c9c8
```

## Vendor/stock GC boundary

```text
Transsion worker/admission
  -> tran_do_f2fs_gc
  -> stock f2fs_gc
  -> stock victim selection
  -> stock node/data migration
  -> stock freeing/checkpoint/retry
```

The whole downstream direct-call scan found no additional `tran_*` target inserted between the wrapper and migration/cleanup.

## Negative findings

- No separate vendor victim scorer proven.
- No separate vendor migration engine proven.
- No classic lowmemorykiller function family found.
- `is_f2fs_fragmentation()` has no proven direct caller and is not promoted to an active urgency gate.
- `gc_mode` is not permanently changed by the vendor wrapper; nonzero `gc_type` changes are restored after GC.
