# MT6768 Boot Dependency Reconstruction Graph

## Purpose

Evidence-backed dependency ordering for the X683/H694 MT6768 Linux 4.14.141 kernel reconstruction.

This document records only confirmed relationships from stock artifacts and existing reconstruction notes.

## Boot critical dependency chain

```
ARM64 kernel entry
        |
        v
MT6768 platform initialization
        |
        +----------------+
        |                |
        v                v
clock provider     pinctrl provider
        |                |
        +-------+--------+
                |
                v
          regulator / PMIC
                |
                v
          MediaTek MSDC host
          msdc@11240000
                |
                v
             MMC core
                |
                v
            mmc block
                |
                v
              F2FS
                |
                v
          Android userspace
```

## Confirmed DTB evidence

Storage node:

```
msdc@11240000
```

Properties confirmed:

```
compatible = mediatek,msdc
index = 1
bus-width = 4
status = okay
```

Clock dependencies:

```
msdc1-clock
msdc1-hclock
```

Regulator dependencies:

```
vmmc-supply  -> MT6358 ldo_vmch
vqmmc-supply -> MT6358 ldo_vmc
```

## Reconstruction boundary

Known:

- DT storage node exists.
- MSDC depends on clock, regulator, and pin configuration.
- MMC/F2FS depend on successful block device initialization.

Unknown:

- Exact vendor probe ordering.
- Exact clock gate IDs.
- Exact regulator enable sequence.
- Exact Transsion MSDC source layout.

Unknown items remain unresolved until recovered from binary evidence.
