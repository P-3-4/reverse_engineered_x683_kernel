# X683 Scheduler / PPM / Power / Thermal Deep Reconstruction — 2026-08-20

Scheduler symbols: `schedtune_enqueue_task` `0xffffff92d0b26944`; `schedtune_dequeue_task` `0xffffff92d0b26ca4`; `schedtune_cpu_boost` `0xffffff92d0b26e20`; `schedtune_task_boost` `0xffffff92d0b26f78`; `schedtune_prefer_idle` `0xffffff92d0b26fc4`.

PPM/DVFS symbols: `ppm_limit_callback` `0xffffff92d0f814f0`; `ppm_main_suspend` `0xffffff92d0fa0cdc`; `ppm_main_resume` `0xffffff92d0fa0d78`; `ppm_main_pdrv_probe` `0xffffff92d0fa0e10`; `mt_ppm_main` `0xffffff92d0fa14d4`; `mt_ppm_set_dvfs_table` `0xffffff92d0fa3808`; `mt_ppm_register_client` `0xffffff92d0fa39c4`; `_mt_cpufreq_pdrv_probe` `0xffffff92d0f81200`; `cpufreq_driver_target` `0xffffff92d15375e4`.

Thermal symbols include `tscpu_thermal_probe` `0xffffff92d1345ef4`, `mtk_thermal_zone_device_register_wrapper` `0xffffff92d133d930`, `thermal_zone_device_update` `0xffffff92d151188c`, and `thermal_cooling_device_register` `0xffffff92d151242c`.

The DT contains `/mt_cpufreq` (`mediatek,mt-cpufreq`) and `/gpufreq` (`mediatek,mt6768-gpufreq`). The config proves schedtune, HMP/HMP+, EAS, MTK scheduler boost, schedutil, CPUFREQ, cpuidle, EEM, PBM and EARA thermal support.

GPU PM entry points include `kbase_pm_suspend`, `kbase_pm_resume`, `kbase_pm_do_poweron` and `kbase_pm_do_poweroff`. Exact private callback/client containers remain indirect and offset-backed.
