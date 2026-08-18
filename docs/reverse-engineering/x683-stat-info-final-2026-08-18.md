# X683/H694 `f2fs_stat_info` — final binary reconstruction

Authority: uploaded stock `boot(8).img` and standalone compressed kernel, verified against Image SHA-256 `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`.

## 1. Object identity and size

`f2fs_sb_info + 0x568` is a pointer to the X683 statistics object.

The allocator at `0x375c40` requests exactly `0x238` bytes for this object. The initialized list head and `sbi` back-pointer establish the object layout:

```text
+0x000  list_head.next
+0x008  list_head.prev
+0x010  sbi back-pointer
```

The X683 object is therefore a `0x238`-byte statistics variant. It is not safe to paste a later upstream `struct f2fs_stat_info` verbatim over the tail.

## 2. Fully resolved prefix through the vendor GC counters

The following layout is established by direct update/debug accesses plus the matching 4.14-era F2FS statistics ordering:

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
+0x128  aw_cnt
+0x12c  max_aw_cnt
+0x130  vw_cnt
+0x134  max_vw_cnt
+0x138  valid_count
+0x13c  valid_node_count
+0x140  valid_inode_count
+0x144  discard_blks
+0x148  bimodal
+0x14c  avg_vblocks
+0x150  util_free
+0x154  util_valid
+0x158  util_invalid
+0x15c  rsvd/overprovision statistics field family; exact pairing verified by debug print path
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
```

The `+0x15c` slot is deliberately kept conservative because the debug formatter prints `OverProv` and `Resv` as a pair and the exact two-field source ordering needs the corresponding update-site byte slice for final assignment.

## 3. Direct binary proof for the GC counters

The X683 GC body uses:

```text
stat = *(sbi + 0x568)
```

and at `0x35278c..0x352798` performs:

```c
stat->call_count++;
```

The terminal policy path at `0x366e7c..0x366f28` increments:

```c
stat->bg_cp_count++;
```

This corrects earlier project prose that incorrectly identified `stat + 0x16c` as `dirty_count` or as an unspecified vendor completion counter.

Historical-layout-only reasoning is superseded here by direct X683 debug/GC evidence:

```text
stat +0x164 = call_count
stat +0x168 = cp_count
stat +0x16c = bg_cp_count
```

## 4. Segment-accounting proof

The GC completion paths directly establish:

```c
stat->tot_segs++;
stat->node_segs++;
stat->data_segs++;
stat->bg_node_segs += background;
stat->bg_data_segs += background;
```

The relevant X683 offsets are:

```text
+0x170 tot_segs
+0x174 node_segs
+0x178 data_segs
+0x184 bg_node_segs
+0x188 bg_data_segs
```

The later block-accounting path similarly establishes:

```text
+0x18c tot_blks
+0x190 data_blks
+0x194 node_blks
+0x198 bg_data_blks
+0x19c bg_node_blks
```

This mapping is independently validated by the stock debug strings:

```text
" - data segments : %d (%d)"
" - node segments : %d (%d)"
"Try to move %d blocks (BG: %d)"
" - data blocks : %d (%d)"
" - node blocks : %d (%d)"
```

## 5. CP/statistics print-path proof

The stock debug formatter reads:

```text
+0x164 + sbi/stat-derived BG counter  -> "GC calls: %d (BG: %d)"
+0x168 +0x16c                         -> "CP calls: %d (BG: %d)"
+0x17c +0x180 + free-counter         -> "Free: %d (%d)"
```

Therefore `+0x16c` is definitively the background CP counter in the X683 statistics object.

## 6. Seven SBI policy fields now have a stat_info mirror

A stock statistics-copy routine around `0x375ed8..0x375f0c` copies the seven SBI fields into the corresponding I/O statistic fields:

```text
sbi +0x444 -> stat +0x0c0 = nr_wb_cp_data
sbi +0x448 -> stat +0x0c4 = nr_wb_data
sbi +0x44c -> stat +0x0c8 = nr_rd_data
sbi +0x450 -> stat +0x0cc = nr_rd_node
sbi +0x454 -> stat +0x0d0 = nr_rd_meta
sbi +0x45c -> stat +0x0d4 = nr_dio_read
sbi +0x458 -> stat +0x0d8 = nr_dio_write
```

This is the strongest semantic resolution of the seven policy fields obtained so far. They are not arbitrary Transsion GC booleans; they are X683-resident I/O statistics mirrored into `f2fs_stat_info`.

Consequently the seven-field policy gate can be read semantically as an I/O-activity discriminator:

```text
mode != 3:
    any of write/read/direct-I/O counters nonzero
        -> active guarded path
    all seven zero
        -> clean/alternate path
```

The numeric SBI field names themselves should still remain offset-based in source reconstruction until the exact X683 `f2fs_sb_info` declaration is recovered.

## 7. Background flag used by accounting

At X683 `f2fs_gc()` entry:

```text
w0 = sbi
w1 = sync
w2 = background
w3 = requested segno
```

The binary derives a boolean background indicator and stores it in the per-call stack slot consumed by the statistics paths. Therefore the `bg_*` counters above are real `background ? 1 : 0` accumulators, not arbitrary vendor flags.

## 8. Tail beyond `+0x1a8`

The allocator establishes an exact `0x238`-byte object, so the X683 tail ends at `+0x237`.

The first `0x1b0` bytes after `+0x1a0` are therefore not safely recoverable by blindly copying a later public structure. The remaining tail is vendor/X683-specific or a shortened variant of the classic per-current-segment arrays. No field is assigned there without direct load/store evidence.

## 9. Important superseded conclusions

The following older interpretations are no longer authoritative:

```text
stat +0x16c = dirty_count            FALSE
stat +0x16c = generic completion     FALSE
stat +0x164 = dirty_count            FALSE
```

Correct X683 mapping:

```text
stat +0x160 = prefree_count
stat +0x164 = call_count
stat +0x168 = cp_count
stat +0x16c = bg_cp_count
```

The seven SBI policy fields are also now semantically tied to the I/O-stat family through the stock stat-copy routine.

## 10. Remaining stat_info work

The high-value unresolved stat tasks are now only:

```text
+0x15c exact Resv/OverProv member ordering
+0x1b0..+0x237 exact X683 tail/array names
```

Everything through `+0x1a8` used by GC/debug policy is now semantically mapped with binary-backed evidence.
