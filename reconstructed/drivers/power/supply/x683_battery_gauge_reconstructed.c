/* X683 MT6358 fuel-gauge / charger / power-supply integration. */
#include <stdint.h>
extern int mt6358_gauge_probe(void *dev);
extern int mt6358_gauge_remove(void *dev);
extern void mt6358_gauge_shutdown(void *dev);
extern int do_charger_detect(void *ctx);
extern int hw_charging_get_charger_type(void);
extern int battery_update(void *data);
extern int battery_debug_init(void);
extern int evb_battery_init(void);
extern int power_supply_register(void *dev, void *psy);
extern void power_supply_changed(void *psy);
extern int power_supply_get_property(void *, int, void *);
extern int register_battery_notifier(void *nb);
extern int battery_notifier(void *event);
extern int usb_phy_notify_charger_work(void *work);
extern int usb_phy_get_charger_type(void);
extern int tran_get_charger_type(void);
extern int tran_battery_probe(void *dev);

int x683_gauge_probe(void *dev) { return mt6358_gauge_probe(dev); }
int x683_gauge_remove(void *dev) { return mt6358_gauge_remove(dev); }
void x683_gauge_shutdown(void *dev) { mt6358_gauge_shutdown(dev); }
int x683_charger_detect(void *ctx) { return do_charger_detect(ctx); }
int x683_charger_type(void)
{
    (void)usb_phy_get_charger_type();
    (void)hw_charging_get_charger_type();
    return tran_get_charger_type();
}
int x683_battery_update(void *data) { return battery_update(data); }
int x683_battery_debug_init(void) { return battery_debug_init(); }
int x683_battery_init(void) { return evb_battery_init(); }
int x683_power_supply_register(void *dev, void *psy) { return power_supply_register(dev, psy); }
void x683_power_supply_changed(void *psy) { power_supply_changed(psy); }
int x683_battery_property(void *psy, int prop, void *value)
{ return power_supply_get_property(psy, prop, value); }
int x683_register_battery_notifier(void *nb) { return register_battery_notifier(nb); }
int x683_battery_event(void *event) { return battery_notifier(event); }
int x683_tran_battery_probe(void *dev) { return tran_battery_probe(dev); }
int x683_usb_charger_event(void *work) { return usb_phy_notify_charger_work(work); }
