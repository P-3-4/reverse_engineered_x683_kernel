# X683 Device-Tree → Driver Initialization — 2026-08-20

The standalone `device-tree.tar.gz` is incomplete: it contains only a `proc/device-tree` symlink. A complete DT table was recovered from the boot image.

High-value bindings proven by DT + kallsyms include:

| node | compatible | X683 symbol/path |
|---|---|---|
| `/msdc@11230000` | `mediatek,msdc` | `msdc_drv_probe` |
| `/msdc@11240000` | `mediatek,msdc` | `msdc_drv_probe` |
| `/m4u@10205000` | `mediatek,m4u` | `m4u_probe` |
| `/mt6358_gauge` | `mediatek,mt6358_gauge` | `mt6358_gauge_probe` |
| `/mt_cpufreq` | `mediatek,mt-cpufreq` | `_mt_cpufreq_pdrv_probe` |
| `/gpufreq` | `mediatek,mt6768-gpufreq` | `__mt_gpufreq_pdrv_probe` |
| `/mt_charger` | `mediatek,mt-charger` | `mt_charger_probe` |
| `/charger` | `mediatek,charger` | `mtk_charger_probe` |
| `/btscharger` | `tran,ts_btscharger` | `tran_btscharger_probe` |
| `/touch` | `mediatek,touch` | `tpd_probe` / Ilitek path |
| `/audio@11220000` | `mediatek,audio` | `mt6768_afe_pcm_dev_probe` |
| `/dsi0@14014000` | `mediatek,dsi0` | DSI/DDP path; exact probe container unresolved |
| `/wifi@18000000` | `mediatek,wifi` | WMT/WLAN module path |

The parsed DT has 542 nodes and 382 nodes with `compatible` and/or `reg`. Exact initcall-table ordering is still unresolved because the full per-driver initcall pointer relationship has not been proven.
