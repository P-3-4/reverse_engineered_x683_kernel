# X683/H694 F2FS `sm_info` child-structure reconstruction

Target: stock Infinix X683/H694, Linux 4.14.141+.

Binary authority: decompressed stock X683 Image and X683 kallsyms. Historical F2FS source is correlation only.

## Executive result

The stock `f2fs_build_segment_manager()` gives enough evidence to promote most of the four `sm_info` child identities and several internal fields.

```text
sm_info + 0x00 -> sit_info *
sm_info + 0x08 -> free_segmap_info *
sm_info + 0x10 -> dirty_seglist_info *
sm_info + 0x18 -> curseg_info[6]
```

The parent object is `0xa8` bytes. The child allocations observed in the constructor are:

```text
free_info   = 0x20 bytes
curseg      = 0x2a0 bytes = 6 * 0x70
 dirty_info = 0x90 bytes
```

## 1. `free_segmap_info`

The X683 constructor allocates exactly `0x20` bytes and stores it at `sm_info + 0x08`. It allocates two bitmap objects and stores them at `+0x10` and `+0x18`.

The resulting layout is the historical 4.14 organization:

| Offset | Candidate | Confidence |
|---:|---|---|
| `+0x00` | `start_segno` | strong; later constructor arithmetic initializes it |
| `+0x04` | `free_segments` | strong structural match |
| `+0x08` | `free_sections` | strong structural match |
| `+0x0c` | `segmap_lock` | binary zeroing / historical position |
| `+0x10` | `free_segmap` | **binary-confirmed pointer** |
| `+0x18` | `free_secmap` | **binary-confirmed pointer** |

The bitmap sizes are derived from `main_segments`, matching the normal F2FS free-segment manager. Historical F2FS defines the same fields in this order. citeturn0search4turn5search0

## 2. `dirty_seglist_info`

The X683 constructor allocates exactly `0x90` bytes and stores the object at `sm_info + 0x10`.

The constructor initializes a mutex at `dirty_info + 0x48`. It then allocates eight bitmap pointers at:

```text
+0x08
+0x10
+0x18
+0x20
+0x28
+0x30
+0x38
+0x40
```

The remaining allocation at `+0x88` is a victim bitmap.

This gives a very strong reconstruction:

| Offset | Candidate | Confidence |
|---:|---|---|
| `+0x00` | `v_ops` / victim-selection pointer | strong historical match; constructor leaves it for later installation |
| `+0x08..+0x40` | `dirty_segmap[8]` | **binary-confirmed as eight pointer slots** |
| `+0x48` | `seglist_lock` (`struct mutex`) | **binary-confirmed** by `__mutex_init` |
| `+0x68..+0x84` | `nr_dirty[8]` | **binary-confirmed array region**; GC directly reads `+0x68..+0x7c` |
| `+0x88` | `victim_secmap` / victim bitmap | **binary-confirmed allocation target** |

The eight `nr_dirty` entries are therefore:

```text
+0x68 nr_dirty[0]
+0x6c nr_dirty[1]
+0x70 nr_dirty[2]
+0x74 nr_dirty[3]
+0x78 nr_dirty[4]
+0x7c nr_dirty[5]
+0x80 nr_dirty[6]
+0x84 nr_dirty[7]
```

The historical dirty-type ordering is the six CURSEG types followed by `DIRTY` and `PRE`, which explains why the X683 GC path reads six consecutive counters in the same region. citeturn0search4turn0search6

This is stronger than the previous state: the six-counter region is now established as the first six elements of an eight-element `nr_dirty[]` array, not six unrelated vendor counters.

## 3. `curseg_info[6]`

The constructor allocates `0x2a0` bytes and advances by `0x70` for six iterations. Therefore:

```text
sizeof(struct curseg_info) = 0x70
NR_CURSEG_TYPE = 6
```

Each entry is initialized as follows:

| Offset | Candidate | Confidence |
|---:|---|---|
| `+0x00` | `curseg_mutex` | **binary-confirmed** (`__mutex_init`) |
| `+0x20` | `sum_blk` | **binary-confirmed**; 0x1000-byte allocation stored here |
| `+0x28` | `journal_rwsem` | **binary-confirmed** (`__init_rwsem`) |
| `+0x50` | `journal` | **binary-confirmed**; 0x1fb-byte allocation stored here |
| `+0x5c` | `segno` | strong; initialized to `NULL_SEGNO` (`-1`) |
| `+0x60` | `next_blkoff`/adjacent 16-bit state | binary-zeroed; exact member assignment remains to be proven |

The constructor's pointer and lock offsets are authoritative. The scalar tail should not yet be force-fitted to a historical layout because the X683 object has a `0x70` stride and the exact ARM64 lock layout must be respected.

Historical F2FS independently confirms the semantic fields `curseg_mutex`, `sum_blk`, `journal_rwsem`, `journal`, `alloc_type`, `segno`, `next_blkoff`, `zone`, and `next_segno`. citeturn2search2turn2search4

## 4. `sit_info`

The constructor allocates exactly `0xa8` bytes for `sit_info` and installs it at `sm_info + 0x00`.

The following fields are directly established:

| Offset | Candidate | Confidence |
|---:|---|---|
| `+0x00` | `s_ops` / segment-allocation operations pointer | **binary-confirmed non-null function-table pointer** |
| `+0x08` | `sit_base_addr` | **binary-confirmed scalar initialization** |
| `+0x0c` | `sit_blocks` | strong structural match |
| `+0x10` | `written_valid_blocks` | **binary-zeroed** |
| `+0x18` | SIT bitmap pointer | **binary-confirmed allocation/copy destination** |
| `+0x20` | bitmap-size/count field | strong; initialized from the SIT bitmap calculation |
| `+0x40` | `sentry_lock` | **binary-confirmed** by `__init_rwsem` |
| `+0x68` | `sentries` | **binary-confirmed pointer allocation** |
| `+0x70` | `sec_entries` | **binary-confirmed conditional allocation** |
| `+0x78` | time/state field | binary-confirmed 64-bit initialization; semantic name unresolved |
| `+0x80` | time/state field | binary-confirmed 64-bit initialization from time helper |

The X683 `sentries` array uses a `0x28`-byte stride. Each segment entry receives three pointer-like bitmap allocations at relative `+0x08`, `+0x10`, and `+0x18`. This matches the historical `seg_entry` family containing current-valid, checkpoint-valid, and discard/related maps. citeturn3search4turn4search2

The `sec_entries` allocation is conditional on `segs_per_sec >= 2`, matching historical F2FS section-level accounting. citeturn4search2

### Important unresolved SIT region

The X683 `sit_info` object is `0xa8` bytes, but its exact later time/victim tail differs from some historical 4.14 revisions. Therefore fields after `+0x80` are not being assigned historical names without direct X683 runtime confirmation.

## 5. `sm_info + 0x40..0x47`

This is now confirmed as a real **8-byte X683 insertion before the standard geometry sequence**.

The constructor begins the standard geometry at:

```text
+0x48 seg0_blkaddr
+0x4c main_blkaddr
+0x50 ssa_blkaddr
+0x54 segment_count
+0x58 main_segments
+0x5c reserved_segments
+0x60 ovp_segments
+0x64 rec_prefree_segments-like value
+0x68 trim_sections-like slot
+0x70 sit_entry_set list_head
```

This is exactly the historical ordering shifted by eight bytes, while the final control pointers remain at `+0x98/+0xa0`. Historical F2FS places `seg0_blkaddr`, the geometry/count fields, reclaim threshold, trim count, list head, policy fields, and then flush/discard pointers in this sequence. citeturn7search0turn7search1

The safe conclusion is:

```text
sm_info + 0x40..0x47 = UNKNOWN X683 64-bit field
```

It must not be renamed `seg0_blkaddr`.

## 6. `sm_info + 0x78..0x94`

The surrounding fields can now be split more accurately using the historical sequence and the X683 constructor:

```text
+0x78  ipu_policy                  candidate/strong structural match
+0x7c  min_ipu_util                candidate/strong structural match
+0x80  min_fsync_blocks            binary writes 0x10 conditionally
+0x84  min_seq_blocks              low 32 bits of a binary 64-bit initialization
+0x88  min_hot_blocks              high 32 bits of that same initialization
+0x8c  min_ssr_sections            binary writes computed threshold
+0x90..0x97  UNKNOWN X683 64-bit extension
+0x98  fcc_info                    confirmed pointer
```

The important correction is that `+0x90` is **not** safely a standalone `u32` policy field. The `fcc_info` pointer is aligned at `+0x98`, leaving an eight-byte region immediately before it. The constructor writes `0x10` into the low word of that region, so the whole `+0x90..0x97` area should remain an X683 extension until a runtime consumer proves its type.

This also resolves the earlier misleading description of `+0x8c..0x94` as unrelated threshold fields: the historical policy sequence accounts for `+0x78..0x8c`, while `+0x90..0x97` is the remaining vendor/unknown slot before `fcc_info`.

## 7. Transsion-vs-stock classification

### Stock / historical F2FS strongly supported

```text
free_segmap_info
  start/free counters
  segmap_lock
  free_segmap/free_secmap

dirty_seglist_info
  dirty_segmap[8]
  seglist_lock
  nr_dirty[8]
  victim bitmap

curseg_info
  curseg_mutex
  sum_blk
  journal_rwsem
  journal
  segment state fields

sit_info
  SIT base/count state
  SIT bitmap state
  sentry lock
  sentries
  section entries

sm_info
  geometry
  sit_entry_set
  IPU/threshold policy sequence
  flush/discard controls
```

These have strong historical F2FS correspondence. citeturn0search4turn2search4turn7search0

### Genuine X683 modifications still isolated

```text
sm_info + 0x40..0x47
sm_info + 0x90..0x97
```

These are the two parent-structure regions that cannot be explained by simply selecting the historical structure and applying normal alignment.

The X683 configuration also contains `CONFIG_F2FS_TRAN_GC=y`, which supports treating the GC-policy changes elsewhere in the project as vendor functionality, but it does not by itself prove that either of these two structure slots is specifically a Transsion field.

## 8. Current reconstructed parent

```c
struct f2fs_sm_info_x683 {
        void *sit_info;              /* +0x00 */
        void *free_info;             /* +0x08 */
        void *dirty_info;            /* +0x10 */
        void *curseg_array;           /* +0x18 */
        /* +0x20: curseg_lock */
        /* ... */
        u64  x683_unknown_40;         /* +0x40 */
        u32  seg0_blkaddr;            /* +0x48 */
        u32  main_blkaddr;            /* +0x4c */
        u32  ssa_blkaddr;             /* +0x50 */
        u32  segment_count;           /* +0x54 */
        u32  main_segments;           /* +0x58 */
        u32  reserved_segments;       /* +0x5c */
        u32  ovp_segments;            /* +0x60 */
        u32  rec_prefree;              /* +0x64 */
        u32  trim_sections;            /* +0x68 */
        struct list_head sit_entry_set;/* +0x70 */
        u32  ipu_policy;              /* +0x80? see note */
        /* +0x78..0x8c retain exact scalar mapping pending direct consumers */
        u64  x683_unknown_90;         /* +0x90 */
        void *fcc_info;               /* +0x98 */
        void *dcc_info;               /* +0xa0 */
};
```

The C sketch intentionally leaves the policy scalar offsets annotated rather than presenting an internally inconsistent field list. The offset table above is authoritative for this phase.

## 9. Phase status

This pass materially closes the four-child question:

```text
sit_info           identity + major anchors recovered
free_segmap_info   effectively reconstructed
 dirty_seglist_info effectively reconstructed
curseg_info[6]     object size + major pointer/lock anchors recovered
```

The remaining work is no longer broad structure discovery. It is **consumer-based semantic proof** for:

```text
sit_info tail fields
curseg scalar tail fields
sm_info +0x40..0x47
sm_info +0x90..0x97
```

Those should now be attacked through their runtime call sites rather than by searching for another generic F2FS structure definition.
