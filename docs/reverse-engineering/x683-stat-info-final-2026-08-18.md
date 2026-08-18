# X683/H694 `f2fs_stat_info` — final binary reconstruction

Authority: uploaded stock `boot(8).img` and standalone compressed kernel, verified against Image SHA-256 `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`.

## 1. Object identity and size

`f2fs_sb_info + 0x568` is a pointer to the X683 statistics object.

The allocator at `0x375c40` requests exactly `0x238` bytes for this object. The initialized list head and `sbi` back-pointer establish:

```text
+0x000  list_head.next
+0x008  list_head.prev
+0x010  sbi back-pointer
```

The X683 object therefore occupies exactly `0x238` bytes (`+0x000..+0x237`). The layout through `+0x230` is now resolved; there is no unaccounted tail after `+0x230`.

## 2. Resolved `f2fs_stat_info` layout

```text
+0x000  struct list_head stat_list
+0x010  struct f2fs_sb_info *sbi
+0x018  all_area_segs
+0x01c  sit_area_segs
+0x020  nat_area_segs
+0x024  ssa_area_segs
+0x028  main_area_segs
+0x02c  main_area_sections
+0x030  main_area_zones
+0x038  hit_largest
+0x040  hit_cached
+0x048  hit_rbtree
+0x050  hit_total
+0x058  total_ext
+0x060  ext_tree
+0x064  zombie_tree
+0x068  ext_node
+0x06c  ndirty_node
+0x070  ndirty_dent
+0x074  ndirty_meta
+0x078  ndirty_imeta
+0x07c  ndirty_data
+0x080  ndirty_qdata
+0x084  inmem_pages
+0x088  ndirty_dirs
+0x08c  ndirty_files
+0x090  nquota_files
+0x094  ndirty_all
+0x098  nats
+0x09c  dirty_nats
+0x0a0  sits
+0x0a4  dirty_sits
+0x0a8  free_nids
+0x0ac  avail_nids
+0x0b0  alloc_nids
+0x0b4  total_count
+0x0b8  utilization
+0x0bc  bg_gc
+0x0c0  nr_wb_cp_data
+0x0c4  nr_wb_data
+0x0c8  nr_rd_data
+0x0cc  nr_rd_node
+0x0d0  nr_rd_meta
+0x0d4  nr_dio_read
+0x0d8  nr_dio_write
+0x0dc  io_skip_bggc
+0x0e0  other_skip_bggc
+0x0e4  nr_flushing
+0x0e8  nr_flushed
+0x0ec  flush_list_empty
+0x0f0  nr_discarding
+0x0f4  nr_discarded
+0x0f8  nr_discard_cmd
+0x0fc  undiscard_blks
+0x100  inline_xattr
+0x104  inline_inode
+0x108  inline_dir
+0x10c  append
+0x110  update
+0x114  orphans
+0x118  compr_inode
+0x120  compr_blocks
+0x128  valid_count
+0x12c  valid_node_count
+0x130  valid_inode_count
+0x134  discard_blks
+0x138  bimodal / BDF statistic
+0x13c  avg_vblocks
+0x140  util_free / distribution statistic
+0x144  util_valid / distribution statistic
+0x148  util_invalid / distribution statistic
+0x14c  rsvd_segs
+0x150  overp_segs
+0x154  dirty_count
+0x158  node_pages
+0x15c  meta_pages
+0x160  prefree_count
+0x164  call_count
+0x168  cp_count
+0x16c  bg_cp_count
+0x170  tot_segs
+0x174  node_segs
+0x178  data_segs
+0x17c  free_segs
+0x180  free_secs
+0x184  bg_node_segs
+0x188  bg_data_segs
+0x18c  tot_blks
+0x190  data_blks
+0x194  node_blks
+0x198  bg_data_blks
+0x19c  bg_node_blks
+0x1a0  skipped_atomic_files[0]
+0x1a8  skipped_atomic_files[1]
+
+/* X683 tail */
+
+0x1b0  curseg[0]
+0x1b4  curseg[1]
+0x1b8  curseg[2]
+0x1bc  curseg[3]
+0x1c0  curseg[4]
+0x1c4  curseg[5]
+
+0x1c8  cursec[0]
+0x1cc  cursec[1]
+0x1d0  cursec[2]
+0x1d4  cursec[3]
+0x1d8  cursec[4]
+0x1dc  cursec[5]
+
+0x1e0  curzone[0]
+0x1e4  curzone[1]
+0x1e8  curzone[2]
+0x1ec  curzone[3]
+0x1f0  curzone[4]
+0x1f4  curzone[5]
+
+0x1f8  X683 SBI-derived statistic #0 (source sbi+0x570)
+0x1fc  X683 SBI-derived statistic #1 (source sbi+0x574)
+0x200  X683 SBI-derived statistic #2 (source sbi+0x578)
+0x204  X683 SBI-derived statistic #3 (source sbi+0x57c)
+0x208  X683 SBI-derived statistic #4 (source sbi+0x580)
+0x20c  X683 SBI-derived statistic #5 (source sbi+0x584)
+0x210  X683 SBI-derived statistic #6 (source sbi+0x588)
+0x214  X683 SBI-derived statistic #7 (source sbi+0x58c)
+0x218  X683 SBI-derived statistic #8 (source sbi+0x590)
+
+0x220  base/total memory-accounting size in bytes
+0x228  static memory-accounting size in bytes
+0x230  cached memory-accounting size in bytes
+```

The first three tail arrays are binary-proven from the current-segment table at `sm_info + 0x18` and its `curseg_info + 0x5c` values. `cursec[]` and `curzone[]` are the corresponding normalized segment/section/zone values after division by `segs_per_sec` and the X683 per-zone divisor.

The `+0x1f8..+0x218` fields are explicitly copied from SBI offsets `+0x570..+0x590`. Their **source offset mapping is proven**, while their original X683 member names remain unresolved.

The memory fields are not speculative: the formatter prints all three in a dedicated memory summary, and the builder repeatedly updates them before printing.

## 3. Exact proof of the controversial middle fields

The stock debug formatter proves the classic semantics:

```text
+0x14c +0x150 -> "OverProv:%d Resv:%d"
+0x154        -> dirty-count statistic
+0x158        -> node-page statistic
+0x15c        -> second argument of "meta: %4d in %4d"
+0x160        -> "Prefree"
+0x164        -> "GC calls"
+0x168,+0x16c -> "CP calls"
```

Thus the previous uncertainty over `+0x15c` is resolved:

```text
stat +0x15c = meta_pages
```

There is no evidence that `+0x15c` is an I/O-policy field; it belongs to the ordinary statistics structure.

## 4. GC accounting

At `0x35278c..0x352798`, the X683 GC path performs:

```c
stat->call_count++;
```

Therefore:

```text
stat +0x164 = call_count
```

The terminal policy path at `0x366e7c..0x366f28` increments:

```c
stat->bg_cp_count++;
```

Therefore:

```text
stat +0x16c = bg_cp_count
```

This supersedes all previous claims that `stat +0x16c` was `dirty_count` or an unnamed vendor completion counter.

## 5. Segment and block accounting

The GC completion paths directly establish:

```c
stat->tot_segs++;
stat->node_segs++;
stat->data_segs++;
stat->bg_node_segs += background;
stat->bg_data_segs += background;
```

and:

```c
stat->tot_blks += blocks;
stat->data_blks += blocks;
stat->bg_data_blks += background ? blocks : 0;
stat->node_blks += blocks;
stat->bg_node_blks += background ? blocks : 0;
```

So the offsets are:

```text
+0x170 tot_segs
+0x174 node_segs
+0x178 data_segs
+0x184 bg_node_segs
+0x188 bg_data_segs
+0x18c tot_blks
+0x190 data_blks
+0x194 node_blks
+0x198 bg_data_blks
+0x19c bg_node_blks
```

The debug strings independently print these exact fields as data/node segments and blocks, making the semantic mapping binary-backed.

## 6. Seven SBI policy fields resolved through `stat_info`

The statistics-copy routine at `0x375ed8..0x375f0c` performs:

```text
sbi +0x45c -> stat +0x0d4 = nr_dio_read
sbi +0x458 -> stat +0x0d8 = nr_dio_write
sbi +0x444 -> stat +0x0c0 = nr_wb_cp_data
sbi +0x448 -> stat +0x0c4 = nr_wb_data
sbi +0x44c -> stat +0x0c8 = nr_rd_data
sbi +0x450 -> stat +0x0cc = nr_rd_node
sbi +0x454 -> stat +0x0d0 = nr_rd_meta
```

Therefore the seven fields are exactly the SBI-side storage for these seven I/O statistics, mirrored into `f2fs_stat_info`:

```text
+0x444  nr_wb_cp_data
+0x448  nr_wb_data
+0x44c  nr_rd_data
+0x450  nr_rd_node
+0x454  nr_rd_meta
+0x458  nr_dio_write
+0x45c  nr_dio_read
```

This resolves their functional meaning. The seven-field `0x366cd4` branch is therefore an **I/O-activity discriminator** rather than seven arbitrary GC state flags.

## 7. I/O-policy interpretation

For `gc_mode != 3`:

```text
any of:
    writeback-with-CP
    normal writeback
    data reads
    node reads
    metadata reads
    direct-I/O writes
    direct-I/O reads

nonzero -> active guarded policy path 0x366da4

all seven zero -> clean/alternate policy path 0x366ee0
```

`gc_mode == 3` bypasses that discriminator and enters the shared stage directly at `0x366de4`.

This is now a substantially stronger interpretation than simply calling these offsets “vendor guard words”.

## 8. Background accounting

At X683 `f2fs_gc()` entry:

```text
w0 = sbi
w1 = sync
w2 = background
w3 = requested_segno
```

The binary converts `w2` to a per-call boolean consumed by the `bg_*` statistics. The segment and block accounting therefore follows the ordinary F2FS semantic meaning of background GC.

## 9. Object-size sanity check

The allocation request is `0x238` bytes.

The final field is at `+0x230` and is 8 bytes wide, ending at `+0x237`.

Thus:

```text
last used byte = +0x237
object size    = 0x238
```

There is no unexplained trailing padding or hidden field required by the current access set.

## 10. Final status

### High confidence / byte-backed

```text
sbi+0x568              = stat_info pointer
stat object size       = 0x238
stat+0x14c             = rsvd_segs
stat+0x150             = overp_segs
stat+0x154             = dirty_count
stat+0x158             = node_pages
stat+0x15c             = meta_pages
stat+0x160             = prefree_count
stat+0x164             = call_count
stat+0x168             = cp_count
stat+0x16c             = bg_cp_count
stat+0x170..+0x19c     = segment/block GC accounting
stat+0x1a0/+0x1a8     = skipped_atomic_files[2]
stat+0x1b0..+0x1f4    = curseg/cursec/curzone arrays
stat+0x1f8..+0x218    = nine SBI-derived X683 statistics
stat+0x220             = total memory-accounting size
stat+0x228             = static memory-accounting size
stat+0x230             = cached memory-accounting size
```

### Remaining naming gap

Only the original X683 source-level names of the nine SBI-derived fields at `+0x1f8..+0x218` remain unproved. Their exact source offsets are fixed and their statistics-object positions are complete.
