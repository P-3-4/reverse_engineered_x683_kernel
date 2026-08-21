/* X683 F2FS data allocation / block mapping reconstruction. */
#include <stdint.h>
#include <stdbool.h>
struct x683_sbi { unsigned char opaque[0]; };
struct x683_inode { unsigned char opaque[0]; };
struct x683_page { unsigned char opaque[0]; };
struct x683_io { unsigned char opaque[0]; };
extern int f2fs_get_block(struct x683_inode *, uint64_t ofs, void *map);
extern int f2fs_map_blocks(struct x683_inode *, void *map, uint32_t max, int create, int flag);
extern int f2fs_submit_page_bio(void *fio);
extern int f2fs_submit_page_write(void *fio, void *page, int type);
extern int f2fs_overwrite_io(struct x683_io *fio);
extern void f2fs_allocate_data_block(struct x683_sbi *, struct x683_page *, uint64_t old, uint64_t *newb, void *sum, int type, struct x683_io *, bool add_list);

/* f2fs_allocate_data_block @ 0xffffff92d0dea3c8. */
void x683_allocate_data_block(struct x683_sbi *sbi, struct x683_page *page,
                              uint64_t old_blkaddr, uint64_t *new_blkaddr,
                              void *sum, int type, struct x683_io *fio, bool add_list)
{ f2fs_allocate_data_block(sbi, page, old_blkaddr, new_blkaddr, sum, type, fio, add_list); }

int x683_get_block(struct x683_inode *inode, uint64_t ofs, void *map)
{ return f2fs_get_block(inode, ofs, map); }
int x683_map_blocks(struct x683_inode *inode, void *map, uint32_t max, int create, int flag)
{ return f2fs_map_blocks(inode, map, max, create, flag); }
int x683_write_io(struct x683_io *fio, struct x683_page *page, int type)
{ return f2fs_submit_page_write(fio, page, type); }
int x683_overwrite_io(struct x683_io *fio) { return f2fs_overwrite_io(fio); }
int x683_submit_bio(struct x683_io *fio) { return f2fs_submit_page_bio(fio); }
