# MT6768 Storage Reconstruction State

## Scope

This document records evidence-backed reconstruction progress for the X683/H694 MT6768 kernel.

## Confirmed evidence

- Target kernel generation: Linux 4.14.141 vendor modified.
- Architecture: ARM64.
- Kernel configuration enables MediaTek MMC/eMMC support.
- Kernel configuration enables F2FS and Transsion GC support.

## Verified F2FS vendor integration

Recovered symbols include:

- tran_gc_init
- tran_gc_stop
- tran_gc_thread_func
- tran_do_f2fs_gc

The recovered GC path uses:

```
tran_do_f2fs_gc()
    -> f2fs_gc(sbi, sync, true, NULL_SEGNO)
```

## Storage reconstruction status

The confirmed boot-critical storage chain is:

```
F2FS
  |
block layer
  |
MMC block
  |
MediaTek eMMC/MSDC vendor stack
  |
MT6768 platform
```

## Verified DTB dependency extraction

From the stock boot image DTB:

```
/msdc@11240000
```

Confirmed properties:

```
compatible = mediatek,msdc
index = 1
bus-width = 4
status = okay
```

Confirmed clock dependencies:

```
msdc1-clock
msdc1-hclock
```

Confirmed regulator dependencies:

```
vmmc-supply  -> MT6358 ldo_vmch
vqmmc-supply -> MT6358 ldo_vmc
```

These findings are based on extracted stock DTB data. They do not represent a generic MT6768 reference implementation.

## Remaining unknowns

- Exact vendor MSDC source ownership.
- Exact MSDC probe implementation details.
- Clock ID mapping inside the MT6768 clock provider.
- Full regulator sequencing during MMC initialization.
- Missing vendor source files.
