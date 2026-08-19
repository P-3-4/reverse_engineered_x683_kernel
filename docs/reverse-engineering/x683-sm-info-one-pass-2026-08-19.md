# X683/H694 F2FS `f2fs_sm_info` one-pass reconstruction

Target: stock Infinix X683/H694 kernel, Linux 4.14.141+.

Evidence basis: decompressed X683 boot Image, X683 kallsyms, direct AArch64 disassembly of the stock `f2fs_build_segment_manager`, `f2fs_create_flush_cmd_control`, `f2fs_destroy_flush_cmd_control`, and `f2fs_destroy_segment_manager` paths, plus historical Android/common 4.14 F2FS source correlation.

## Executive result

The stock X683 binary allocates exactly `0xa8` bytes for `struct f2fs_sm_info` and stores it at `sbi + 0x80`.

The two previously opaque tail pointers are now binary-confirmed:

```text
sm_info + 0x98 = fcc_info  -> struct flush_cmd_control *
sm_info + 0xa0 = dcc_info  -> struct discard_cmd_control *
```

The discard-control object is allocated at exactly `0x20b0` bytes and its complete field layout can be recovered from constructor initialization. The flush-control object has the expected 4.14 layout.

A second important correction is that the X683 `sm_info` geometry does NOT begin at `+0x40` as the older working document assumed. The constructor directly initializes the geometry sequence beginning at `+0x48`. Therefore `+0x40..0x47` is a real unresolved 8-byte X683 field/reserved extension and must not be silently renamed `seg0_blkaddr`.

## 1. `f2fs_sm_info` size and anchor

At stock Image offset `0x36d138` (`f2fs_build_segment_manager` according to the X683 symbol table), the binary allocates `0xa8` bytes and stores the result at `sbi + 0x80`.

Thus:

```text
sbi + 0x80 = struct f2fs_sm_info *
sizeof(struct f2fs_sm_info) = 0xa8
```

The `+0x98` and `+0xa0` members are therefore the final two pointer slots in the object.

## 2. Recovered `sm_info` layout

| Offset | Working interpretation | Status |
|---:|---|---|
| `0x00` | `sit_info *` | strong; destructor/free path + historical ordering |
| `0x08` | `free_info *` | binary-confirmed pointer installation |
| `0x10` | `dirty_info *` | binary-confirmed pointer chain in GC path |
| `0x18` | `curseg_array *` | binary-confirmed; allocation is `0x2a0` = 6 × `0x70` |
| `0x20` | `curseg_lock` | binary-confirmed initialization target |
| `0x40` | unknown 64-bit X683 field | **unresolved; do not call `seg0_blkaddr`** |
| `0x48` | `seg0_blkaddr` | strong binary/source match |
| `0x4c` | `main_blkaddr` | strong binary/source match |
| `0x50` | `ssa_blkaddr` | strong binary/source match |
| `0x54` | `segment_count` | strong binary/source match |
| `0x58` | `main_segments` | strong binary/source match |
| `0x5c` | `reserved_segments` | strong binary/source match |
| `0x60` | `ovp_segments` | strong binary/source match |
| `0x64` | `rec_prefree_segments`-like threshold | strong structural match; exact X683 naming pending |
| `0x68` | `sit_entry_set` list head | binary-confirmed initialization |
| `0x70` | second word of `sit_entry_set` list head | binary-confirmed |
| `0x78` | `ipu_policy` candidate | structural; exact vendor layout still being verified |
| `0x7c` | adjacent policy field | unresolved |
| `0x80` | policy threshold field | binary initialized conditionally to `0x10`; name unresolved |
| `0x84` | 64-bit initialization region | binary exact; contains fixed low/high words before later writes |
| `0x8c` | `blocks_per_seg * segs_per_sec` threshold candidate | binary exact; likely sequential-block threshold family |
| `0x90` | constant `0x10` | binary exact; semantic name unresolved |
| `0x94` | reserved/main-segment-derived threshold candidate | binary exact; computed from `sm_info+0x5c / sbi+0x3e0` |
| `0x98` | `fcc_info *` | **binary-confirmed** |
| `0xa0` | `dcc_info *` | **binary-confirmed** |

Historical F2FS uses the same core geometry sequence (`seg0_blkaddr`, `main_blkaddr`, `ssa_blkaddr`, segment counts, reserved/overprovision values, reclaim threshold, list head, policy fields, then flush/discard control pointers). The X683 binary, however, shifts the proven geometry sequence to `+0x48`, leaving `+0x40..0x47` unresolved. Historical source is used as correlation only. citeturn5search0turn5search1

### Important correction to the previous reconstruction

Earlier documentation placed:

```text
sm_info + 0x40 = seg0_blkaddr
```

That is not supported by the stock constructor. The constructor writes the value copied from the raw-super `segment0_blkaddr` at `+0x48`, and no corresponding constructor store to `+0x40` was found in `f2fs_build_segment_manager`.

The safe representation is therefore:

```c
struct f2fs_sm_info_x683 {
        void *sit_info;          /* +0x00 */
        void *free_info;         /* +0x08 */
        void *dirty_info;        /* +0x10 */
        void *curseg_array;      /* +0x18 */
        /* +0x20: rwsem */
        /* +0x40: UNKNOWN X683 64-bit field */
        u32 seg0_blkaddr;        /* +0x48 */
        u32 main_blkaddr;        /* +0x4c */
        u32 ssa_blkaddr;         /* +0x50 */
        u32 segment_count;       /* +0x54 */
        u32 main_segments;       /* +0x58 */
        u32 reserved_segments;   /* +0x5c */
        u32 ovp_segments;        /* +0x60 */
        u32 rec_prefree_like;    /* +0x64 */
        /* +0x68: list_head */
        /* +0x78..0x94: policy/threshold region; exact X683 names pending */
        void *fcc_info;          /* +0x98 */
        void *dcc_info;          /* +0xa0 */
};
```

This is deliberately offset-based until the `+0x40` and `+0x78..0x94` vendor layout is independently proven.

## 3. `curseg_array`

The constructor allocates exactly `0x2a0` bytes and stores the resulting pointer at `sm_info + 0x18`.

The subsequent loop advances the object pointer by `0x70` six times:

```text
0x2a0 / 0x70 = 6
```

Therefore:

```text
sm_info + 0x18 = struct curseg_info *curseg_array
curseg_array[0..5]
sizeof(struct curseg_info) = 0x70
```

This matches the six normal F2FS current-segment slots in this kernel generation.

## 4. `free_info`

The constructor allocates a `0x20`-byte object and stores it at `sm_info + 0x08`.

It then allocates two bitmap-related buffers and places their pointers at object offsets `+0x10` and `+0x18`. This matches the historical `struct free_segmap_info` organization.

Status: **binary-confirmed object identity; individual scalar names remain subject to the child-structure pass.**

## 5. `dirty_info`

The stock GC path independently proves:

```text
sbi + 0x80 -> sm_info
sm_info + 0x10 -> dirty_info
```

and reads the consecutive counters:

```text
dirty_info + 0x68
+0x6c
+0x70
+0x74
+0x78
+0x7c
```

These match the historical `nr_dirty[]` accounting region. The current binary evidence is sufficient to retain `dirty_info` as a named child, while the six semantic counter names remain one independent-call-site proof away.

## 6. `fcc_info`: complete reconstruction

The X683 `f2fs_create_flush_cmd_control` path obtains `sm_info` from `sbi + 0x80`, reads `sm_info + 0x98`, allocates the control object if absent, initializes it, and stores it back at `+0x98`.

The recovered object is the historical 4.14 `struct flush_cmd_control`:

```text
+0x00  f2fs_issue_flush       struct task_struct *
+0x08  flush_wait_queue       wait_queue_head_t
+0x20  issued_flush           atomic_t
+0x24  queued_flush           atomic_t
+0x28  issue_list              struct llist_head
+0x30  dispatch_list          struct llist_node *
```

The constructor directly zeroes `+0x20`, `+0x24`, and `+0x28`, initializes the wait queue at `+0x08`, and stores/stops the thread pointer at `+0x00`. The destructor clears the pointer and frees the object when requested.

This matches historical Android/common F2FS. citeturn3view0

## 7. `dcc_info`: complete reconstruction

The X683 constructor allocates exactly `0x20b0` bytes for the discard-control object and stores it at `sm_info + 0xa0`.

The binary initialization gives an unusually strong complete layout:

| Offset | Field | Evidence |
|---:|---|---|
| `0x0000` | `f2fs_issue_discard` | thread pointer initialized later |
| `0x0008` | `entry_list` | list head initialized |
| `0x0018` | `pend_list[0]` | first pending list |
| `0x0018..0x2017` | `pend_list[512]` | 512 × 16-byte list heads |
| `0x2018` | `wait_list` | list head initialized |
| `0x2028` | `fstrim_list` | list head initialized |
| `0x2038` | `discard_wait_queue` | wait-queue initialization region |
| `0x2050` | `discard_wake` | scalar between wait queue and mutex |
| `0x2058` | `cmd_lock` | mutex initialization target |
| `0x2078` | `nr_discards` | zeroed |
| `0x207c` | `max_discards` | `main_segments << log_blocks_per_seg` |
| `0x2080` | `discard_granularity` | initialized from device/config path |
| `0x2084` | `undiscard_blks` | zeroed |
| `0x2088` | `next_pos` | zeroed |
| `0x208c` | `issued_discard` | atomic zero |
| `0x2090` | `queued_discard` | atomic zero |
| `0x2094` | `discard_cmd_cnt` | atomic zero |
| `0x2098` | `root` | zeroed RB-root storage |
| `0x20a8` | `rbtree_check` | byte zeroed |

The `0x20b0` size is independently explained by the 512-entry pending-list array and the historical field ordering. Historical F2FS source places `entry_list`, `pend_list`, `wait_list`, `fstrim_list`, wait queue, wake flag, mutex, discard counters, RB tree, and consistency flag in this same order. citeturn4search0turn4search4

### X683 DCC observation

The X683 constructor computes:

```text
max_discards = MAIN_SEGS << log_blocks_per_seg
```

and writes it at `+0x207c`.

That is stock F2FS behavior, not evidence of a Transsion policy addition. The object layout itself therefore appears stock for this source generation.

## 8. Transsion delta classification for this phase

### Confirmed stock F2FS

```text
free_info
 dirty_info
curseg_array
fcc_info
dcc_info
curseg_lock
segment geometry
sit_entry_set
normal discard-control object
normal flush-control object
```

These align with historical Android/common 4.14 F2FS. citeturn5search0turn5search3turn4search0

### Likely X683/vendor extension

The strongest unresolved vendor-layout evidence is:

```text
sm_info + 0x40..0x47
sm_info + 0x78..0x94
```

The standard historical structure explains the surrounding fields, but the X683 constructor's exact writes do not justify silently assigning historical names to every slot. In particular, the 8-byte region at `+0x40` is directly before the geometry sequence and is not initialized as `seg0_blkaddr`.

### Not a Transsion delta

The following should NOT be labeled vendor code merely because they were recovered from X683:

```text
fcc_info
flush queue fields
dcc_info
512-entry pending-list array
cmd_lock
issued/queued/discard counters
RB-tree root
curseg_array
free_info
```

These are normal F2FS machinery and have direct historical source matches. citeturn3view0turn4search0

## 9. Remaining child structures

The parent and control-object boundaries are now sharply constrained:

```text
struct sit_info           = sm_info + 0x00
struct free_segmap_info   = sm_info + 0x08
struct dirty_seglist_info = sm_info + 0x10
struct curseg_info[6]     = sm_info + 0x18
```

The next child-level reconstruction should recover these from their constructor/destructor accesses and runtime users rather than infer them from generic source alone.

For `stat_info`, continue treating `sbi + 0x568` as the pointer itself; offsets after `0x568` belong to `f2fs_sb_info`, not the pointed object. The project statistics recovery document records that distinction. fileciteturn39file0L2-L2

## 10. Phase conclusion

This pass closes the original `sm_info + 0x98 / +0xa0` question and establishes the full discard/flush control boundaries.

The strongest new facts are:

```text
sizeof(f2fs_sm_info) = 0xa8
sm_info + 0x98 = flush_cmd_control *
sm_info + 0xa0 = discard_cmd_control *
sizeof(discard_cmd_control) = 0x20b0
curseg_array = 6 × 0x70
sm_info + 0x40..0x47 remains a genuine unknown X683 field
```

No unsupported vendor names are being promoted into the structure.
