# X683/H694 F2FS statistics and dirty-info recovery

Target stock kernel: Linux 4.14.141+ / X683-H694.

This document records the current binary-derived reconstruction of the F2FS statistics area and the dirty-segment accounting path. Public F2FS source is used only as a structural reference; the stock X683 binary remains authoritative.

## 1. `stat_info` pointer

The stock `f2fs_gc()` loads a pointer from `sbi + 0x568` and uses the resulting object for F2FS GC statistics/accounting.

Current interpretation:

```text
sbi + 0x568 = struct f2fs_stat_info *
```

Confidence: **high**.

### Sanity correction

The bytes after `sbi + 0x568` are **not** automatically members of the pointed-to `struct f2fs_stat_info`.

The pointer itself occupies the `sbi` slot at `0x568`. Therefore addresses such as `sbi + 0x570` and `sbi + 0x5d4` are still offsets within `struct f2fs_sb_info`.

Historical F2FS source commonly places additional statistics counters directly in `struct f2fs_sb_info` after the `stat_info` pointer. Therefore the following mapping is a candidate **SBI statistics region**, not a `stat_info` object layout.

## 2. Historical SBI statistics-region fingerprint

The November 2018 Android/common 4.14 generation is a particularly strong structural reference. Its `f2fs_sb_info` declaration places the statistics fields immediately after `stat_info` in the following order. Normal ARM64/C alignment gives this candidate X683-era sequence:

```text
0x568  stat_info pointer
0x570  meta_count[4]
0x580  segment_count[2]
0x588  block_count[2]
0x590  inplace_count
0x594  padding for 64-bit alignment
0x598  total_hit_ext
0x5a0  read_hit_rbtree
0x5a8  read_hit_largest
0x5b0  read_hit_cached
0x5b8  inline_xattr
0x5bc  inline_inode
0x5c0  inline_dir
0x5c4  aw_cnt
0x5c8  vw_cnt
0x5cc  max_aw_cnt
0x5d0  max_vw_cnt
0x5d4  bg_gc
0x5d8  io_skip_bggc
0x5dc  other_skip_bggc
```

This sequence matches the candidate offsets already recovered from the X683 binary analysis. The structural match is now strong enough to sharply narrow the historical source family, but these names remain **structural candidates** until individual X683 call sites prove the access semantics. In particular, `0x5d4..0x5dc` are not promoted merely because the historical ordering matches.

The same historical generation also contains the four-argument ABI:

```c
f2fs_gc(sbi, sync, background, segno);
```

and the GC-manager/statistics combination seen in X683.

## 3. Direct X683 dirty-info evidence

The corrected `tran_gc_thread_func` disassembly contains this sequence:

```asm
ldr     x8, [x19, #0x80]
ldr     x9, [x8]
ldr     w21, [x19, #0x3d8]
ldr     w23, [x9, #0x10]
...
ldp     x8, x9, [x8, #0x8]
...
ldp     w10, w11, [x9, #0x68]
ldp     w12, w10, [x9, #0x70]
ldp     w11, w9,  [x9, #0x78]
```

The pointer chain is:

```text
x19 = sbi
sbi + 0x80 -> sm_info
sm_info + 0x10 -> dirty_info
```

The six values read from `dirty_info + 0x68..0x7c` participate in the same calculation.

This is consistent with the historical F2FS dirty-segment accounting array, but the exact semantic mapping still requires another independent X683 path.

Current hypothesis:

```text
dirty_info + 0x68  nr_dirty[0]  candidate
dirty_info + 0x6c  nr_dirty[1]  candidate
dirty_info + 0x70  nr_dirty[2]  candidate
dirty_info + 0x74  nr_dirty[3]  candidate
dirty_info + 0x78  nr_dirty[4]  candidate
dirty_info + 0x7c  nr_dirty[5]  candidate
```

Confidence: **high structural match**, but not yet field-name confirmation.

## 4. `0x5d4..0x5dc`

The historical source fingerprint remains:

```text
0x5d4  bg_gc
0x5d8  io_skip_bggc
0x5dc  other_skip_bggc
```

Current confidence:

```text
0x5d4  strong structural candidate
0x5d8  strong structural candidate
0x5dc  strong structural candidate
```

No direct X683 instruction has yet been found that uniquely proves all three names. They must remain candidates until the corresponding counter read/increment call sites are recovered.

## 5. `struct f2fs_stat_info` itself

The actual object pointed to by `sbi + 0x568` must be reconstructed separately.

Do **not** infer its members from the `sbi + 0x570..` addresses.

Historical 4.14 F2FS provides a reference object beginning approximately with:

```text
+0x00  stat_list
+0x10  sbi
+0x18  all_area_segs
+0x1c  sit_area_segs
+0x20  nat_area_segs
+0x24  ssa_area_segs
+0x28  main_area_segs
+0x2c  main_area_sections
+0x30  main_area_zones
+0x38  hit_largest
+0x40  hit_cached
+0x48  hit_rbtree
+0x50  hit_total
+0x58  total_ext
+...
```

These are **historical reference offsets only**, not X683 offsets. The actual X683 object remains unresolved.

The next binary pass must follow the loaded `stat_info` pointer and recover accesses relative to that pointer, e.g.:

```text
stat_info pointer
    -> +0x00
    -> +0x04
    -> +0x08
    -> ...
```

Only those pointer-relative offsets can establish the actual `struct f2fs_stat_info` layout.

## 6. Reconstruction rule

The working order is:

```text
stock disassembly
    -> identify base register
    -> exact byte offset
    -> distinguish SBI vs pointed-to object
    -> historical structure correlation
    -> independent second call site
    -> promote field name
```

A historical match alone is never sufficient to promote a field to binary-confirmed status.
