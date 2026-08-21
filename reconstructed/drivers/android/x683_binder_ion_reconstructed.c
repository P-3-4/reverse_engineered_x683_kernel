/* X683 Android kernel memory/IPC integration: Binder + ION. */
#include <stdint.h>
#include <stddef.h>
extern int binder_transaction(void *proc, void *thread, void *t, int reply, int flags, int code, uint32_t size);
extern int binder_alloc_prepare_to_free(void *alloc, void *buffer);
extern int binder_alloc_new_buf(void *alloc, void *buf, size_t size, size_t off_start, size_t off_end, int pid);
extern void binder_alloc_free_buf(void *alloc, void *buffer);
extern int binder_alloc_shrinker_init(void);
extern int ion_alloc(void *client, size_t len, size_t align, unsigned int heap_mask, unsigned int flags);
extern void ion_free(void *client, void *handle);
extern long ion_ioctl(void *file, unsigned int cmd, unsigned long arg);
extern int ion_heap_init_shrinker(void *heap);

int x683_binder_transaction(void *proc, void *thread, void *t, int reply, int flags, int code, uint32_t size)
{ return binder_transaction(proc, thread, t, reply, flags, code, size); }
int x683_binder_alloc_prepare_free(void *alloc, void *buffer)
{ return binder_alloc_prepare_to_free(alloc, buffer); }
int x683_binder_alloc_new(void *alloc, void *buf, size_t size, size_t start, size_t end, int pid)
{ return binder_alloc_new_buf(alloc, buf, size, start, end, pid); }
void x683_binder_alloc_free(void *alloc, void *buffer) { binder_alloc_free_buf(alloc, buffer); }
int x683_binder_shrinker_init(void) { return binder_alloc_shrinker_init(); }
int x683_ion_alloc(void *client, size_t len, size_t align, unsigned int heap_mask, unsigned int flags)
{ return ion_alloc(client, len, align, heap_mask, flags); }
void x683_ion_free(void *client, void *handle) { ion_free(client, handle); }
long x683_ion_ioctl(void *file, unsigned int cmd, unsigned long arg)
{ return ion_ioctl(file, cmd, arg); }
int x683_ion_heap_shrinker_init(void *heap) { return ion_heap_init_shrinker(heap); }
