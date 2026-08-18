# X683/H694 `f2fs_stat_info` — final v2 layout

Binary authority: uploaded stock X683/H694 Image, SHA-256 `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`.

## Object

```text
sbi +0x568 -> f2fs_stat_info *
allocation size = 0x238 bytes
```

The GC/statistics object is an X683/Transsion variant of the historical 4.14 `f2fs_stat_info`. The historical structure is used only where the binary's ordering and direct producers agree.

## Resolved fields

```text
+0x000  list_head.next
+0x008  list_head.prev
+0x010  sbi back-pointer

+0x14c  rsvd_segs       (direct statistics snapshot)
+0x150  overp_segs      (direct statistics snapshot)
+0x154  dirty_count     (sum of six dirty-segment type counters)
+0x158  node_pages      (snapshot from node-manager state)
+0x15c  meta_pages      (snapshot from metadata-manager state)
+0x160  prefree_count
+0x164  call_count
+0x168  cp_count
+0x16c  bg_cp_count
+
+0x170  tot_segs
+0x174  node_segs
+0x178  data_segs
+0x17c  free_segs
+0x180  free_secs
+0x184  bg_node_segs
+0x188  bg_data_segs
+
+0x18c  tot_blks
+0x190  data_blks
+0x194  node_blks
+0x198  bg_data_blks
+0x19c  bg_node_blks
+
+0x1a0  skipped_atomic_files[0]
+0x1a8  skipped_atomic_files[1]
+
+0x1b0  curseg[0] = CURSEG_HOT_DATA
+0x1b4  curseg[1] = CURSEG_WARM_DATA
+0x1b8  curseg[2] = CURSEG_COLD_DATA
+0x1bc  curseg[3] = CURSEG_HOT_NODE
+0x1c0  curseg[4] = CURSEG_WARM_NODE
+0x1c4  curseg[5] = CURSEG_COLD_NODE
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
+0x1f8  sbi+0x570 = meta_count[0]
+0x1fc  sbi+0x574 = meta_count[1]
+0x200  sbi+0x578 = meta_count[2]
+0x204  sbi+0x57c = meta_count[3]
+0x208  sbi+0x580 = segment_count[0]
+0x20c  sbi+0x584 = segment_count[1]
+0x210  sbi+0x588 = block_count[0]
+0x214  sbi+0x58c = block_count[1]
+0x218  sbi+0x590 = inplace_count
+```

The curseg/cursec/curzone arrays and the SBI snapshot fields are directly copied by the statistics formatter at `0x375ed8..0x37631c`. The six-element array order is the standard X683 F2FS six-current-segment order.

## Exact GC accounting

At GC completion:

```c
stat->call_count++;
```

At the vendor terminal path:

```c
stat->bg_cp_count++;
```

The segment accounting is:

```c
stat->tot_segs++;
stat->node_segs++;
stat->data_segs++;
stat->bg_node_segs += background;
stat->bg_data_segs += background;
```

The block accounting is:

```c
stat->tot_blks++;
stat->data_blks++;
stat->node_blks++;
stat->bg_data_blks += background;
stat->bg_node_blks += background;
```

This matches the historical F2FS statistics API and debug output, including `call_count`, `cp_count`, `bg_cp_count`, total/data/node segments and block counters. citeturn906146search0turn906146search9

## Seven SBI I/O counters mirrored into stat_info

The stock statistics-copy routine maps:

```text
sbi+0x444 -> stat+0x0c0 nr_wb_cp_data
sbi+0x448 -> stat+0x0c4 nr_wb_data
sbi+0x44c -> stat+0x0c8 nr_rd_data
sbi+0x450 -> stat+0x0cc nr_rd_node
sbi+0x454 -> stat+0x0d0 nr_rd_meta
sbi+0x458 -> stat+0x0d8 nr_dio_write
sbi+0x45c -> stat+0x0d4 nr_dio_read
```

These correspond to the historical F2FS I/O-stat fields used for writeback/read/direct-I/O accounting.

## Remaining tail

The X683 allocator size is `0x238`, and no direct statistics-copy stores occur at `+0x220/+0x228/+0x230` in the primary formatter. Historical F2FS variants place three 64-bit memory-accounting fields (`base_mem`, `cache_mem`, `page_mem`) at the end of the structure. Because the X683 object is shorter than variants containing `dirty_seg[]/full_seg[]/valid_blks[]`, these three final 64-bit fields remain **probable but not promoted to byte-proven names** until a direct X683 writer/read is located.

Thus the binary-backed semantic boundary is:

```text
known/proven through +0x218
object ends at +0x237
+0x220/+0x228/+0x230 = remaining tail candidates
```

## Corrections superseding older notes

```text
stat +0x164 = call_count
stat +0x168 = cp_count
stat +0x16c = bg_cp_count
stat +0x154 = dirty_count
stat +0x158 = node_pages
stat +0x15c = meta_pages
```

Older descriptions of `+0x16c` as `dirty_count` are obsolete.
