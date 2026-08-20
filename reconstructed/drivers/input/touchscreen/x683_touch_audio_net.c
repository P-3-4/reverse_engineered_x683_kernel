/* X683 input/audio/network driver surfaces selected by DT + config + kallsyms. */
#include <stdint.h>

struct x683_driver_symbol { const char *name; uint64_t address; };

static const struct x683_driver_symbol x683_touch[] __attribute__((used)) = {
    { "tpd_probe", 0xffffff92d144c1e4ULL },
    { "tpd_fb_notifier_callback", 0xffffff92d144c864ULL },
    { "ilitek_tddi_init", 0xffffff92d14593acULL },
    { "ilitek_spi_probe", 0xffffff92d145a098ULL },
    { "ilitek_plat_probe", 0xffffff92d145b9d8ULL },
    { "ilitek_plat_isr_top_half", 0xffffff92d145b6b8ULL },
    { "ilitek_plat_isr_bottom_half", 0xffffff92d145b838ULL },
    { "tpd_suspend", 0xffffff92d145b960ULL },
    { "tpd_resume", 0xffffff92d145b99cULL },
};

static const struct x683_driver_symbol x683_audio[] __attribute__((used)) = {
    { "mt6768_afe_pcm_platform_probe", 0xffffff92d16697ccULL },
    { "mt6768_afe_pcm_dev_probe", 0xffffff92d16697f8ULL },
    { "mtk_afe_pcm_new", 0xffffff92d1664db8ULL },
    { "mtk_afe_fe_startup", 0xffffff92d1664ec0ULL },
    { "mtk_afe_fe_hw_params", 0xffffff92d1665224ULL },
    { "mtk_afe_fe_trigger", 0xffffff92d1665a68ULL },
    { "mtk_afe_dai_suspend", 0xffffff92d1665f6cULL },
    { "mtk_afe_dai_resume", 0xffffff92d1666040ULL },
};

static const struct x683_driver_symbol x683_network[] __attribute__((used)) = {
    { "mtk_btif_probe", 0xffffff92d10717e0ULL },
    { "mtk_wcn_wlan_gen4_init", 0xffffff8cc185c414ULL },
    { "do_wlan_drv_init", 0xffffff8cc185c564ULL },
    { "wlanProbeSuccessForLowLatency", 0xffffff8cc1a04378ULL },
};

/* Network/WCN symbols outside the Image are module symbols from kallsyms;
 * no module binary was supplied, so no module disassembly is claimed. */
