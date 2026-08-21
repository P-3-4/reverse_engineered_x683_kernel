/* X683 asynchronous callback candidates, kallsyms-proven. */
#include <stdint.h>
struct x683_async { const char *name; uint64_t address; const char *class_name; };
static const struct x683_async x683_async_surface[] = {
 {"msdc_irq",0xffffff92d1565aa0ULL,"IRQ"},
 {"tscpu_workqueue_start_timer",0xffffff92d1345e34ULL,"timer/work"},
 {"tscpu_workqueue_cancel_timer",0xffffff92d1345d9cULL,"timer/work"},
 {"ppm_main_suspend",0xffffff92d0fa0cdcULL,"PM"},
 {"ppm_main_resume",0xffffff92d0fa0d78ULL,"PM"},
 {"msdc_runtime_suspend",0xffffff92d1567718ULL,"PM"},
 {"msdc_runtime_resume",0xffffff92d156778cULL,"PM"},
 {"mtk_btif_drv_suspend",0xffffff92d10715a4ULL,"PM"},
 {"mtk_btif_drv_resume",0xffffff92d1071618ULL,"PM"},
 {"ilitek_tddi_wq_esd_check",0xffffff92d1459930ULL,"work"},
 {"ilitek_tddi_wq_bat_check",0xffffff92d14599acULL,"work"},
 {"thermal_zone_device_update",0xffffff92d151188cULL,"thermal"},
};

/* Symbol names establish callback candidates, not queue/creator ownership.
 * Creator -> queue -> event edges require executable XREF/BLR evidence. */
