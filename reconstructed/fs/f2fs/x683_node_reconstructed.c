/* X683 F2FS node/NAT reconstruction. */
#include <stdint.h>
#include <stdbool.h>
struct x683_sbi { unsigned char opaque[0]; };
struct x683_page { unsigned char opaque[0]; };
struct x683_dnode { unsigned char opaque[0]; };
extern struct x683_page *f2fs_get_node_page(struct x683_sbi *, uint32_t nid);
extern struct x683_page *f2fs_new_node_page(struct x683_dnode *, uint32_t nid);
extern int f2fs_fsync_node_pages(struct x683_sbi *, uint32_t ino, int mode);
extern int f2fs_sync_node_pages(struct x683_sbi *, int mode);
extern int f2fs_flush_nat_entries(struct x683_sbi *, void *cpc);

struct x683_node_path { uint32_t nid; uint32_t ofs_in_node; struct x683_page *node_page; };

int x683_node_lookup(struct x683_sbi *sbi, uint32_t nid, struct x683_node_path *out)
{
    struct x683_page *page = f2fs_get_node_page(sbi, nid);
    if (!page) return -2;
    out->nid = nid; out->node_page = page; out->ofs_in_node = 0;
    return 0;
}

int x683_node_alloc(struct x683_sbi *sbi, struct x683_dnode *dn, uint32_t nid)
{
    struct x683_page *page = f2fs_new_node_page(dn, nid);
    (void)sbi;
    return page ? 0 : -12;
}

int x683_node_sync(struct x683_sbi *sbi, uint32_t ino)
{
    int ret = f2fs_fsync_node_pages(sbi, ino, 0);
    return ret ? ret : f2fs_sync_node_pages(sbi, 0);
}

int x683_node_checkpoint_flush(struct x683_sbi *sbi, void *cpc)
{ return f2fs_flush_nat_entries(sbi, cpc); }

int x683_f2fs_build_node_manager(struct x683_sbi *sbi) { (void)sbi; return 0; }
void x683_f2fs_destroy_node_manager(struct x683_sbi *sbi) { (void)sbi; }
