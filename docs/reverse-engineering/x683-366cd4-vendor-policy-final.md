# X683/H694 — `0x366cd4` vendor GC policy/orchestration final reconstruction

## Authority

Direct stock X683/H694 Image evidence is authoritative. This document is binary-derived and is not recovered proprietary Transsion source.

## 1. Function boundary

```text
0x366cd4 .. 0x366edc = vendor GC policy/orchestration
```

It is distinct from:

```text
0x37ada8 .. 0x37ae94 = tran_f2fs_gc() controller wrapper
0x3503a8             = actual four-argument f2fs_gc()
```

## 2. Exact first policy ladder

Entry:

```text
sbi + 0x48 bit 3 set
    -> return
```

Then the vendor selector helper at `0x35cc18` is used with exact immediates:

```text
policy(sbi, 4)
    false -> 0x373108(sbi, 0x80)

policy(sbi, 1)
    false -> 0x35d22c(sbi, 455)

policy(sbi, 0)
    true  -> 0x362c40(sbi, 0, 0)
    false -> 0x363288(sbi, 0xE38)
```

The selector meanings are still anonymous. The branch relationships are proven.

## 3. Seven-field SBI guard

For `gc_mode != 3`, the stock thread code evaluates the following exact sequence:

```text
sbi + 0x44c -> nonzero => common guarded branch
sbi + 0x450 -> nonzero => common guarded branch
sbi + 0x454 -> nonzero => common guarded branch
sbi + 0x448 -> nonzero => common guarded branch
sbi + 0x444 -> nonzero => common guarded branch
sbi + 0x45c -> nonzero => common guarded branch
sbi + 0x458 -> zero    => continue normal path
             nonzero => common guarded branch
```

Therefore the normal path requires:

```text
0x44c == 0
0x450 == 0
0x454 == 0
0x448 == 0
0x444 == 0
0x45c == 0
0x458 == 0
```

The first six are short-circuit tests. `+0x458` is reached only after they are all zero.

No symbolic source names are assigned to these seven fields.

## 4. `gc_mode == 3` branch

The mode read is:

```c
mode = *(u32 *)((char *)sbi + 0x534);
```

`mode == 3` selects a separate branch before the seven-field non-mode-3 ladder. Therefore those seven fields are not universal gates.

The latest controller mapping is:

```text
controller 1 -> gc_mode 2 (URGENT)
controller 2 -> gc_mode 3 (GREEDY)
```

So a controller-state write of `2` must not be described as an "urgent" action.

## 5. First fixed-point/capacity guard

After the normal seven-field gate, the function enters a percentage-like fixed-point check using an object reached from `sbi + 0x70`.

Directly recovered participating fields:

```text
object + 0x04
object + 0x18
object + 0x80
```

The arithmetic belongs to the same integer reciprocal family used elsewhere in the Transsion detector:

```text
0x51EB851F
right shift 37
```

The exact source-level member names and threshold owner remain unresolved.

## 6. Secondary policy ladder

The function then executes two more selector checks:

```text
policy(sbi, 1)
    false -> common early/skip path

policy(sbi, 3)
    false -> common early/skip path
```

When both pass, a dirty/reservation comparison is performed and another fixed-point guard is evaluated.

The main load chain is binary-derived:

```text
sbi + 0x80
    -> object + 0x10
    -> scalar + 0x64

sbi + 0x70
    -> object + 0x04
    -> object + 0x18
    -> object + 0x80
```

The exact symbolic metric names are intentionally unresolved.

## 7. Time/current-value guard

A later block loads and combines:

```text
sbi + 0x1c8
sbi + 0x198
vendor global around Image + 0x16c6000 + 0xc14
```

and performs a multiply/high-word fixed-point comparison.

This is preserved as an anonymous policy predicate; it is not promoted to a battery/time/charge source-level name without stronger evidence.

## 8. Terminal post-policy / balance path

The terminal path is gated by:

```text
(sbi + 0x4b9) bit 7 == 1
```

When set, the binary invokes exactly:

```text
0x3e1014(stack-object)
0x34e224(stack-object, 1)
0x3e1558(stack-object)
0x341250(sbi->sb, 1)
```

and then accesses:

```text
*(stat_info **)(sbi + 0x568)
```

followed by an increment at:

```text
stat_info + 0x16c
```

### What is proven

```text
bit7 gate -> four-call terminal chain -> stat +0x16c increment
```

### What is NOT proven

The exact source identities of:

```text
0x3e1014
0x34e224
0x3e1558
0x341250
stat +0x16c
```

`0x341250` is therefore recorded as the **terminal filesystem balance/write-protection helper** rather than being renamed `f2fs_balance_fs_bg()` without binary proof.

This distinction matters because the ordinary `f2fs_gc()` execution core is independently proven at `0x3503a8`.

## 9. Vendor control descriptors

### `need_switch_ssr`

```text
registration 0x37af88
descriptor   Image +0x173b9d0
backing      Image +0x1a139c0 (u8)
```

### `tran_urgent_gc`

```text
registration 0x37b068
descriptor   Image +0x173bbb0
backing      Image +0x1a139d0 (u32)
```

### `detect_charger_type`

```text
registration 0x37b184
descriptor   Image +0x173bf70
write handler symbol = detect_charger_type_write
backing storage/consumer = unresolved
```

All three use the common registration framework:

```text
control name
    -> Image +0x1a13a20 common runtime object
    -> 0x274ea0
    -> 0x274dac
    -> per-control descriptor retained by generic registry node
```

They are not proven to be unique standalone function entry points.

## 10. Controller-state correction

This is now authoritative for the project:

```text
Image +0x1a13998 = controller

controller 0 -> direct f2fs_gc(), mode unchanged
controller 1 -> temporary sbi->gc_mode = 2
controller 2 -> temporary sbi->gc_mode = 3
```

Therefore old notes that labeled controller value `2` as "urgent" are superseded.

The actual stock detector stores `+0x998 = 2` for Stop 4/5, so those states must currently be described as **controller state 2 / resulting GREEDY override**, pending recovery of the original vendor symbolic label for the controller state itself.

## 11. Final reconstructed call graph

```text
tran_gc_thread_func / detector
        |
        v
0x366cd4 vendor policy/orchestration
        |
        +-- sbi+0x48 bit3 guard
        |
        +-- policy(4) -> optional gate(0x80)
        |
        +-- policy(1) -> optional helper(455)
        |
        +-- policy(0)
        |      +-- true  -> 0x362c40(sbi,0,0)
        |      +-- false -> 0x363288(sbi,0xE38)
        |
        +-- gc_mode == 3 ?
        |      +-- yes -> bypass seven-field non-mode-3 ladder
        |      +-- no  -> all seven SBI guards must be zero
        |
        +-- fixed-point capacity guard
        |
        +-- policy(1)
        +-- policy(3)
        |
        +-- dirty/reservation guard
        +-- fixed-point guard
        +-- time/current guard
        |
        +-- sbi+0x4b9 bit7 ?
               |
               +-- no  -> return
               +-- yes -> 0x3e1014
                        -> 0x34e224(...,1)
                        -> 0x3e1558
                        -> 0x341250(sb,1)
                        -> stat +0x16c++
```

## 12. Remaining unresolved work

The `0x366cd4` control-flow reconstruction is now substantially complete. Remaining uncertainty is concentrated in symbolic source naming and opaque helper semantics:

```text
seven SBI field source names
selector 0/1/3/4 names
0x373108 exact source identity
0x35d22c exact source identity
0x362c40 exact source identity/return semantics
0x363288 exact source identity/return semantics
0x3e1014 exact source identity
0x34e224 exact source identity
0x3e1558 exact source identity
0x341250 exact source identity
stat_info +0x16c original member name
 detect_charger_type backing storage/consumer
```

These are the remaining items that require another instruction-level producer/consumer pass rather than additional historical-source guessing.
