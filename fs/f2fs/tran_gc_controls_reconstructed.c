/*
 * X683/H694 Transsion GC control/descripor reconstruction.
 *
 * Binary-derived registry metadata from stock X683 tran_gc_init().
 * Not proprietary source recovery. Informational/non-build integration file.
 */
#include <linux/types.h>

struct x683_tran_gc_control_binding {
	const char *name;
	unsigned int registration_offset;
	unsigned int string_offset;
	unsigned int descriptor_offset;
	const char *backing_state;
	const char *status;
};

/* Image-relative vendor GC state recovered independently from proc handlers. */
#define X683_GC_TIMES_OFF              0x1a13990
#define X683_GC_CONTROLLER_OFF         0x1a13998
#define X683_SSR_GC_TIMES_OFF          0x1a139a0
#define X683_STATIC_PASS_TIMES_OFF     0x1a139b0
#define X683_GC_SKIP_TIMES_OFF         0x1a139b8
#define X683_NEED_SWITCH_SSR_OFF       0x1a139c0
#define X683_GC_TO_STATIC_OFF          0x1a139c8
#define X683_TRAN_URGENT_GC_OFF        0x1a139d0
#define X683_LAST_PHASE_OFF            0x1a139d4
#define X683_GC_WAKE_UP_TIMES_OFF      0x1a139d8
#define X683_THREAD_CREATE_TIMES_OFF   0x1a139e0
#define X683_THREAD_DESTROY_TIMES_OFF  0x1a139e8
#define X683_GC_SEGMENT_INFO0_OFF      0x1a139f0
#define X683_GC_SEGMENT_INFO1_OFF      0x1a139f4
#define X683_IS_FRAGMENTATION_OFF      0x1a13a00
#define X683_DETECT_WAKELOCK_OFF       0x1a13a05

/*
 * These three controls share the generic registration path:
 *   control name -> common runtime object -> 0x274ea0 -> 0x274dac
 *   -> per-control descriptor retained by the generic registry node.
 */
static const struct x683_tran_gc_control_binding x683_controls[] = {
	{
		.name = "need_switch_ssr",
		.registration_offset = 0x37af88,
		.string_offset = 0x10a6359,
		.descriptor_offset = 0x173b9d0,
		.backing_state = "global +0x1a139c0 (u8)",
		.status = "backing state bound; generic callback semantics unresolved",
	},
	{
		.name = "tran_urgent_gc",
		.registration_offset = 0x37b068,
		.string_offset = 0x10a63ac,
		.descriptor_offset = 0x173bbb0,
		.backing_state = "global +0x1a139d0 (u32)",
		.status = "backing state bound; generic callback semantics unresolved",
	},
	{
		.name = "detect_charger_type",
		.registration_offset = 0x37b184,
		.string_offset = 0x10a6414,
		.descriptor_offset = 0x173bf70,
		.backing_state = "not independently identified",
		.status = "registration + write handler proven; backing state unresolved",
	},
};

static const char *x683_tran_gc_control_architecture(void)
{
	return
		"generic registry node + per-control descriptor/private state; "
		"not one standalone implementation function per name";
}

/*
 * Do not treat controller value 2 as intrinsically "urgent".
 * The current stock wrapper proves:
 *   controller 1 -> sbi->gc_mode 2
 *   controller 2 -> sbi->gc_mode 3
 *
 * Therefore Stop-4/5 stores of controller 2 must be described as raw
 * controller-state transitions until the original symbolic label is proven.
 */
