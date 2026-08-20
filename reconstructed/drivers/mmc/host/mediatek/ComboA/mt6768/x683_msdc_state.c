/*
 * X683 / MT6768 MSDC evidence-backed reconstruction unit.
 *
 * This is intentionally not a compile-complete replacement for MediaTek's
 * private driver. Unknown members remain byte-offset backed. The symbols and
 * DT facts below are taken from the X683 Image/kallsyms/DTB.
 */
#include <stdint.h>

#define X683_MSDC0_BASE 0x11230000ULL
#define X683_MSDC1_BASE 0x11240000ULL
#define X683_MSDC0_REG_SIZE 0x10000ULL
#define X683_MSDC1_REG_SIZE 0x10000ULL

struct x683_msdc_opaque_host {
    uint8_t bytes[0x560];
};

struct x683_msdc_dt_binding {
    const char *compatible;
    uint64_t base;
    uint64_t size;
    const char *clock0;
    const char *clock1;
    const char *clock2;
};

static const struct x683_msdc_dt_binding x683_msdc_dt[] __attribute__((used)) = {
    { "mediatek,msdc", X683_MSDC0_BASE, X683_MSDC0_REG_SIZE,
      "msdc0-clock", "msdc0-hclock", "msdc0-aes-clock" },
    { "mediatek,msdc", X683_MSDC1_BASE, X683_MSDC1_REG_SIZE,
      "msdc1-clock", "msdc1-hclock", 0 },
};

/* X683 kallsyms: exact entry points. */
extern int msdc_drv_probe(void *pdev);              /* 0xffffff92d1564ba0 */
extern int msdc_ops_request(void *host, void *mrq); /* 0xffffff92d1566448 */
extern void msdc_do_request(void *host, void *mrq);  /* 0xffffff92d15631dc */
extern int msdc_irq(int irq, void *dev_id);          /* 0xffffff92d1565aa0 */
extern int msdc_execute_tuning(void *host, int opcode); /* 0xffffff92d1564068 */
extern int msdc_suspend(void *dev);                  /* 0xffffff92d1567664 */
extern int msdc_resume(void *dev);                   /* 0xffffff92d15676a4 */
extern int msdc_runtime_suspend(void *dev);          /* 0xffffff92d1567718 */
extern int msdc_runtime_resume(void *dev);           /* 0xffffff92d156778c */
extern int msdc_cqhci_reset(void *host);             /* 0xffffff92d156705c */
extern int msdc_cqhci_crypto_cfg(void *host, void *cfg); /* 0xffffff92d15670b8 */

/*
 * Proven storage path:
 *
 *   mmc core -> msdc_ops_request -> msdc_do_request -> DMA/command issue
 *             -> msdc_irq -> completion/error handling
 *
 * The request function also contains an indirect callback loop (BLR) over a
 * linked callback container. That target is deliberately unresolved here;
 * naming an ops member would exceed the binary evidence.
 *
 * X683 config proves CONFIG_MMC_MTK_PRO=y, CONFIG_MTK_EMMC_SUPPORT=y,
 * CONFIG_MTK_EMMC_CACHE=y and CONFIG_MTK_EMMC_HW_CQ=y. CQ support is not the
 * generic CONFIG_MTK_EMMC_CQ_SUPPORT path. The DT proves two enabled MSDC
 * nodes, with AES clock only on MSDC0.
 */
struct x683_msdc_reconstruction_notes {
    uint32_t config_mmc_mtk_pro;
    uint32_t config_emmc_support;
    uint32_t config_emmc_cache;
    uint32_t config_emmc_hw_cq;
    uint32_t indirect_callback_sites;
};

static const struct x683_msdc_reconstruction_notes x683_msdc_notes __attribute__((used)) = {
    1, 1, 1, 1, 1,
};
