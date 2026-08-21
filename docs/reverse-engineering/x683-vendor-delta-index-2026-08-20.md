# X683 Vendor Delta Index — 2026-08-20

## High-confidence configuration deltas

The actual X683 config identifies the Transsion project as `tran_x683`, BOM `tran_x683_e1`, PCBA `tran_pcba_h694`, and enables Transsion-specific IO/cgroup, performance, battery/charger, framebuffer, binder-monitor, and power features.

| Area | X683 evidence | Classification |
|---|---|---|
| Product identity | `CONFIG_CUSTOM_TRAN_PROJECT="tran_x683"`, `CONFIG_CUSTOM_TRAN_BOM="tran_x683_e1"`, `CONFIG_CUSTOM_TRAN_PCBA="tran_pcba_h694"` | Proven vendor build identity |
| IO | `CONFIG_TRAN_IOCFQ_CGROUP_SUPPORT=y` | Proven vendor config delta |
| Performance | `CONFIG_TRAN_PERF_LOG=y` | Proven vendor config delta |
| Binder | `CONFIG_TRAN_ANDROID_BINDER_BLOCK_MONITOR=y`, `CONFIG_TRAN_ANDROID_BINDER_BUFFER_EXHAUST_MONITOR=y` | Proven vendor config delta |
| F2FS | `CONFIG_F2FS_TRAN_GC=y` | Proven vendor config delta |
| Display | `CONFIG_TRAN_LCM_POWER_GPIO=y`, `CONFIG_TRAN_AAL_SET_SODI=y`, `CONFIG_TRAN_LCM_SHUTDOWN_ENABLE=y` | Proven vendor config delta |
| Charger | Transsion OTG/aging/JEITA/BTS charger options are enabled | Proven vendor config delta |
| FPSGO | `CONFIG_MTK_FPSGO_V3=y`; related `CONFIG_TRAN_PNP_FPSGO_BOOST` is disabled | Mixed MTK/vendor boundary; behavior requires binary evidence |
| ION | `CONFIG_MTK_ION_CACHE_OPTIMIZATION=y` | Proven MTK vendor feature |
| Scheduler | `CONFIG_SCHED_TUNE=y`, UCLAMP, EAS plus MTK/Transsion scheduler surfaces | Proven feature boundary; exact policy logic still binary-dependent |

## Symbol-side vendor surface

The kallsyms inventory contains vendor-likely symbol families including `msdc`, `m4u`, `ion`, `ppm`, `eem`, `pbm`, `ged`, `mali`, `wmt`, `btif`, `mt6358`, `fpsgo` and `tran_*`. Names alone are not treated as proof of behavior; all unknown relationships remain marked unresolved.
