# X683/H694 vendor GC control registration binding

This is reconstructed/inferred from the verified stock X683/H694 boot image. It is not proprietary Transsion source.

## Binary authority

Boot image SHA-256:

`a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`

Decompressed Image SHA-256:

`96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

All addresses below are decompressed Image offsets.

## Registration routine

The vendor GC debug/control registration sequence beginning around `0x37b2f0` repeatedly calls a common helper at `0x274ec8`.

The setup loads the same global descriptor/object pointer from:

```text
Image + 0x1698a20
```

and passes that object as the second argument while passing each control name string as the first argument.

Therefore the sequence is a **generic attribute/control registration table**, not a collection of direct calls to `need_switch_ssr()`, `tran_urgent_gc()`, or `detect_charger_type()` implementations.

## Exact named-control call sites

| Control | Registration call | Name string offset | Descriptor/data slot index | qword-0 pointer |
|---|---:|---:|---:|---|
| `need_switch_ssr` | `0x37b3e4` | `0x10a6359` | 9 | `0xffffff80099c5558` |
| `tran_urgent_gc` | `0x37b484` | `0x10a63ac` | 19 | `0xffffff80099c55b0` |
| `detect_charger_type` | `0x37b504` | `0x10a6414` | 27 | `0xffffff80099c55f8` |

The index is the position in the ordered 24-byte descriptor sequence beginning at the object around `0x1698a20`. The first qword is treated here only as a **descriptor/data pointer**. It is not claimed to be a function pointer.

## Other controls in the same sequence

The same helper registers, in order, controls including:

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

This ordering makes the registration-table interpretation strong: the strings are not random nearby literals.

## Common helper `0x274ec8`

Direct body inspection shows `0x274ec8` is a generic registry/attribute helper. It saves the input arguments, performs lookup/allocation/list traversal, and has an error path for a missing registration target.

It is therefore **not** the implementation of any of the three named controls.

The common helper receives:

```text
x0 = control-name string
x1 = common vendor descriptor/object
```

and performs generic registration/lookup work.

## What is actually bound

### `need_switch_ssr`

High-confidence binding:

```text
string 0x10a6359
    -> registration call 0x37b3e4
    -> descriptor index 9
    -> descriptor/data pointer 0xffffff80099c5558
    -> generic helper 0x274ec8
```

This proves `need_switch_ssr` is represented in the vendor control/attribute descriptor and identifies its associated data slot.

It does **not** yet prove that `0xffffff80099c5558` is the function implementing the decision.

### `tran_urgent_gc`

```text
string 0x10a63ac
    -> registration call 0x37b484
    -> descriptor index 19
    -> descriptor/data pointer 0xffffff80099c55b0
    -> generic helper 0x274ec8
```

Again, this proves the control/data binding, not a callback address.

The independently proven GC state machine still establishes that controller state `2` causes the wrapper to temporarily force `gc_mode = 3` (URGENT) before calling stock `f2fs_gc()`.

### `detect_charger_type`

```text
string 0x10a6414
    -> registration call 0x37b504
    -> descriptor index 27
    -> descriptor/data pointer 0xffffff80099c55f8
    -> generic helper 0x274ec8
```

This is a control/data binding. Its exact runtime producer/consumer is still indirect.

### `tran_gc_usb_wakelock`

`tran_gc_usb_wakelock` exists at string offset `0x10a5ee7`, but it is **not present in this particular contiguous registration sequence**. Do not attach it to the same descriptor table without locating its separate registration/use site.

## Why these are not direct function calls

The descriptor region around `0x1698a20` is heterogeneous: adjacent qwords include pointers into kernel data/BSS and pointers into executable image regions. The structure is therefore more complex than a simple `{name, callback, flags}` array, and the qword-0 fields above cannot safely be called callbacks without tracing the generic helper's field interpretation.

Accordingly:

```text
need_switch_ssr   != proven function symbol
tran_urgent_gc    != proven function symbol
detect_charger_type != proven function symbol
```

What is proven is:

```text
name -> vendor control descriptor -> data/state slot -> generic registration
```

## Relation to the GC detector

The direct stock detector/wrapper path remains:

```text
Stop 4
  -> +0x998 = 2 (unless +0x9c0 blocks)
  -> +0x9f8 = 1
  -> tran_f2fs_gc
  -> temporary sbi+0x534 = 3
  -> f2fs_gc(sbi, sync, true, -1)
  -> restore gc_mode
```

Thus `need_switch_ssr` and `tran_urgent_gc` are currently best treated as **vendor control/state interfaces surrounding the proven controller state machine**, not as direct calls in the Stop-4 basic block.

## Remaining binding work

The remaining exact path is:

```text
0x274ec8
  -> determine descriptor field offsets used for read/show/write callbacks
  -> trace descriptor indices 9, 19, 27 through those fields
  -> find all direct loads/stores of the associated BSS/data pointers
  -> bind the resulting state variables to the detector/wrapper
```

Only after that should an implementation name be assigned to `need_switch_ssr`, `tran_urgent_gc`, or `detect_charger_type`.
