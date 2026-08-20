# X683 Input / Audio / Network / Security — 2026-08-20

## Touch

The config enables the MediaTek touchscreen framework and both Ilitek no-flash and NT36xxx no-flash common paths. Key symbols: `tpd_probe` `0xffffff92d144c1e4`; `tpd_fb_notifier_callback` `0xffffff92d144c864`; `ilitek_tddi_init` `0xffffff92d14593ac`; `ilitek_spi_probe` `0xffffff92d145a098`; `ilitek_plat_probe` `0xffffff92d145b9d8`; `ilitek_plat_isr_top_half` `0xffffff92d145b6b8`; `ilitek_plat_isr_bottom_half` `0xffffff92d145b838`; `tpd_suspend` `0xffffff92d145b960`; `tpd_resume` `0xffffff92d145b99c`.

## Audio

The DT contains MediaTek audio, MT6768/MT6358 sound and audio SRAM nodes. Key entry points include `mt6768_afe_pcm_platform_probe`, `mt6768_afe_pcm_dev_probe`, `mtk_afe_pcm_new`, FE startup/hw_params/trigger and AFE suspend/resume.

## Network

The DT contains MediaTek Wi-Fi and BTIF. WMT/WLAN symbols are outside the supplied Image and therefore are module evidence only. No module disassembly is claimed because the module binaries were not supplied.

## Security

The actual config proves SELinux, F2FS/FS encryption, dm-verity/FEC, AES/XTS/GCM/SHA families and Microtrust TEE support; generic `CONFIG_TEE` is disabled. Configuration is not promoted to runtime-execution proof without call/data-reference evidence.
