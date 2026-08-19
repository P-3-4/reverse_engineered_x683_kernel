/* X683 framebuffer -> performance/power/vendor policy bridge. */
#include <stdint.h>
extern int fb_register_client(void *nb);
extern int fb_unregister_client(void *nb);
extern int fb_notifier_call_chain(unsigned long val, void *v);
extern int fb_event(void *event);
extern int ppm_lcmoff_fb_notifier_callback(void *nb, unsigned long event, void *data);
extern int ged_fb_notifier_callback(void *nb, unsigned long event, void *data);
extern int cm_mgr_fb_notifier_callback(void *nb, unsigned long event, void *data);
extern int ged_dvfs_trigger_fb_dvfs(void);
extern int tran_disp_lcm_shutdown(void *ctx);

int x683_register_display_notifier(void *nb) { return fb_register_client(nb); }
int x683_unregister_display_notifier(void *nb) { return fb_unregister_client(nb); }
int x683_display_event(void *event) { return fb_event(event); }
int x683_display_notify(unsigned long event, void *data)
{
    int ret = fb_notifier_call_chain(event, data);
    (void)ppm_lcmoff_fb_notifier_callback(0, event, data);
    (void)ged_fb_notifier_callback(0, event, data);
    (void)cm_mgr_fb_notifier_callback(0, event, data);
    (void)ged_dvfs_trigger_fb_dvfs();
    return ret;
}
int x683_display_shutdown(void *ctx) { return tran_disp_lcm_shutdown(ctx); }
