# X683/H694 vendor GC control registration binding

This is reconstructed/inferred from the verified stock X683/H694 boot image. It is not proprietary Transsion source.

## Binary authority

Boot image SHA-256:

`a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`

Decompressed Image SHA-256:

`96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

All addresses below are decompressed Image offsets.

## Registration routine

The vendor GC debug/control registration sequence begins around `0x37b2f0` and repeatedly calls a common helper at `0x274ec8`.

The setup loads the second argument from:

```text
ADRP x20, 0x1a13000
LDR  x1, [x20, #0xa20]
```

so the pointer source is **Image + 0x1a13a20**, not `0x1698a20`.

The value stored at that BSS/global location is runtime state; the on-image bytes are initially zero. The same loaded pointer is passed as `x1` for the whole contiguous registration sequence.

Therefore this code is a **generic attribute/control registration path**, not a collection of direct calls to `need_switch_ssr()`, `tran_urgent_gc()`, or `detect_charger_type()`.

## Exact named-control registration call sites

| Control | Registration call | Name string offset |
|---|---:|---:|
| `need_switch_ssr` | `0x37b3e4` | `0x10a6359` |
| `tran_urgent_gc` | `0x37b484` | `0x10a63ac` |
| `detect_charger_type` | `0x37b504` | `0x10a6414` |

The same sequence also registers:

```text
life_time
written_data
bad_block
lvdf
is_fragmentation
detect_wakelock
emmc_gc_time
need_switch_ssr
gc_time
data_movement
wake_up_detect_time
ssr_gc_times
percent_of_free_segment
total_segment
has_enough_free_seg
gc_type
free_segment
tran_urgent_gc
invalid_segment
f2fs_status
static_pass_times
gc_skip_times
gc_times
thread_destroy_times
thread_create_times
detect_charger_type
gc_to_static_detect_times
last_phase
gc_segment_info
inc_gc_seg_threshold
dec_gc_seg_threshold
```

The repeated `x1` load from `Image + 0x1a13a20` and the direct name-string sequence are binary-confirmed.

## Common helper `0x274ec8`

Direct disassembly shows `0x274ec8` is a generic registry/attribute lookup-and-link helper.

Its relevant flow is:

```text
x0 = supplied control name
x1 = common runtime object
        |
        +-> internal lookup/allocation helper
        +-> list traversal through [x1 + 0x38]
        +-> name/hash comparison
        +-> link/registration
        +-> error path if the target is absent
```

It is **not** the implementation of any one named GC control.

The helper does not receive a distinct callback pointer at these registration call sites.

## What is actually bound

### `need_switch_ssr`

Proven registration binding:

```text
string 0x10a6359
    -> call 0x37b3e4
    -> common registration helper 0x274ec8
    -> runtime object loaded from Image + 0x1a13a20
```

The actual read/store callback or decision function is still indirect through the runtime object's registered attribute structures.

### `tran_urgent_gc`

```text
string 0x10a63ac
    -> call 0x37b484
    -> common registration helper 0x274ec8
    -> runtime object loaded from Image + 0x1a13a20
```

Again, this is a control/attribute registration binding, not an implementation address.

The independent GC evidence proves that controller state `2` causes the Transsion wrapper to force `gc_mode = 3` (URGENT) for one `f2fs_gc()` invocation, but that does **not** prove `tran_urgent_gc` is the function implementing that transition.

### `detect_charger_type`

```text
string 0x10a6414
    -> call 0x37b504
    -> common registration helper 0x274ec8
    -> runtime object loaded from Image + 0x1a13a20
```

Its runtime producer/consumer remains indirect.

### `tran_gc_usb_wakelock`

`tran_gc_usb_wakelock` exists at string offset `0x10a5ee7`, but it does **not** occur in this contiguous registration sequence. Its separate registration/use path is still unresolved.

## Important correction to earlier reconstruction

A previous pass associated the ordered 24-byte records around `Image + 0x1698a20` with the three named controls. That association is **withdrawn**.

The actual registration routine loads its common runtime object from `Image + 0x1a13a20`. The bytes at `0x1698a20` are not sufficient evidence for a direct name-to-callback mapping and must not be used as such.

This correction prevents the project from turning a coincidental data pattern into a false callback binding.

## Current binding confidence

| Item | Status |
|---|---|
| registration routine location | High |
| common helper = `0x274ec8` | High |
| common runtime-object source = `Image + 0x1a13a20` | High |
| `need_switch_ssr` registration call = `0x37b3e4` | High |
| `tran_urgent_gc` registration call = `0x37b484` | High |
| `detect_charger_type` registration call = `0x37b504` | High |
| these names are registered controls/attributes | High |
| implementation callback addresses | Unresolved |
| `tran_gc_usb_wakelock` registration path | Unresolved |

## Next exact target

The remaining binding problem is now narrowly defined:

```text
runtime object @ Image + 0x1a13a20
        ↓
object/list initialized before or during 0x37b2f0
        ↓
attribute nodes reached through [object + 0x38]
        ↓
show/store/value callback fields
        ↓
need_switch_ssr / tran_urgent_gc / detect_charger_type implementation
```

The next useful reverse-engineering pass should therefore trace the initialization of `Image + 0x1a13a20` and the fields of the objects reachable through its `+0x38` list. Only that path can produce an evidence-backed implementation address.
