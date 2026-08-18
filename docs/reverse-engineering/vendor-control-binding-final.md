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

That helper allocates/initializes the runtime control object and returns it to the caller. The returned pointer is stored in:

```text
Image + 0x1a13a20
```

Subsequent registration calls load that same runtime pointer from `+0xa20` and pass it as the common object argument.

This corrects the earlier mistaken association with `Image + 0x1698a20`.

## 2. Registration wrapper

Each control is registered through:

```text
0x274ea0 -> 0x274dac
```

The call shape at `0x37af..0x37b2..` is:

```text
x0 = control-name string
w1 = 0xfff
x2 = [Image + 0x1a13a20]   // common runtime registry object
x3 = per-control descriptor/backing object
```

`0x274ea0` clears `x4` and tail-calls `0x274dac`.

The `0x274dac` body proves the per-control pointer is retained in the newly created registration node:

```asm
0x274e28  adrp x8, 0xe4a000
0x274e2c  add  x8, x8, #0x2c0
0x274e30  stp  x8, x20, [x21, #0x20]
0x274e34  str  x19, [x21, #0x60]
```

where `x20 = x3`.

Therefore the node stores:

```text
node + 0x20 = common registration object/implementation context
node + 0x28 = per-control descriptor/backing object (x3)
node + 0x60 = x4/cookie (zero in this wrapper)
```

The common object at `0x274ec8` performs list lookup/linking by name and does not implement an individual control.

## 3. Exact named controls and backing objects

### `need_switch_ssr`

```text
registration call: 0x37af88
string:            Image + 0x10a6359
x3 descriptor:     Image + 0x173b9d0
```

The boot Image contains zeroed storage at `0x173b9d0`, consistent with BSS/runtime state rather than an inline function address.

### `tran_urgent_gc`

```text
registration call: 0x37b0a4
string:            Image + 0x10a65?? / "tran_urgent_gc"
x3 descriptor:     Image + 0x173be80
```

The descriptor storage is zero-initialized in the static Image.

### `detect_charger_type`

```text
registration call: 0x37b184
string:            Image + 0x10a6414
x3 descriptor:     Image + 0x173bf70
```

Again, this is zero-initialized runtime storage, not a code address.

The exact string address for `tran_urgent_gc` should be taken from the disassembly at the registration call; the descriptor binding itself is direct.

## 4. What this proves

The three names are **registered controls/attributes with per-control backing descriptors**.

They are **not proven to be direct function symbols**.

The implementation path is generic:

```text
control name
   -> common runtime object @ Image + 0x1a13a20
   -> generic registration wrapper 0x274ea0
   -> node construction 0x274dac
   -> node retains per-control descriptor at +0x28
   -> common registry/attribute machinery
```

This explains why searching for a unique branch-and-link immediately associated with each string does not produce a separate `need_switch_ssr`, `tran_urgent_gc`, or `detect_charger_type` function body.

## 5. Generic callback/operation path

The registry implementation uses a common operation structure. In the surrounding registry helpers, nodes expose an operation/context pointer and a private-data pointer rather than storing one vendor implementation function per control.

At the operation path around `0x2744d0` the common machinery loads an operation function pointer from its operation object and invokes it with the node/context and the current attribute/value parameters.

This is the decisive architectural distinction:

```text
per-control state/data = distinct
control operation       = common/generic
```

So the correct reverse-engineering target is the **generic show/store/read/write operation** and then the behavior of each per-control backing descriptor.

## 6. `tran_gc_usb_wakelock`

The string `tran_gc_usb_wakelock` exists at Image `0x10a5ee7` but is not part of the same contiguous registration sequence containing the three named controls above.

Its registration/use site must therefore be traced independently.

Do not assign it to the same descriptor table without direct evidence.

## 7. Relation to the proven GC controller

The vendor GC detector independently proves:

```text
Stop 4
  -> +0x998 = 2 (unless +0x9c0 blocks)
  -> +0x9f8 = 1
  -> tran_f2fs_gc
  -> temporary sbi+0x534 = 3
  -> f2fs_gc(sbi, sync, true, -1)
  -> restore previous gc_mode
```

That controller path is distinct from the debug/control registration mechanism.

The presence of the `tran_urgent_gc` control therefore does not by itself prove that Stop 4 calls a function named `tran_urgent_gc`. The stock Stop-4 basic block directly performs the controller store; the wrapper later consumes that state.

Likewise, `need_switch_ssr` and `detect_charger_type` should not be inserted into `tran_gc_thread_func()` unless a direct use of their backing descriptors is recovered.

## 8. Final binding table

| Name | Registration | Per-control descriptor | Direct implementation function | Status |
|---|---:|---:|---:|---|
| `need_switch_ssr` | `0x37af88` | `Image + 0x173b9d0` | not separate/proven | **bound to control descriptor** |
| `tran_urgent_gc` | `0x37b0a4` | `Image + 0x173be80` | not separate/proven | **bound to control descriptor** |
| `detect_charger_type` | `0x37b184` | `Image + 0x173bf70` | not separate/proven | **bound to control descriptor** |
| `tran_gc_usb_wakelock` | separate path | unresolved | unresolved | **unbound** |

## 9. Next binary target

The registration problem itself is no longer the blocker.

The remaining useful target is now:

```text
per-control descriptor storage
        ↓
generic attribute read/write operation
        ↓
actual values written into +0x173b9d0 / +0x173be80 / +0x173bf70
        ↓
callers/readers of those values
        ↓
final vendor semantic binding
```

In particular, direct static references to these BSS addresses are sparse because the generic registry carries the descriptor pointer. Therefore the next pass should reverse the common operation callback invoked from the registry path, rather than continue searching for three imaginary standalone functions.
