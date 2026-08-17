# X683/H694 F2FS source fingerprint

Target stock kernel: Linux 4.14.141+ / X683-H694.

This document records the historical-source fingerprint established from the stock binary plus public F2FS references. Public source is a reference only; the stock binary remains authoritative.

## 1. GC ABI fingerprint

The stock X683 Transsion wrapper calls:

```c
f2fs_gc(sbi,
        (sbi->mount_opt.opt >> 14) & 1,
        true,
        NULL_SEGNO);
```

Therefore the reconstruction requires the historical four-argument form:

```c
int f2fs_gc(struct f2fs_sb_info *sbi,
            bool sync,
            bool background,
            unsigned int segno);
```

Historical F2FS source contains this exact four-argument prototype.

## 2. GC-manager structure fingerprint

The X683 binary matches the historical ordering:

```text
sbi + 0x508  gc_mutex
sbi + 0x528  gc_thread
sbi + 0x530  cur_victim_sec
sbi + 0x534  gc_mode
sbi + 0x538  next_victim_seg[2]
sbi + 0x540  skipped_atomic_files[2]
sbi + 0x550  skipped_gc_rwsem
sbi + 0x558  gc_pin_file_threshold
sbi + 0x560  max_victim_search
sbi + 0x564  migration_granularity
sbi + 0x568  stat_info
```

`gc_mutex @ 0x508` is now directly binary-confirmed: the stock Transsion GC thread materializes `sbi + 0x508` and passes it through the mutex trylock/unlock path. This supersedes the earlier unresolved status of `gc_mutex`.

The remaining GC fields are independently supported by stock initialization/access patterns and historical source ordering.

Historical Android/common 4.14 F2FS is especially relevant: the ASB-2018-11-05_4.14 history shows the same GC-manager field family and ordering. This does **not** prove X683 was built from Android/common; it identifies a strong structural ancestor.

## 3. Post-`stat_info` fingerprint

The X683 stock configuration has `CONFIG_F2FS_STAT_FS=y`.

The matching older F2FS generation gives the following structural candidate layout after `stat_info`:

```text
0x568  stat_info pointer
0x570  meta_count[4]
0x580  segment_count[2]
0x588  block_count[2]
0x590  inplace_count
0x594  AArch64 alignment padding
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

The historical Android/common 4.14 maintenance history provides a strong match for the statistics additions, including `meta_count[META_MAX]`, `io_skip_bggc`, and `other_skip_bggc`.

## 4. Current confidence of `0x5d4–0x5dc`

```text
0x5d4  bg_gc             strong structural candidate
0x5d8  io_skip_bggc      strong structural candidate
0x5dc  other_skip_bggc   strong structural candidate
```

These names remain **structural candidates**, not final binary-confirmed fields. The remaining proof step is to locate stock X683 call sites that distinguish their increment/read semantics.

## 5. Historical baseline narrowed

The source search is now narrowed from generic `Linux 4.14` to the **older Android/common 4.14 F2FS generation**, with the ASB-2018-11-05_4.14 history as a particularly strong candidate baseline.

The reason is the combination of fingerprints rather than kernel version alone:

```text
four-argument f2fs_gc()
gc_mutex / gc_thread family
skipped_gc_rwsem
gc_pin_file_threshold
max_victim_search
migration_granularity
stat_info statistics block
bg_gc / io_skip_bggc / other_skip_bggc
reservation-field additions
```

This does **not** prove that X683 was built from Android/common. Transsion/MediaTek changes and later backports are still expected. The proper working model is:

```text
Android/common 4.14 historical F2FS
            ↓
      strong structural ancestor
            ↓
       MTK/Transsion changes
            ↓
       X683 stock binary
```

The exact ancestor still requires function-level comparison of:

```text
f2fs.h
gc.c
gc.h
segment.c
super.c
debug.c
```

against the stock implementation.

## 6. X683 binary authority

The stock X683 kernel exposes F2FS source-path strings for `f2fs.h`, `segment.h`, `segment.c`, and `super.c`.

Known stock configuration includes:

```text
CONFIG_F2FS_FS=y
CONFIG_F2FS_STAT_FS=y
CONFIG_F2FS_TRAN_GC=y
```

The binary remains the final authority for every vendor-specific divergence from the public baseline.
