# X683/H694 stock boot.img — direct gc_mode evidence

## Image identity

- File: stock `boot.img`
- Size: 33,554,432 bytes
- SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- Android boot header: `ANDROID!`
- Board/name: `CY-X683-H694-E`
- Page size: 2048
- Kernel compressed size: 9,755,348 bytes
- Ramdisk compressed size: 943,464 bytes
- Kernel address: `0x40080000`
- Kernel text offset: `0x80000`
- Decompressed kernel: AArch64 Linux Image, 26 MiB

The kernel was extracted from this exact image and analyzed directly. The full boot image does not need to be re-uploaded for this evidence set; future analysis can use the recorded image hash plus these offsets/evidence artifacts.

## `sbi + 0x534` is directly proven as `gc_mode`

The stock kernel contains multiple direct 32-bit accesses to `[sbi + 0x534]`. The strongest evidence is in the Transsion GC wrapper at kernel-image offset `0x37ada8`:

```text
37add8  ldr     w21, [x19, #0x534]       // save current gc_mode
37adec  mov     w2, #1
37adf0  mov     w3, #-1
37adf8  mov     w0, x19
37adfc  str     w9, [x19, #0x534]       // temporary override
37ae00  bl      0x3503a8
37ae04  str     w21, [x19, #0x534]       // restore previous mode
```

The same function has a second policy branch:

```text
37ae50  mov     w9, #2
37ae68  str     w9, [x19, #0x534]       // temporary GC_GREEDY
37ae6c  bl      0x3503a8
37ae7c  str     w21, [x19, #0x534]       // restore
```

This is direct machine-code evidence, not a historical-layout inference.

## Exact Transsion policy behavior

The wrapper at `0x37ada8` reads a Transsion GC control value from a separate global structure at `+0x998` and selects the temporary `gc_mode` override:

```text
control == 0:
    call f2fs_gc() without overriding gc_mode

control == 1:
    gc_mode = 2  (GREEDY)
    f2fs_gc()
    restore previous gc_mode

control == 2:
    gc_mode = 3  (URGENT)
    f2fs_gc()
    restore previous gc_mode
```

The kernel then logs the selected policy using the literal strings:

- `gc mode is COST`
- `gc mode is URGENT`
- `gc mode is GREEDY`

The corresponding references are at kernel offsets `0x37ae44`, `0x37ae14`, and `0x37ae74`.

## Direct upstream sysfs writers

The kernel also contains the normal F2FS `gc_idle` / `gc_urgent` controls. In the sysfs handler region beginning at `0x374d4c`, the following stores are present:

```text
3750f4  str w9, [x19, #0x534]    // w9 = 3
37515c  str w8, [x19, #0x534]    // input value 1 or 2
375168  str wzr,[x19, #0x534]    // NORMAL
375174  str wzr,[x19, #0x534]    // NORMAL fallback
```

The `3750f4` path also wakes the associated GC machinery, matching the urgent-GC behavior. The `37515c/375168` paths correspond to the idle-GC policy control.

Therefore the X683 image contains **two distinct ways to influence `gc_mode`**:

1. Standard F2FS sysfs `gc_idle` / `gc_urgent` control.
2. A Transsion-specific GC wrapper that temporarily overrides `gc_mode` immediately around `f2fs_gc()` and restores the prior value.

## Transsion GC wrapper

The wrapper is strongly identified by adjacent literals and control flow:

- `tran_f2fs_gc`
- `gc mode is COST`
- `gc mode is URGENT`
- `gc mode is GREEDY`
- `tran_gc_usb_wakelock`
- `TRAN_GC %s stopped.`
- `tran_gc_thread_func create`
- `tran_gc loop static detect`
- `kernel or os is holding wakelock!`
- `f2fs is writing data`
- `match: Stop condition 4, dec_seg=%d, inc_written_seg=%d, switch to SSR`

The wrapper calls the stock GC implementation at `0x3503a8`.

## Other direct `0x534` evidence

Additional accesses were found at:

```text
0x345d58  ldr w27, [x19, #0x534]
0x345d6c  str w25, [x19, #0x534]   // w25 = 3
0x345d78  str w27, [x19, #0x534]   // restore

0x3500a4  ldr w8,  [x19, #0x534]
0x352f10  ldr w11, [x22, #0x534]
0x352f58  ldr w12, [x22, #0x534]
0x365918  ldr w14, [x0,  #0x534]
0x3750f4  str w9,  [x19, #0x534]   // 3
0x37515c  str w8,  [x19, #0x534]   // 1/2
0x375168  str wzr, [x19, #0x534]   // 0
0x375174  str wzr, [x19, #0x534]   // 0
0x37add8  ldr w21, [x19, #0x534]
0x37adfc  str w9,  [x19, #0x534]   // 3
0x37ae04  str w21, [x19, #0x534]   // restore
0x37ae68  str w9,  [x19, #0x534]   // 2
0x37ae7c  str w21, [x19, #0x534]   // restore
0x8d3598  ldr w1,  [x8,  #0x534]   // unrelated structure, not sbi
0xcba45c  ldr w9,  [x19, #0x534]
0xcbd4bc  str w8,  [x19, #0x534]
0xcc4370  str w8,  [x19, #0x534]
```

The `0x37ada8` wrapper is the highest-confidence vendor-specific evidence because its argument is the F2FS superblock pointer and its call target is the reconstructed stock `f2fs_gc()`.

## State model now supported by the stock image

```text
                    Transsion GC controller
                              |
             +----------------+----------------+
             |                                 |
        normal/COST                         override
             |                                 |
       f2fs_gc()                       +-------+-------+
                                       |               |
                                    mode 2           mode 3
                                   GREEDY           URGENT
                                       |               |
                                  f2fs_gc()        f2fs_gc()
                                       |               |
                                       +-------+-------+
                                               |
                                      restore old gc_mode
```

Separately, upstream-style sysfs controls can directly establish:

```text
0 = NORMAL
1 = IDLE_CB / COST
2 = IDLE_GREEDY / GREEDY
3 = URGENT
```

The Transsion wrapper does not leave its temporary override installed; it explicitly restores the previous value.

## Remaining questions

1. Identify the exact Transsion global at `0x1a130000 + 0x998` and its enum/name from the surrounding GC structure.
2. Reconstruct the complete `tran_f2fs_gc()` control algorithm around `0x37ada8`.
3. Trace the static-detection loop at `0x376f84`–`0x377b74` and its charging/USB/wakelock/framebuffer predicates.
4. Match the Transsion GC control value to `/proc`/sysfs debug nodes (`tran_urgent_gc`, `need_switch_ssr`, `detect_charger_type`, etc.).
5. Determine whether `0x345d58` is a second vendor urgent-GC entry point or another stock/vendor caller.

## Method

All conclusions above come from direct AArch64 disassembly and literal/string cross-references in the supplied stock X683/H694 boot image. Historical F2FS source is used only for semantic naming/correlation; it is not substituted for the machine-code evidence.
