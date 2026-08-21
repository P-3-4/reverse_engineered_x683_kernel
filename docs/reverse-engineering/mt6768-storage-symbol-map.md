# X683/H694 MT6768 storage/platform reconstruction map

This map records direct evidence from the recovered X683/H694 configuration and `kallsyms`. It is an integration target, not a claim that generic MT6768 source is identical to the stock implementation.

## Configuration evidence

Recovered Linux configuration identifies the platform and storage model as:

```text
CONFIG_MTK_PLATFORM="mt6768"
CONFIG_MMC=y
CONFIG_MMC_BLOCK=y
CONFIG_MMC_MTK_PRO=y
CONFIG_MTK_MMC_DEBUG=y
```

The generic `CONFIG_MMC_MTK` and `CONFIG_MMC_MTK_SDIO` options are disabled. The recovered configuration therefore points at the vendor/proprietary `MMC_MTK_PRO` path rather than the generic MMC-MTK implementation.

Other boot-critical platform configuration includes:

```text
CONFIG_ARM64=y
CONFIG_ARM64_4K_PAGES=y
CONFIG_ARM64_VA_BITS=39
CONFIG_OF=y
CONFIG_OF_FLATTREE=y
CONFIG_OF_EARLY_FLATTREE=y
CONFIG_PINCTRL_MTK_V2=y
CONFIG_PINCTRL_MTK_PARIS=y
CONFIG_PINCTRL_MT6768=y
CONFIG_REGULATOR_MT6358=y
CONFIG_MTK_PMIC_CHIP_MT6358=y
CONFIG_SERIAL_8250_MT6577=y
CONFIG_SERIAL_8250_CONSOLE=y
```

The recovered built-in command line is:

```text
console=tty0 console=ttyMT3,921600n1 root=/dev/ram vmalloc=496M slub_max_order=0 slub_debug=O 
```

## Direct storage symbol evidence

The recovered `kallsyms` contains a substantial MSDC implementation. Representative addresses:

```text
ffffff92d155d4dc  msdc_dump_register_core
ffffff92d155f32c  msdc_do_command
ffffff92d1560a4c  msdc_pio_read
ffffff92d156127c  msdc_pio_write
ffffff92d1561dac  msdc_rw_cmd_using_sync_dma
ffffff92d1562f50  msdc_do_request_prepare
ffffff92d15631dc  msdc_do_request
ffffff92d1563bd4  msdc_error_tuning
ffffff92d1564068  msdc_execute_tuning
ffffff92d15641dc  msdc_ops_set_ios
ffffff92d1564ba0  msdc_drv_probe
ffffff92d1565aa0  msdc_irq
ffffff92d1566248  msdc_post_req
ffffff92d1566334  msdc_pre_req
ffffff92d1566448  msdc_ops_request
ffffff92d1566cc0  msdc_ops_enable_sdio_irq
ffffff92d156705c  msdc_cqhci_reset
ffffff92d15670b8  msdc_cqhci_crypto_cfg
ffffff92d1567414  msdc_cqhci_pre_irq_complete
ffffff92d1567664  msdc_suspend
ffffff92d15676a4  msdc_resume
ffffff92d1567718  msdc_runtime_suspend
ffffff92d156778c  msdc_runtime_resume
ffffff92d1576ce0  msdc_of_parse
ffffff92d15772ec  msdc_dt_init
ffffff92d157e240  autok_msdc_tx_setting
ffffff92d157ec64  autok_msdc_device_rx_set
ffffff92d157f394  autok_msdc_device_rx_get
```

These symbols establish that the stock image contains its own MSDC host driver, DT parsing/init path, DMA/request path, tuning/autok path, CQHCI hooks, and PM/runtime-PM callbacks.

## MT6768 platform ownership evidence

Direct symbols also include:

```text
ffffff92d0ec2634  mt6768_pinctrl_probe
ffffff92d11ad0dc  mt6768_devapc_probe
ffffff92d11ad0f8  mt6768_devapc_remove
ffffff92d11ad10c  mt6768_devapc_dbg_read
ffffff92d11ad120  mt6768_devapc_dbg_write
```

This gives direct binary evidence for at least MT6768 pinctrl and DEVAPC platform drivers. It does not by itself identify their complete DTS ownership or source directory.

## Storage reconstruction priority

The evidence changes the storage phase from a generic “find an MMC driver” task to a specific reconstruction target:

1. recover the historical/vendor MSDC implementation matching the symbol family above;
2. recover the DT bindings consumed by `msdc_of_parse()` and `msdc_dt_init()`;
3. map the X683 DT node to the correct MSDC host instance;
4. preserve the CQHCI/crypto and tuning paths enabled by the stock binary;
5. validate the resulting host against the stock symbol/call graph before attempting F2FS userdata boot.

## What is not yet proven

The evidence does **not** yet prove:

- the exact source-tree path of the stock MSDC driver;
- the exact DT node name/compatible/reg/interrupt values for the X683 host;
- which public MT6768 kernel revision is the correct source baseline;
- the complete CQHCI/crypto implementation and its structure offsets;
- the exact ownership of all `msdc_*` helpers between the host driver and platform support files.

Those remain binary/source correlation tasks rather than assumptions to be filled from a generic MT6768 tree.
