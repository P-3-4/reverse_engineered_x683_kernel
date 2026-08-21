/* X683 F2FS recovery surface. */
#include <stdint.h>
#include <stdbool.h>
struct x683_sbi { unsigned char opaque[0]; };
extern int f2fs_recover_fsync_data(struct x683_sbi *, bool check_only);
extern int f2fs_recover_inode_page(struct x683_sbi *, void *page);
extern int f2fs_restore_node_summary(struct x683_sbi *, void *sum, uint32_t segno);
extern int f2fs_space_for_roll_forward(struct x683_sbi *);

/* f2fs_recover_fsync_data @ 0xffffff92d0df0d08. */
int x683_recover_fsync_data(struct x683_sbi *sbi, bool check_only)
{ return f2fs_recover_fsync_data(sbi, check_only); }
int x683_recover_inode_page(struct x683_sbi *sbi, void *page)
{ return f2fs_recover_inode_page(sbi, page); }
int x683_restore_node_summary(struct x683_sbi *sbi, void *sum, uint32_t segno)
{ return f2fs_restore_node_summary(sbi, sum, segno); }
int x683_space_for_roll_forward(struct x683_sbi *sbi)
{ return f2fs_space_for_roll_forward(sbi); }
