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

The call shape is:

```text
x0 = control-name string
w1 = 0xfff
x2 = [Image + 0x1a13a20]   // common runtime registry object
x3 = per-control descriptor/backing object
```

`0x274ea0` clears `x4` and calls `0x274dac`.

The `0x274dac` body proves the per-control pointer is retained in the newly created registration node:

```asm
0x274e28  adrp x8, 0xe4a000
0x274e2c  add  x8, x8, #0x2c0
0x274e30  stp  x8, x20, [x21, #0x20]
0x274e34  str  x19, [x21, #0x60]
```

where `x20 = x3`.

Thus, for these registration nodes:

```text
node + 0x20 = common implementation/operation context
node + 0x28 = per-control descriptor/backing object
node + 0x60 = wrapper cookie (zero for this registration path)
```

The helper at `0x274ec8` is generic registry lookup/linking; it is not a vendor-control implementation.

## 3. Exact named controls and descriptor addresses

### `need_switch_ssr`

```text
registration call: 0x37af88
string:            Image + 0x10a6359
x3 descriptor:     Image + 0x173b9d0
```

The static Image bytes at `0x173b9d0` are zeroed, so this is runtime/BSS-backed state, not an inline function pointer.

### `tran_urgent_gc`

```text
registration call: 0x37b068
string:            Image + 0x10a63ac
x3 descriptor:     Image + 0x173bbb0
```

The descriptor storage is zero-initialized in the static Image.

### `detect_charger_type`

```text
registration call: 0x37b184
string:            Image + 0x10a6414
x3 descriptor:     Image + 0x173bf70
```

Again, the descriptor is runtime/BSS-backed storage.

### `tran_gc_usb_wakelock`

String:

```text
Image + 0x10a5ee7
```

It is not part of this contiguous three-control registration sequence and remains separately unresolved.

## 4. What is actually proven

The three named controls are **registered controls/attributes with distinct per-control backing descriptors**.

They are **not proven to be standalone function symbols**.

The proven path is:

```text
control name
   -> common runtime object @ Image + 0x1a13a20
   -> generic registration wrapper 0x274ea0
   -> node construction 0x274dac
   -> per-control descriptor retained at node +0x28
   -> common registry/attribute machinery
```

The operation layer is shared. The per-control state/data differs.

This explains why a direct BL search around each name does not reveal a unique implementation function.

## 5. Generic operation path

The surrounding registry machinery uses an operation/context object plus per-node private data rather than a dedicated function pointer embedded beside each control name.

The helper family around `0x2743xx..0x2746xx` contains the actual operation dispatch. At `0x2744d0`, the generic machinery loads an operation function pointer from its operation object and invokes it with the node/context and attribute/value parameters.

Therefore the correct model is:

```text
common operation/callback
        +
per-control descriptor/private data
```

not:

```text
control name -> unique vendor function
```

## 6. Relation to the proven GC state machine

The direct stock Stop-4 path remains:

```text
Stop 4
  -> +0x998 = 2 (unless +0x9c0 blocks)
  -> +0x9f8 = 1
  -> tran_f2fs_gc
  -> temporary sbi+0x534 = 3
  -> f2fs_gc(sbi, sync, true, -1)
  -> restore previous gc_mode
```

Thus the existence of the `tran_urgent_gc` control does **not** prove that Stop 4 calls a function named `tran_urgent_gc`. The Stop-4 machine code itself performs the controller store directly.

Similarly, `need_switch_ssr` and `detect_charger_type` must not be inserted into `tran_gc_thread_func()` without a direct read/store/call chain to their backing descriptors.

## 7. Final binding table

| Name | Registration call | String | Descriptor | Direct standalone implementation | Status |
|---|---:|---:|---:|---|---|
| `need_switch_ssr` | `0x37af88` | `0x10a6359` | `0x173b9d0` | not proven | **bound** |
| `tran_urgent_gc` | `0x37b068` | `0x10a63ac` | `0x173bbb0` | not proven | **bound** |
| `detect_charger_type` | `0x37b184` | `0x10a6414` | `0x173bf70` | not proven | **bound** |
| `tran_gc_usb_wakelock` | separate path | `0x10a5ee7` | unresolved | unresolved | **unbound** |

## 8. Remaining useful work

The registration structure itself is no longer the blocker. The remaining high-value task is to trace **reads/writes of the three backing descriptors through the common operation layer** and correlate those values with:

```text
controller +0x998
controller +0x9c0
vendor +0x974
vendor +0xd84/+0xd94
vendor +0xa10
```

That will establish whether any of the three controls directly drive the GC controller or are only diagnostic/configuration endpoints.
