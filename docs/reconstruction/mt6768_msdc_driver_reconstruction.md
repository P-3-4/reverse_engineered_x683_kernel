# MT6768 MSDC Driver Reconstruction

## Evidence source

Sources used:

- x683_kallsyms.txt
- stock boot DTB mapping
- repository storage reconstruction notes

No vendor source assumptions are treated as confirmed.

## Located MSDC symbols

| Symbol | Address | Type | Confidence |
|---|---|---|---|
| msdc_drv_probe | ffffff92d1564ba0 | text | high |
| msdc_add_host | ffffff92d1565a68 | text | high |
| msdc_init_hw | ffffff92d15647dc | text | high |
| msdc_ops_set_ios | ffffff92d15641dc | text | high |
| msdc_ops_request | ffffff92d1566448 | text | high |

## MMC interaction symbols

| Symbol | Address | Role |
|---|---|---|
| mmc_alloc_host | ffffff92d1548464 | MMC host allocation |
| mmc_add_host | ffffff92d1548648 | MMC host registration |

## Reconstructed initialization order

Evidence-supported sequence:

```
msdc_drv_probe()
        |
        v
msdc_add_host()
        |
        v
msdc_init_hw()
        |
        v
MMC host registration
        |
        v
MMC block layer
```

## DT dependencies confirmed

Node:

```
msdc@11240000
```

Properties:

```
compatible = mediatek,msdc
index = 1
bus-width = 4
```

Dependencies:

```
clock:
    msdc1-clock
    msdc1-hclock

regulators:
    vmmc-supply
    vqmmc-supply
```

## Remaining unknowns

UNKNOWN until binary analysis:

- exact clock lookup calls
- exact regulator acquisition sequence
- pinctrl state names
- DMA setup details
- host private structure layout
- interrupt registration details

Next analysis target:

- disassemble msdc_drv_probe
- recover call graph
- map DT property reads
- reconstruct minimal compatible driver path
