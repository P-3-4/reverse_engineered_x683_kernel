/* X683 hardware integration surface: kallsyms + boot-image DT evidence. */
#include <stdint.h>
struct x683_sym { const char *name; uint64_t address; };
struct x683_dt { const char *path; const char *compatible; uint64_t base; };

static const struct x683_sym x683_storage[] = {
 {"msdc_drv_probe",0xffffff92d1564ba0ULL},
 {"msdc_ops_request",0xffffff92d1566448ULL},
 {"msdc_do_request",0xffffff92d15631dcULL},
 {"msdc_irq",0xffffff92d1565aa0ULL},
 {"msdc_execute_tuning",0xffffff92d1564068ULL},
 {"msdc_cqhci_reset",0xffffff92d156705cULL},
 {"msdc_cqhci_crypto_cfg",0xffffff92d15670b8ULL},
 {"msdc_suspend",0xffffff92d1567664ULL},
 {"msdc_resume",0xffffff92d15676a4ULL},
 {"msdc_runtime_suspend",0xffffff92d1567718ULL},
 {"msdc_runtime_resume",0xffffff92d156778cULL},
};

static const struct x683_sym x683_power[] = {
 {"mt6358_gauge_probe",0xffffff92d0fbb448ULL},
 {"ppm_limit_callback",0xffffff92d0f814f0ULL},
 {"ppm_main_pdrv_probe",0xffffff92d0fa0e10ULL},
 {"ppm_main_suspend",0xffffff92d0fa0cdcULL},
 {"ppm_main_resume",0xffffff92d0fa0d78ULL},
 {"tscpu_thermal_probe",0xffffff92d1345ef4ULL},
 {"mtk_thermal_zone_device_register_wrapper",0xffffff92d133d930ULL},
 {"mtk_thermal_cooling_device_register_wrapper",0xffffff92d133df80ULL},
};

static const struct x683_sym x683_media_input[] = {
 {"mtk_get_mali_dev",0xffffff92d10c4324ULL},
 {"mtkfb_init",0xffffff92d1d4ae1cULL},
 {"disp_probe",0xffffff92d128ddc8ULL},
 {"disp_lcm_probe",0xffffff92d12e9224ULL},
 {"ilitek_spi_probe",0xffffff92d145a098ULL},
 {"tpd_probe",0xffffff92d144c1e4ULL},
 {"mtk_btif_probe",0xffffff92d10717e0ULL},
};

static const struct x683_dt x683_dt_high_value[] = {
 {"/msdc@11230000","mediatek,msdc",0x11230000ULL},
 {"/msdc@11240000","mediatek,msdc",0x11240000ULL},
 {"/m4u@10205000","mediatek,m4u",0x10205000ULL},
 {"/pwrap@1000d000/mt6358-pmic","mediatek,mt6358-pmic",0x1000d000ULL},
 {"/mt6358_gauge","mediatek,mt6358_gauge",0},
 {"/btif@1100c000","mediatek,btif",0x1100c000ULL},
 {"/audio@11220000","mediatek,audio,syscon",0x11220000ULL},
 {"/mali@13040000","mediatek,mali,arm,mali-midgard,arm,mali-bifrost",0x13040000ULL},
 {"/gpufreq","mediatek,mt6768-gpufreq",0},
 {"/dsi0@14014000","mediatek,dsi0",0x14014000ULL},
 {"/wifi@18000000","mediatek,wifi",0x18000000ULL},
 {"/btscharger","tran,ts_btscharger",0},
 {"/mt_charger","mediatek,mt-charger",0},
 {"/charger","mediatek,charger",0},
 {"/touch","mediatek,touch",0},
};

/* X683 proven path: DT -> probe surface -> IRQ/work/PM -> device state.
 * Exact ops-table members remain unresolved and are intentionally omitted. */
