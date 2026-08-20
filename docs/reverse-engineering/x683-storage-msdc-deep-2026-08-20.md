# X683 Storage / MSDC Deep Reconstruction — 2026-08-20

The boot-image DT contains two enabled `mediatek,msdc` nodes: MSDC0 at `0x11230000` with `msdc0-clock`, `msdc0-hclock`, `msdc0-aes-clock`; and MSDC1 at `0x11240000` with `msdc1-clock`, `msdc1-hclock`.

The config proves `CONFIG_MMC_MTK_PRO=y`, `CONFIG_MTK_EMMC_SUPPORT=y`, `CONFIG_MTK_EMMC_CACHE=y`, and `CONFIG_MTK_EMMC_HW_CQ=y`; generic `CONFIG_MTK_EMMC_CQ_SUPPORT` is disabled.

Exact X683 entry points: `msdc_drv_probe` `0xffffff92d1564ba0`; `msdc_ops_request` `0xffffff92d1566448`; `msdc_do_request_prepare` `0xffffff92d1562f50`; `msdc_do_request` `0xffffff92d15631dc`; `msdc_execute_tuning` `0xffffff92d1564068`; `msdc_irq` `0xffffff92d1565aa0`; `msdc_irq_data_complete` `0xffffff92d1567470`; `msdc_cqhci_reset` `0xffffff92d156705c`; `msdc_cqhci_crypto_cfg` `0xffffff92d15670b8`; `msdc_suspend` `0xffffff92d1567664`; `msdc_resume` `0xffffff92d15676a4`; `msdc_runtime_suspend` `0xffffff92d1567718`; `msdc_runtime_resume` `0xffffff92d156778c`.

`msdc_ops_request` contains direct host/error calls plus an indirect BLR loop over a linked callback structure. The callback target is deliberately unresolved. `msdc_drv_probe` contains low-level translation/address-space setup and initialization branches visible in the key disassembly.

The F2FS source fingerprint includes checkpoint/data/segment/node/recovery and the block/MMC core paths, allowing the persistent XREF generator to connect the filesystem/storage stack without substituting generic upstream implementations.

Private request/CQ/error-state structures remain partially opaque because the vendor driver source and module/debug metadata are not supplied.
