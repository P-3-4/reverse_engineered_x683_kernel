/* X683 DT -> driver binding evidence extracted from the boot-image DTB. */
#include <stdint.h>

struct x683_dt_driver_binding {
    const char *node;
    const char *compatible;
    const char *probe_symbol;
    uint64_t base;
    uint64_t size;
};

static const struct x683_dt_driver_binding x683_dt_driver_bindings[] __attribute__((used)) = {
    { "/msdc@11230000", "mediatek,msdc", "msdc_drv_probe", 0x11230000, 0x10000 },
    { "/msdc@11240000", "mediatek,msdc", "msdc_drv_probe", 0x11240000, 0x10000 },
    { "/m4u@10205000", "mediatek,m4u", "m4u_probe", 0x10205000, 0x1000 },
    { "/mt6358_gauge", "mediatek,mt6358_gauge", "mt6358_gauge_probe", 0, 0 },
    { "/mt_cpufreq", "mediatek,mt-cpufreq", "_mt_cpufreq_pdrv_probe", 0, 0 },
    { "/gpufreq", "mediatek,mt6768-gpufreq", "__mt_gpufreq_pdrv_probe", 0, 0 },
    { "/mt_charger", "mediatek,mt-charger", "mt_charger_probe", 0, 0 },
    { "/charger", "mediatek,charger", "mtk_charger_probe", 0, 0 },
    { "/btscharger", "tran,ts_btscharger", "tran_btscharger_probe", 0, 0 },
    { "/touch", "mediatek,touch", "tpd_probe", 0, 0 },
    { "/audio@11220000", "mediatek,audio", "mt6768_afe_pcm_dev_probe", 0x11220000, 0x1000 },
    { "/dsi0@14014000", "mediatek,dsi0", "ddp_dsi / DSI path", 0x14014000, 0x1000 },
    { "/wifi@18000000", "mediatek,wifi", "wmt/wlan module path", 0x18000000, 0x100000 },
};

/* Probe names are promoted only where symbol naming and DT compatibility
 * agree. DSI/Wi-Fi deliberately retain path-level uncertainty. */
