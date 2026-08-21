# X683 Storage Hardware / Block Path — 2026-08-19

## Hardware conclusion

X683 enables MediaTek eMMC/MSDC support: `CONFIG_MMC=y`, `CONFIG_MMC_BLOCK=y`, `CONFIG_MMC_MTK_PRO=y`, `CONFIG_MTK_EMMC_SUPPORT=y`, `CONFIG_MTK_EMMC_HW_CQ=y`. The recovered runtime storage surface is `mmc_*`/`msdc_*`; UFS is not the primary configured storage path.

## Key functions

```text
msdc_drv_probe        0xffffff92d1564ba0  size 0x808
msdc_ops_request      0xffffff92d1566448  size 0x764
msdc_irq              0xffffff92d1565aa0  size 0x69c
msdc_execute_tuning   0xffffff92d1564068
msdc_dma_setup        0xffffff92d1562634
msdc_dma_start        0xffffff92d1562c3c
msdc_dma_stop         0xffffff92d1562d6c
msdc_runtime_suspend  0xffffff92d1567718  size 0x74
msdc_runtime_resume   0xffffff92d156778c
msdc_dt_init          0xffffff92d15772ec
```

`msdc_drv_probe()` allocates an MMC host, parses DT state, initializes CMDQ and clock resources. `msdc_ops_request()` reaches request preparation, crypto setup, command handling and error/tuning paths. `msdc_irq()` is the completion/error interrupt surface.

## Runtime path

```text
F2FS -> bio -> MMC block/CMDQ -> msdc_ops_request
     -> DMA/command setup -> msdc_irq -> completion/error tuning
```

The driver also contains eMMC crypto, cache, tuning and runtime PM surfaces.

## PM

`msdc_suspend()` directly calls the runtime suspend path. Runtime suspend disables/unprepares three clocks and updates PM QoS state. Resume reverses this resource path.

## Reconstruction

`reconstructed/drivers/mmc/host/mediatek/ComboA/mt6768/msdc_reconstructed.c`
