/* X683 MT6768 power/DVFS integration surface. */
#include <stdint.h>
extern int _mt_cpufreq_pdrv_init(void);
extern int mtk_cpuidle_init(void);
extern int ppm_platform_init(void);
extern int eem_init(void);
extern int pbm_module_init(void);
extern int spm_fs_init(void);
extern int mtk_thermal_platform_init(void);

int x683_power_stack_init(void)
{
    int ret = 0;
    ret |= _mt_cpufreq_pdrv_init();
    ret |= mtk_cpuidle_init();
    ret |= ppm_platform_init();
    ret |= eem_init();
    ret |= pbm_module_init();
    ret |= spm_fs_init();
    ret |= mtk_thermal_platform_init();
    return ret;
}
