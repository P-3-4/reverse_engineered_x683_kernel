# X683/H694 F2FS `stat_info` Binary Recovery

Target: Infinix X683/H694, MT6768, Linux 4.14.141+.

## 1. Direct pointer recovery

Stock code at Image offset `0x350c18` loads:

```asm
ldr x8, [x20, #0x568]
```

with `x20 = struct f2fs_sb_info *`.

The resulting `x8` is then used as the base of statistics fields. Therefore:

```text
sbi + 0x568 = struct f2fs_stat_info *
```

This is binary-confirmed.

## 2. Direct `stat_info` member updates

At Image offset `0x350c28` the stock kernel performs:

```asm
ldr w10, [x8, #0x18c]
ldr w11, [x8, #0x194]
ldr w12, [x8, #0x19c]
...
add w9, w10, #0x1
add w10, w11, #0x1
...
str w9,  [x8, #0x18c]
str w10, [x8, #0x194]
add w11, w12, w11
str w11, [x8, #0x19c]
```

This independently proves three `stat_info` members are live counters.

Historical 4.14 F2FS structure ordering matches them exactly:

```text
0x188  tot_segs
0x18c  node_segs
0x190  data_segs
0x194  free_segs
0x198  free_secs
0x19c  bg_node_segs
0x1a0  bg_data_segs
```

Because the X683 binary independently identifies `0x18c`, `0x194`, and `0x19c` as incremented fields with those semantics, the surrounding six-field sequence is now strongly established:

```text
STAT + 0x188  tot_segs       high confidence
STAT + 0x18c  node_segs      confirmed
STAT + 0x190  data_segs      high confidence
STAT + 0x194  free_segs      confirmed
STAT + 0x198  free_secs      high confidence
STAT + 0x19c  bg_node_segs   confirmed
STAT + 0x1a0  bg_data_segs   high confidence
```

## 3. Independent statistics-copy path

A stock statistics/debug path at approximately Image `0x375e18` loads the `stat_info` pointer and copies SBI statistics into it.

Observed direct relationships:

```text
SBI offset       STAT destination
---------------------------------
0x5a8            +0x38
0x5b0            +0x40
0x5a0            +0x48
0x598            +0x58
0x3a4            +0x60
0x3b8            +0x64
0x3bc            +0x68
0x434            +0x6c
0x428            +0x70
0x438            +0x74
0x440            +0x78
0x42c            +0x7c
0x430            +0x80
0x43c            +0x84
0x5e0            +0x88
0x5e4            +0x8c
0x424            +0x90
0x5e8            +0x94
0x5c4            +0x118
0x5c8            +0x120
0x5cc            +0x11c
0x5d0            +0x124
0x45c            +0xd4
0x458            +0xd8
0x444            +0xc0
0x448            +0xc4
0x44c            +0xc8
0x450            +0xcc
0x454            +0xd0
```

The first part is especially strong because the destination offsets correspond exactly to the known historical `f2fs_stat_info` ordering.

## 4. Confirmed historical-layout anchors inside `stat_info`

The matching 4.14-era structure has:

```text
+0x38 hit_largest
+0x40 hit_cached
+0x48 hit_rbtree
+0x50 hit_total
+0x58 total_ext
...
+0x168 node_pages
+0x16c meta_pages
+0x170 compress_pages          (revision-dependent)
+0x174 compress_page_hit       (revision-dependent)
+0x178 prefree_count
+0x17c call_count
+0x180 cp_count
+0x184 bg_cp_count
+0x188 tot_segs
+0x18c node_segs
+0x190 data_segs
+0x194 free_segs
+0x198 free_secs
+0x19c bg_node_segs
+0x1a0 bg_data_segs
+0x1a4 tot_blks
+0x1a8 data_blks
+0x1ac node_blks
+0x1b0 bg_data_blks
+0x1b4 bg_node_blks
```

The X683 binary's live accesses at `0x18c`, `0x194`, and `0x19c` therefore anchor the segment-statistics block.

Public historical F2FS source shows the same field ordering and the same four-argument GC API. citeturn807903search3turn807903search5

## 5. Important SBI finding from the copy path

The statistics-copy path proves that several post-`stat_info` SBI fields are real members of `struct f2fs_sb_info`, because they are read directly from `sbi` and copied into the separate statistics object.

In particular:

```text
sbi + 0x5e0
sbi + 0x5e4
sbi + 0x5e8
```

are distinct SBI counters and are **not** `stat_info` offsets.

Their exact source names require semantic identification from additional call sites, but the binary proves their existence and use.

## 6. Current SBI statistics region

The earlier structural region is now better constrained:

```text
sbi + 0x568  stat_info *       confirmed
sbi + 0x570  meta_count[]      strong structural candidate
sbi + 0x580  segment_count[]   strong structural candidate
sbi + 0x588  block_count[]     strong structural candidate
sbi + 0x590  inplace_count     strong structural candidate
sbi + 0x598  total_hit_ext     strong / directly mirrored into stat_info
sbi + 0x5a0  read_hit_rbtree   strong / directly mirrored
sbi + 0x5a8  read_hit_largest  strong / directly mirrored
sbi + 0x5b0  read_hit_cached   strong / directly mirrored
sbi + 0x5b8  inline_xattr      structural candidate
sbi + 0x5bc  inline_inode      structural candidate
sbi + 0x5c0  inline_dir        structural candidate
sbi + 0x5c4  aw_cnt            strong / directly mirrored
sbi + 0x5c8  vw_cnt            strong / directly mirrored
sbi + 0x5cc  max_aw_cnt        strong / directly mirrored
sbi + 0x5d0  max_vw_cnt        strong / directly mirrored
sbi + 0x5d4  bg_gc             confirmed elsewhere by direct increment
sbi + 0x5d8  io_skip_bggc      very high confidence
sbi + 0x5dc  other_skip_bggc   high confidence
```

Historical F2FS source independently identifies `io_skip_bggc` and `other_skip_bggc` as SBI statistics and defines the same four-argument `f2fs_gc()` ABI. citeturn807903search3turn807903search5

## 7. Evidence rule

The historical structure is being used to identify members only where its ordering agrees with independent X683 machine-code behavior. Historical offsets are not treated as authoritative by themselves.

The `stat_info` object remains a separately allocated object pointed to by `sbi + 0x568`.

## 8. Next binary targets

1. Identify the exact semantic names of `sbi + 0x5e0`, `0x5e4`, and `0x5e8`.
2. Recover the `stat_info` region between `0x88` and `0x118` using the same statistics-copy function plus update sites.
3. Map the remaining SBI statistics fields around `0x5b8..0x5dc` and validate them through their update primitives.
4. Continue the full `f2fs_gc()` control-flow reconstruction and compare it against the closest 4.14 baseline.
