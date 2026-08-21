/* X683 scheduler/schedtune/MediaTek PPM policy surface. */
#include <stdint.h>
extern int schedtune_enqueue_task(void *rq, void *task);
extern int schedtune_dequeue_task(void *rq, void *task);
extern int schedtune_task_boost(void *task);
extern int schedtune_cpu_boost(void *rq);
extern int schedtune_prefer_idle(void *task);
extern int ppm_main_register_policy(void *policy);
extern int ppm_main_unregister_policy(void *policy);
extern int ppm_game_mode_change_cb(int mode);
extern int ppm_main_suspend(void *dev);
extern int ppm_main_resume(void *dev);
extern int ppm_main_freq_to_idx(unsigned int freq);
extern int ppm_clear_policy_limit(void *policy);
extern int ppm_main_clear_client_req(void *client);
extern int ppm_profile_update_client_exec_time(void *client, uint64_t ns);
extern int ppm_pwrthro_low_bat_protect(int level);
extern int ppm_pwrthro_bat_per_protect(int level);
extern int ppm_thermal_status_change_cb(void *data);
extern int ppm_lcmoff_is_policy_activated(void);

int x683_sched_enqueue(void *rq, void *task) { return schedtune_enqueue_task(rq, task); }
int x683_sched_dequeue(void *rq, void *task) { return schedtune_dequeue_task(rq, task); }
int x683_sched_boost(void *task) { return schedtune_task_boost(task); }
int x683_sched_cpu_boost(void *rq) { return schedtune_cpu_boost(rq); }
int x683_sched_prefer_idle(void *task) { return schedtune_prefer_idle(task); }
int x683_ppm_policy_register(void *policy) { return ppm_main_register_policy(policy); }
int x683_ppm_policy_unregister(void *policy) { return ppm_main_unregister_policy(policy); }
int x683_ppm_game_mode(int mode) { return ppm_game_mode_change_cb(mode); }
int x683_ppm_suspend(void *dev) { return ppm_main_suspend(dev); }
int x683_ppm_resume(void *dev) { return ppm_main_resume(dev); }
int x683_ppm_freq_index(unsigned int freq) { return ppm_main_freq_to_idx(freq); }
int x683_ppm_clear_limit(void *policy) { return ppm_clear_policy_limit(policy); }
int x683_ppm_clear_client(void *client) { return ppm_main_clear_client_req(client); }
int x683_ppm_exec_time(void *client, uint64_t ns) { return ppm_profile_update_client_exec_time(client, ns); }
int x683_ppm_low_battery(int level) { return ppm_pwrthro_low_bat_protect(level); }
int x683_ppm_battery_percent(int level) { return ppm_pwrthro_bat_per_protect(level); }
int x683_ppm_thermal(void *data) { return ppm_thermal_status_change_cb(data); }
int x683_ppm_display_off(void) { return ppm_lcmoff_is_policy_activated(); }
