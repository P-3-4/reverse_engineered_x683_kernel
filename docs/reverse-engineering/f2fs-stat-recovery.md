# X683/H694 F2FS statistics and dirty-info recovery

Target stock kernel: Linux 4.14.141+ / X683-H694.

This document records the current binary-derived reconstruction of the F2FS statistics area and the dirty-segment accounting path. Public F2FS source is used only as a structural reference; the stock X683 binary remains authoritative.

## 1. `stat_info` pointer

The stock `f2fs_gc()` loads a pointer from `sbi + 0x568` and uses the resulting object for F2FS GC statistics/accounting. The surrounding `f2fs_sb_info` layout matches the older `CONFIG_F2FS_STAT_FS` generation containing `stat_info` followed by the statistics counters.

Current interpretation:

```text
sbi + 0x568 = struct f2fs_stat_info *
```

Confidence: **high**.

## 2. Historical post-`stat_info` layout fingerprint

With `META_MAX == 4` and normal AArch64 alignment, the matching older F2FS layout gives:

```text
0x568  stat_info
0x570  meta_count[4]
0x580  segment_count[2]
0x588  block_count[2]
0x590  inplace_count
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

These names remain **structural candidates** until the X683 binary directly distinguishes each member.

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
add     w8, w11, w10
add     w8, w8, w12
add     w8, w8, w10
add     w8, w8, w11
add     w8, w8, w9
```

The pointer chain is:

```text
x19 = sbi
sbi + 0x80 -> sm_info
sm_info + 0x10 -> dirty_info
```

and the six values read from `dirty_info + 0x68..0x7c` are accumulated together.

This strongly matches the historical F2FS `nr_dirty[]` six-entry per-type dirty-segment accounting array.

Current conclusion:

```text
dirty_info + 0x68  nr_dirty[0]  candidate
 dirty_info + 0x6c  nr_dirty[1]  candidate
 dirty_info + 0x70  nr_dirty[2]  candidate
 dirty_info + 0x74  nr_dirty[3]  candidate
 dirty_info + 0x78  nr_dirty[4]  candidate
 dirty_info + 0x7c  nr_dirty[5]  candidate
```

Confidence: **high structural match**, but the exact semantic names of the six indices still require a second independent X683 path.

## 4. `0x5d4..0x5dc`

The structural source fingerprint remains:

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

## 5. Reconstruction rule

Do not replace these offsets with normal structure members in the reconstructed source until the exact X683-era `f2fs_sb_info` and `f2fs_stat_info` layouts are proven.

The working order is:

```text
stock disassembly
    -> pointer chain
    -> exact byte offset
    -> historical structure correlation
    -> independent second call site
    -> promoted field name
```
