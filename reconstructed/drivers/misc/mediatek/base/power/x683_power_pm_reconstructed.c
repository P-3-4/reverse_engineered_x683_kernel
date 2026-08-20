/* X683 power-management integration evidence model. */
#include <stdint.h>

struct x683_pm_symbol { const char *name; uint64_t address; const char *role; };
static const struct x683_pm_symbol x683_pm[] __attribute__((used)) = {
    { "ppm_main_suspend", 0xffffff92d0fa0cdcULL, "PPM suspend callback" },
    { "ppm_main_resume", 0xffffff92d0fa0d78ULL, "PPM resume callback" },
    { "msdc_suspend", 0xffffff92d1567664ULL, "MSDC system suspend" },
    { "msdc_resume", 0xffffff92d15676a4ULL, "MSDC system resume" },
    { "msdc_runtime_suspend", 0xffffff92d1567718ULL, "MSDC runtime suspend" },
    { "msdc_runtime_resume", 0xffffff92d156778cULL, "MSDC runtime resume" },
    { "kbase_pm_suspend", 0xffffff92d10b7860ULL, "Mali suspend" },
    { "kbase_pm_resume", 0xffffff92d10b7928ULL, "Mali resume" },
    { "kbase_pm_do_poweron", 0xffffff92d10dc0f4ULL, "GPU power-on transition" },
    { "kbase_pm_do_poweroff", 0xffffff92d10dc158ULL, "GPU power-off transition" },
};

/* The binary contains both direct calls and indirect notifier/callback
 * dispatches. This unit records the proven entry points; callback container
 * layouts remain offset-backed until their indirect targets are proven. */
