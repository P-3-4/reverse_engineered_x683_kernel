/* X683 scheduler -> PPM -> DVFS/thermal evidence-backed reconstruction. */
#include <stdint.h>

struct x683_sched_policy_symbol { const char *name; uint64_t address; };
static const struct x683_sched_policy_symbol x683_sched_policy[] __attribute__((used)) = {
    { "schedtune_enqueue_task", 0xffffff92d0b26944ULL },
    { "schedtune_dequeue_task", 0xffffff92d0b26ca4ULL },
    { "schedtune_cpu_boost", 0xffffff92d0b26e20ULL },
    { "schedtune_task_boost", 0xffffff92d0b26f78ULL },
    { "schedtune_prefer_idle", 0xffffff92d0b26fc4ULL },
};

static const struct x683_sched_policy_symbol x683_ppm[] __attribute__((used)) = {
    { "ppm_limit_callback", 0xffffff92d0f814f0ULL },
    { "ppm_main_suspend", 0xffffff92d0fa0cdcULL },
    { "ppm_main_resume", 0xffffff92d0fa0d78ULL },
    { "ppm_main_pdrv_probe", 0xffffff92d0fa0e10ULL },
    { "mt_ppm_main", 0xffffff92d0fa14d4ULL },
    { "mt_ppm_set_dvfs_table", 0xffffff92d0fa3808ULL },
    { "mt_ppm_register_client", 0xffffff92d0fa39c4ULL },
};

static const struct x683_sched_policy_symbol x683_thermal[] __attribute__((used)) = {
    { "tscpu_thermal_probe", 0xffffff92d1345ef4ULL },
    { "mtk_thermal_zone_device_register_wrapper", 0xffffff92d133d930ULL },
    { "thermal_zone_device_update", 0xffffff92d151188cULL },
    { "thermal_cooling_device_register", 0xffffff92d151242cULL },
};

/* X683 config proves SCHED_TUNE, HMP/HMP+, EAS, MTK sched boost, CPUFREQ,
 * MTK CPUFREQ, CPU idle, EEM, PBM and EARA thermal support. */
struct x683_policy_chain {
    const char *scheduler_to_ppm;
    const char *thermal_to_ppm;
    const char *frequency_governor;
};

static const struct x683_policy_chain x683_policy_chain __attribute__((used)) = {
    "schedtune / scheduler callbacks -> PPM limit/client path",
    "thermal callbacks -> PPM policy limit path",
    "schedutil (CONFIG_CPU_FREQ_DEFAULT_GOV_SCHEDUTIL)",
};
