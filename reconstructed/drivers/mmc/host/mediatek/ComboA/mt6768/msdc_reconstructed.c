/* X683 MT6768 eMMC/MSDC host reconstruction. */
#include <stdint.h>
#include <stdbool.h>
struct x683_mmc_host { unsigned char opaque[0]; };
struct x683_mmc_request { unsigned char opaque[0]; };
extern int msdc_drv_probe(void *pdev);
extern int msdc_drv_remove(void *pdev);
extern int msdc_ops_request(struct x683_mmc_host *, struct x683_mmc_request *);
extern int msdc_ops_set_ios(struct x683_mmc_host *);
extern int msdc_execute_tuning(struct x683_mmc_host *, int opcode);
extern int msdc_irq(int irq, void *dev_id);
extern int msdc_suspend(void *dev);
extern int msdc_resume(void *dev);
extern int msdc_runtime_suspend(void *dev);
extern int msdc_runtime_resume(void *dev);
extern int msdc_cqhci_reset(void *host);
extern int msdc_cqhci_crypto_cfg(void *host, void *cfg);
extern void msdc_do_request(void *host, void *mrq);
extern int msdc_dma_setup(void *host, void *data);
extern int msdc_dma_start(void *host);
extern int msdc_dma_stop(void *host);

int x683_msdc_probe(void *pdev) { return msdc_drv_probe(pdev); }
int x683_msdc_remove(void *pdev) { return msdc_drv_remove(pdev); }
int x683_msdc_request(struct x683_mmc_host *host, struct x683_mmc_request *mrq)
{ return msdc_ops_request(host, mrq); }
int x683_msdc_set_ios(struct x683_mmc_host *host) { return msdc_ops_set_ios(host); }
int x683_msdc_tuning(struct x683_mmc_host *host, int opcode)
{ return msdc_execute_tuning(host, opcode); }
int x683_msdc_interrupt(int irq, void *dev_id) { return msdc_irq(irq, dev_id); }
void x683_msdc_request_path(void *host, void *mrq) { msdc_do_request(host, mrq); }
int x683_msdc_dma_prepare(void *host, void *data) { return msdc_dma_setup(host, data); }
int x683_msdc_dma_begin(void *host) { return msdc_dma_start(host); }
int x683_msdc_dma_end(void *host) { return msdc_dma_stop(host); }
int x683_msdc_suspend(void *dev) { return msdc_suspend(dev); }
int x683_msdc_resume(void *dev) { return msdc_resume(dev); }
int x683_msdc_runtime_suspend(void *dev) { return msdc_runtime_suspend(dev); }
int x683_msdc_runtime_resume(void *dev) { return msdc_runtime_resume(dev); }
int x683_msdc_cqhci_reset(void *host) { return msdc_cqhci_reset(host); }
int x683_msdc_cqhci_crypto(void *host, void *cfg) { return msdc_cqhci_crypto_cfg(host, cfg); }
