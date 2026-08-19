# X683 Scheduler / Power / Display / Battery Integration — 2026-08-19

## Scheduler

`CONFIG_SCHED_TUNE=y`. `schedtune_enqueue_task` = `0xffffff92d0b26944` and is directly called by both `enqueue_task_fair()` and `enqueue_task_rt()`. The image also contains task boost, CPU boost and prefer-idle hooks.

## PPM/DVFS

MediaTek PPM exposes game-mode, suspend/resume, DLPT, thermal, battery-protection, display-off and user-limit callbacks. Exact init symbols include `_mt_cpufreq_pdrv_init`, `mtk_cpuidle_init`, `ppm_platform_init`, `eem_init`, `pbm_module_init`, `spm_fs_init` and `mtk_thermal_platform_init`.

## Display

`fb_register_client()` is called by `tran_gc_init`, PPM display-off policy, GED and other display clients. `fb_event()` = `0xffffff92d0dfacf8` directly wakes the Transsion GC waitqueue. PPM and GED framebuffer callbacks consume the same notifier chain.

## Battery / charger

`mt6358_gauge_probe` = `0xffffff92d0fbb448` and registers the MT6358 gauge. `do_charger_detect` uses USB/PMIC charger detection. `battery_update()` directly calls battery measurement helpers and `power_supply_changed()`.

`tran_battery_probe` = `0xffffff92d150cb90`, size `0x72c`, and directly reads DT properties and creates device attributes.

## Cross-subsystem graph

```text
scheduler -> PPM -> cpufreq/DVFS/thermal
battery/charger -> PPM battery protection + USB events
framebuffer -> PPM/GED + Transsion GC wakeup
storage -> PM/cpufreq/thermal policy
```

## Reconstruction

New source models:

- `reconstructed/drivers/misc/mediatek/sched/x683_scheduler_policy_reconstructed.c`
- `reconstructed/drivers/misc/mediatek/display/x683_fb_policy_reconstructed.c`
- `reconstructed/drivers/power/supply/x683_battery_gauge_reconstructed.c`
- `reconstructed/drivers/misc/mediatek/base/power/x683_power_paths_reconstructed.c`
