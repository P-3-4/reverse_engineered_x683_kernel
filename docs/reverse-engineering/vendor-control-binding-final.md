# X683/H694 vendor GC control binding — final registration walk

This document is reconstructed/inferred from the verified stock X683/H694 kernel Image. It is not recovered proprietary Transsion source.

## Binary authority

Boot SHA-256:

`a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`

Decompressed Image SHA-256:

`96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

All offsets below are decompressed Image offsets.

## 1. Registration object creation

At approximately `0x37aeec`, the vendor code registers the top-level `tran_gc_debug` object through the common vendor registry helper at `0x274c48`.

The returned runtime control object is stored at:

```text
Image + 0x1a13a20
```

Subsequent registration calls load the same runtime pointer.

## 2. Registration wrapper

Each control is registered through:

```text
0x274ea0 -> 0x274dac
```

Call shape:

```text
x0 = control-name string
w1 = 0xfff
x2 = [Image + 0x1a13a20]
x3 = per-control descriptor/backing object
```

The registration node retains the per-control pointer through the generic registry machinery.

## 3. Exact named controls

### `need_switch_ssr`

```text
registration call: 0x37af88
string:            Image + 0x10a6359
descriptor:        Image + 0x173b9d0
backing state:     Image + 0x1a139c0 (u8)
```

### `tran_urgent_gc`

```text
registration call: 0x37b068
string:            Image + 0x10a63ac
descriptor:        Image + 0x173bbb0
backing state:     Image + 0x1a139d0 (u32)
```

### `detect_charger_type`

```text
registration call: 0x37b184
string:            Image + 0x10a6414
descriptor:        Image + 0x173bf70
write handler:     detect_charger_type_write
backing state:     unresolved
```

### `tran_gc_usb_wakelock`

```text
string:            Image + 0x10a5ee7
registration path: separate
```

It is associated with the separate wakeup-source path and is not part of the contiguous three-control registration sequence.

## 4. Generic operation model

The three named controls are **registered attributes/controls with per-control descriptor state**. They are not proven to be one standalone implementation function per name.

The correct architecture is:

```text
control name
   -> common runtime object @ Image + 0x1a13a20
   -> generic registration wrapper 0x274ea0
   -> registry node
   -> per-control descriptor/private state
   -> shared attribute-operation layer
```

The surrounding generic operation machinery is therefore a stronger source-level model than inventing direct function calls from the control names.

## 5. Proven relation to GC controller state

The detector Stop-4/5 paths directly write the raw controller state:

```text
controller object +0x998 = 2
```

The current `tran_f2fs_gc()` wrapper mapping, proven from the stock binary, is:

```text
controller 0 -> direct f2fs_gc(); gc_mode unchanged
controller 1 -> temporary sbi->gc_mode = 2
controller 2 -> temporary sbi->gc_mode = 3
```

Therefore the raw Stop-4/5 write of controller value `2` must be described as **controller state 2**. It must not be called "urgent" merely because older project notes used that label.

The resulting wrapper behavior is a temporary `gc_mode = 3`, which the vendor mode evidence identifies as **GREEDY**.

Consequently, the existence of the `tran_urgent_gc` control does not prove that Stop 4 invokes a function named `tran_urgent_gc`.

## 6. Status table

| Name | Registration | Descriptor | Backing state | Handler/consumer status |
|---|---:|---:|---:|---|
| `need_switch_ssr` | `0x37af88` | `0x173b9d0` | `+0x1a139c0` | state bound; generic callback semantics unresolved |
| `tran_urgent_gc` | `0x37b068` | `0x173bbb0` | `+0x1a139d0` | state bound; generic callback semantics unresolved |
| `detect_charger_type` | `0x37b184` | `0x173bf70` | unresolved | write handler proven; runtime consumer unresolved |
| `tran_gc_usb_wakelock` | separate | unresolved | wakeup-source path | unresolved |

## 7. Final rule for reconstruction

Do not insert these names directly into the reconstructed GC state machine unless an instruction-level read/store/call chain proves the relationship.

Use the directly proven raw states and controller transitions first, then bind source-level names only after producer/consumer correlation.
