/* X683 ARM64 runtime architecture map backed by kallsyms. */
#include <stdint.h>
extern void do_undefinstr(void);
extern void do_mem_abort(void);
extern void do_debug_exception(void);
extern void gic_handle_irq(void);
extern void __do_softirq(void);
extern void vectors(void);
extern void cpu_do_suspend(void);
extern void cpu_suspend(void);
extern void arm_cpuidle_init(void);
extern void arm_cpuidle_suspend(void);

void x683_architecture_surface(void)
{
    (void)do_undefinstr; (void)do_mem_abort; (void)do_debug_exception;
    (void)gic_handle_irq; (void)__do_softirq; (void)vectors;
    (void)cpu_do_suspend; (void)cpu_suspend;
    (void)arm_cpuidle_init; (void)arm_cpuidle_suspend;
}
