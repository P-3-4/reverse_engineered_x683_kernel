# X683 F2FS segment-manager child layouts — final binary pass

Date: 2026-08-19
Target: Infinix X683 stock kernel, Linux 4.14-derived F2FS
Authority: decompressed stock X683 Image + X683 kallsyms. Historical F2FS source is used only for member naming/correlation.

## Executive result

The four `sm_info` child structures are now sufficiently reconstructed to close this phase. The remaining apparent unknown parent regions were resolved as normal structure members/alignment; they are not unexplained Transsion insertions.

Exact X683 object sizes:

```text
struct sit_info           0xA8
struct free_segmap_info   0x20
struct dirty_seglist_info 0x90
struct curseg_info        0x70
curseg_array              0x2A0 = 6 * 0x70
```

The X683 `f2fs_sm_info` itself is `0xA8` bytes.

## 1. `sit_info` — final layout

```text
+0x00  s_ops                         pointer
+0x08  sit_base_addr                 u32 / block_t
+0x0c  sit_blocks                    u32 / block_t
+0x10  written_valid_blocks          u64
+0x18  sit_bitmap                    pointer
+0x20  bitmap_size                   u32
+0x28  tmp_map                       pointer
+0x30  dirty_sentries_bitmap         pointer
+0x38  dirty_sentries                u32
+0x3c  sents_per_block               u32 = 55 (0x37)
+0x40  sentry_lock                   rw semaphore object, 0x28 bytes
+0x68  sentries                      struct seg_entry *
+0x70  sec_entries                   struct sec_entry *
+0x78  elapsed_time                  u64
+0x80  mounted_time                  u64
+0x88  min_mtime                     u64
+0x90  max_mtime                     u64
+0x98  last_victim[4]                u32[4]
```

The constructor allocates exactly `0xA8` bytes. It explicitly initializes `s_ops`, SIT base/size, bitmap size, `dirty_sentries=0`, `sents_per_block=0x37`, `sentry_lock`, `sentries`, `sec_entries`, and the time fields. The later constructor logic writes `min_mtime` at `+0x88` and `max_mtime` at `+0x90`; this resolves the earlier mistaken interpretation of those offsets as vendor fields.

`0x37` is correct: a 4 KiB SIT block holds 55 packed 74-byte SIT entries. This is a normal F2FS on-disk constraint, not a vendor magic value.

The four `last_victim` entries occupy the final 16 bytes. The corresponding historical policy sequence is `GC_CB`, `GC_GREEDY`, `ALLOC_NEXT`, `FLUSH_DEVICE`, `MAX_GC_POLICY=4`.

### SIT nested objects

`seg_entry` is allocated/iterated with a `0x28`-byte stride. The constructor and runtime consumers establish the X683 segment-entry layout family containing:

```text
+0x00  type/valid-block bitfield
+0x04  packed validity/checkpoint state
+0x08  cur_valid_map pointer
+0x10  ckpt_valid_map pointer
+0x18  discard/related validity-map pointer
+0x20  mtime / remaining segment state
```

`sec_entry` is allocated with 4-byte entries in the X683 constructor (`sec_entries` is allocated as `count << 2`). Its primary member is `valid_blocks`.

## 2. `free_segmap_info` — final layout

```text
+0x00  start_segno                  u32
+0x04  free_segments                u32
+0x08  free_sections                u32
+0x0c  segmap_lock                  spinlock_t
+0x10  free_segmap                  unsigned long *
+0x18  free_secmap                  unsigned long *
```

The object is exactly `0x20` bytes. The constructor explicitly zeros the lock storage and initializes the three scalar members, then allocates/stores both bitmap pointers.

This matches the older 4.14 F2FS segment-manager variant; no X683-specific structural extension is indicated.

## 3. `dirty_seglist_info` — final layout

```text
+0x00  v_ops                         const struct victim_selection *
+0x08  dirty_segmap[0]              pointer
+0x10  dirty_segmap[1]              pointer
+0x18  dirty_segmap[2]              pointer
+0x20  dirty_segmap[3]              pointer
+0x28  dirty_segmap[4]              pointer
+0x30  dirty_segmap[5]              pointer
+0x38  dirty_segmap[6]              pointer
+0x40  dirty_segmap[7]              pointer
+0x48  seglist_lock                  struct mutex, 0x20 bytes
+0x68  nr_dirty[0]                  s32/u32
+0x6c  nr_dirty[1]
+0x70  nr_dirty[2]
+0x74  nr_dirty[3]
+0x78  nr_dirty[4]
+0x7c  nr_dirty[5]
+0x80  nr_dirty[6]
+0x84  nr_dirty[7]
+0x88  victim_secmap                 unsigned long *
```

The object is exactly `0x90` bytes.

This is an important baseline determination: the X683 object has **no `dirty_secmap` member** between the bitmap array and `seglist_lock`. That is consistent with the older F2FS variant used by this kernel, not evidence of a Transsion replacement. The constructor allocates eight segment bitmaps, initializes the mutex at `+0x48`, and later allocates `victim_secmap` at `+0x88`.

The previously observed X683 accesses at `+0x68..+0x7c` are therefore the first six `nr_dirty[]` counters.

## 4. `curseg_info` — final layout

The constructor allocates `0x2A0` bytes and advances by `0x70` exactly six times. Therefore:

```text
NR_CURSEG_TYPE = 6
sizeof(struct curseg_info) = 0x70
```

Final member map:

```text
+0x00  curseg_mutex                 struct mutex, 0x20 bytes
+0x20  sum_blk                      struct f2fs_summary_block *
+0x28  journal_rwsem                rw semaphore, 0x28 bytes
+0x50  journal                      struct f2fs_journal *
+0x58  alloc_type                   u8
+0x59  padding
+0x5c  segno                        u32
+0x60  next_blkoff                  u16
+0x62  padding
+0x64  zone                         u32
+0x68  next_segno                   u32
+0x6c  padding
```

The constructor explicitly initializes the mutex, allocates `sum_blk` with size `0x1000`, initializes the rwsem, allocates `journal` with size `0x1FB`, sets `segno = NULL_SEGNO`, and clears `next_blkoff`. The other scalar members are zero from the zeroed allocation and are subsequently consumed by the segment-allocation code.

There is **no `seg_type` or `inited` member in this X683 object**. Those belong to later F2FS revisions and must not be imported into the reconstruction.

## 5. Parent `f2fs_sm_info` correction

The child pass also permanently resolves the previous parent-layout confusion.

```text
+0x00  sit_info *
+0x08  free_info *
+0x10  dirty_info *
+0x18  curseg_array *
+0x20  curseg_lock                   struct f2fs_rwsem, 0x28 bytes
+0x48  seg0_blkaddr
+0x4c  main_blkaddr
+0x50  ssa_blkaddr
+0x54  segment_count
+0x58  main_segments
+0x5c  reserved_segments
+0x60  ovp_segments
+0x64  rec_prefree_segments
+0x68  trim_sections
+0x70  sit_entry_set                  struct list_head
+0x80  ipu_policy
+0x84  min_ipu_util
+0x88  min_fsync_blocks
+0x8c  min_seq_blocks
+0x90  min_hot_blocks
+0x94  min_ssr_sections
+0x98  fcc_info                       struct flush_cmd_control *
+0xa0  dcc_info                       struct discard_cmd_control *
```

Thus the formerly suspicious `sm_info + 0x40..0x47` is simply the tail of `curseg_lock`; it is not an unknown vendor field. Likewise the formerly suspicious `+0x90..0x97` is the normal IPU/SSR threshold region, not an X683 insertion.

## 6. Baseline vs Transsion classification

### Binary-confirmed stock/older-F2FS structures

```text
sit_info
free_segmap_info
dirty_seglist_info
curseg_info
f2fs_sm_info geometry/policy layout
flush_cmd_control
discard_cmd_control
```

Their object sizes, member placement, constructor/destructor behavior, and consumers correspond to known 4.14-era F2FS variants.

### Genuine X683/Transsion work is elsewhere

The X683 configuration explicitly enables:

```text
CONFIG_F2FS_TRAN_GC=y
```

and the binary contains separate vendor symbols:

```text
tran_gc_thread_func
tran_urgent_gc_read
tran_urgent_gc_write
tran_do_f2fs_gc
tran_gc_init
tran_gc_stop
```

Those are the correct targets for the vendor-delta phase. They should not be represented as invented fields inside the four standard segment-manager child structures.

## 7. Phase closure

This phase is now closed.

We have exact binary-backed layouts for:

```text
f2fs_sm_info
sit_info
free_segmap_info
dirty_seglist_info
curseg_info[6]
```

plus the previously completed `flush_cmd_control` and `discard_cmd_control` objects.

The remaining work belongs to the next phase: reconstructing the **actual Transsion GC implementation**, starting with the `tran_gc_*` symbols and then correlating their accesses against the standard F2FS GC/victim-selection path.

No unresolved `sm_info +0x40` or `+0x90` vendor fields remain. Earlier notes claiming those were unknown X683 insertions are superseded by this document.
