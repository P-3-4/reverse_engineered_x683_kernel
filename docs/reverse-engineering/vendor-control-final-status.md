# X683/H694 vendor GC controls — final binding status

Binary authority: stock X683/H694 Image. This is reconstructed/inferred analysis, not recovered proprietary source.

## `need_switch_ssr`

Direct registration:

```text
registration: 0x37af88
string:      Image + 0x10a6359
descriptor:  Image + 0x173b9d0
```

The recovered vendor-state map independently identifies:

```text
Image + 0x1a139c0 = need_switch_ssr (u8)
```

The control reaches the common registry/attribute implementation through `0x274ea0 -> 0x274dac`; no unique standalone callback named `need_switch_ssr` is proven.

Status: **backing state bound; callback semantics unresolved**.

## `tran_urgent_gc`

Direct registration:

```text
registration: 0x37b068
string:      Image + 0x10a63ac
descriptor:  Image + 0x173bbb0
```

Independent vendor-state mapping gives:

```text
Image + 0x1a139d0 = tran_urgent_gc (u32)
```

Again, the named control is a generic registry endpoint, not a proven standalone function.

Status: **backing state bound; callback semantics unresolved**.

## `detect_charger_type`

Direct registration:

```text
registration: 0x37b184
string:      Image + 0x10a6414
descriptor:  Image + 0x173bf70
```

A `detect_charger_type_write` handler is directly present in kallsyms, but the backing storage/consumer is not yet independently bound to a specific recovered global or SBI field.

Status: **registration + write handler proven; runtime storage/consumer unresolved**.

## `tran_gc_usb_wakelock`

String:

```text
Image + 0x10a5ee7
```

This is outside the contiguous three-control registration sequence and is associated with the separate wakeup-source path.

Status: **separate path; implementation chain unresolved**.

## Important controller-label correction

The stock `tran_f2fs_gc()` wrapper at `0x37ada8..0x37ae94` proves:

```text
controller 0 -> no temporary gc_mode change
controller 1 -> sbi->gc_mode = 2
controller 2 -> sbi->gc_mode = 3
```

and the vendor `gc_mode` values are:

```text
2 = URGENT
3 = GREEDY
```

Therefore the detector's direct Stop-4/Stop-5 writes of:

```text
+0x998 = 2
```

must be recorded as **controller-state value 2**, not automatically called "urgent". The resulting wrapper behavior is a temporary `gc_mode = 3` selection.

Older project notes that called this store an "urgent" transition are superseded by the binary-proven controller mapping.

## Architecture

The recovered model is:

```text
/proc/tran_gc_debug control
        -> generic registry node
        -> common operation/context
        -> per-control descriptor/private data
        -> vendor state/side effect
```

It is **not**:

```text
control name -> unique standalone GC function
```

No reconstructed GC path should invent direct calls to these control names without a separate binary call/read/write proof.
