# Transsion GC state machine — direct stock reconstruction

## Stock entry point

The supplied X683/H694 boot image contains a Transsion wrapper at kernel-image offset `0x37ada8`. The wrapper calls the stock F2FS GC entry point at `0x3503a8`.

## Controller

At `0x37adc4`:

```text
ldr w8, [x8, #0x998]
```

The value is a Transsion GC controller state in a separate global structure. The structure's complete semantic layout is still being recovered.

## Exact policy branches

### Controller = 0

```text
37add4  cbz w8, 37ae28
37ae28  ldr  w8, [sbi + 0x4b8]
37ae38  ubfx w1, w8, #14, #1
37ae3c  bl   0x3503a8
```

No temporary `gc_mode` override is installed.

### Controller = 2

```text
37add8  ldr  w21, [sbi + 0x534]
37addc  cmp  w8, #2
37ade8  mov  w9, #3
37adec  mov  w2, #1
37adf0  mov  w3, #-1
37adf4  ubfx w1, w8, #14, #1
37adfc  str  w9, [sbi + 0x534]
37ae00  bl   0x3503a8
37ae04  str  w21, [sbi + 0x534]
```

Therefore controller state 2 forces `gc_mode = 3` for one GC invocation, then restores the previous mode.

### Controller = 1 or other nonzero/non-2 state handled by this branch

```text
37ae50  ldr  w8, [sbi + 0x4b8]
37ae54  mov  w9, #2
37ae58  mov  w2, #1
37ae5c  mov  w3, #-1
37ae60  ubfx w1, w8, #14, #1
37ae68  str  w9, [sbi + 0x534]
37ae6c  bl   0x3503a8
37ae7c  str  w21, [sbi + 0x534]
```

The wrapper therefore temporarily forces `gc_mode = 2` and restores the old value.

## Meaning of the modes

The adjacent stock literals are:

```text
10a5e7a  gc mode is COST
10a5e8b  gc mode is URGENT
10a5e9e  gc mode is GREEDY
```

Their direct references occur at:

```text
37ae44  -> COST
37ae14  -> URGENT
37ae74  -> GREEDY
```

This provides direct semantic confirmation for the policy values.

## Standard F2FS control path

A separate sysfs handler around `0x374d4c` directly writes the same field:

```text
3750f4  str w9,  [sbi + 0x534]   // 3, urgent
37515c  str w8,  [sbi + 0x534]   // idle value 1/2
375168  str wzr, [sbi + 0x534]   // normal
375174  str wzr, [sbi + 0x534]   // normal fallback
```

Thus `gc_mode` has both an upstream control interface and a vendor wrapper override.

## Transsion-specific evidence around the wrapper

The same kernel contains:

```text
tran_f2fs_gc
tran_gc_thread_func create
tran_gc loop static detect
tran_gc_usb_wakelock
kernel or os is holding wakelock!
f2fs is writing data
match: Stop condition 4, dec_seg=%d, inc_written_seg=%d, switch to SSR
tran_urgent_gc
need_switch_ssr
detect_charger_type
```

The static-detection thread is therefore not speculative: it is compiled into the stock image and directly surrounds the Transsion GC machinery.

## Correct state-machine model

```text
                 Transsion controller (+0x998)
                              |
             +----------------+----------------+
             |                |                |
             0                1                2
             |                |                |
       preserve mode       mode = 2        mode = 3
             |             GREEDY           URGENT
             |                |                |
             +----------------+----------------+
                              |
                    f2fs_gc(sbi, sync,
                         true, -1)
                              |
                       restore old mode
```

The temporary nature of the vendor override is now proven by the paired load/store around the GC call.

## Next target

Recover the structure containing `+0x998` and map its fields to the Transsion `/proc` debug names. Then trace the static-detection loop to determine exactly which charging/USB/display/wakelock conditions set the controller to 0/1/2.
