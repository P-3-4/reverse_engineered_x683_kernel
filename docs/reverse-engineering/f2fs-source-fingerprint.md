# X683/H694 F2FS source fingerprint

Target stock kernel: Linux 4.14.141+ / X683-H694.

This document records the historical-source fingerprint currently established from the stock binary plus public F2FS references. Public source is a reference only; the stock binary remains authoritative.

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

Public historical Linux/Android-common F2FS trees contain this exact prototype and GC implementation pattern. The older implementation also resets `cur_victim_sec` after foreground GC and tracks `skipped_gc_rwsem` and `skipped_atomic_files` in the GC manager.

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

`gc_mutex @ 0x508` is directly proven by the stock Transsion GC thread materializing `sbi + 0x508` and passing it to `mutex_trylock()` / the corresponding unlock path. The remaining fields are independently supported by stock initialization/access patterns and historical source ordering.

## 3. Post-`stat_info` fingerprint

The X683 stock configuration has `CONFIG_F2FS_STAT_FS=y`.

In the historical F2FS generation that matches the X683 GC-manager sequence, `struct f2fs_sb_info` continues after `stat_info` as follows when `META_MAX == 4` and normal 64-bit alignment is respected:

```text
0x568  stat_info pointer
0x570  atomic_t meta_count[4]
0x580  unsigned int segment_count[2]
0x588  unsigned int block_count[2]
0x590  atomic_t inplace_count
0x594  alignment padding
0x598  atomic64_t total_hit_ext
0x5a0  atomic64_t read_hit_rbtree
0x5a8  atomic64_t read_hit_largest
0x5b0  atomic64_t read_hit_cached
0x5b8  atomic_t inline_xattr
0x5bc  atomic_t inline_inode
0x5c0  atomic_t inline_dir
0x5c4  atomic_t aw_cnt
0x5c8  atomic_t vw_cnt
0x5cc  atomic_t max_aw_cnt
0x5d0  atomic_t max_vw_cnt
0x5d4  int bg_gc
0x5d8  unsigned int io_skip_bggc
0x5dc  unsigned int other_skip_bggc
```

This is a strong source fingerprint because later F2FS revisions inserted compression/statistics fields in this region, which would move the GC statistics offsets. The X683 offsets instead line up with the older layout containing `aw_cnt`, `vw_cnt`, `max_aw_cnt`, `max_vw_cnt`, then `bg_gc`, `io_skip_bggc`, and `other_skip_bggc`.

Public Android/common and Linux F2FS references show this exact older ordering. Examples include Android/common `fs/f2fs/f2fs.h` revisions with:

```c
atomic_t aw_cnt;
atomic_t vw_cnt;
atomic_t max_aw_cnt;
atomic_t max_vw_cnt;
int bg_gc;
unsigned int io_skip_bggc;
unsigned int other_skip_bggc;
```

and the same GC-manager sequence containing `gc_mutex`, `gc_thread`, `cur_victim_sec`, `gc_mode`, `next_victim_seg`, `skipped_atomic_files`, `skipped_gc_rwsem`, `gc_pin_file_threshold`, `max_victim_search`, `migration_granularity`, and `stat_info`.

## 4. Current confidence of `0x5d4–0x5dc`

```text
0x5d4  bg_gc             strong structural candidate
0x5d8  io_skip_bggc      strong structural candidate
0x5dc  other_skip_bggc   strong structural candidate
```

These names are **not yet binary-confirmed**. The remaining proof step is to locate stock X683 call sites that distinguish the increment/read semantics of the three counters.

The important result is that the search space is now sharply constrained: a candidate F2FS source revision that includes later compression-stat fields before `bg_gc` cannot reproduce the X683 offsets without additional vendor changes, while the older layout reproduces them directly.

## 5. Historical baseline direction

The best public baseline candidates are now late-4.14 F2FS trees containing:

```text
four-argument f2fs_gc()
gc_mutex
skipped_atomic_files[2]
skipped_gc_rwsem
gc_pin_file_threshold
max_victim_search
migration_granularity
stat_info
META_MAX == 4
aw_cnt/vw_cnt/max_aw_cnt/max_vw_cnt
bg_gc/io_skip_bggc/other_skip_bggc
```

This is a substantially better fingerprint than selecting a source tree from `Linux 4.14` alone.

The final ancestor still requires function-level comparison of:

```text
f2fs.h
gc.c
gc.h
segment.c
super.c
```

against the stock X683 implementation.

## 6. X683 binary authority

The stock X683 kernel was built from a `kernel-4.14` tree and exposes the F2FS source-path strings:

```text
../../../../../../kernel-4.14/fs/f2fs/f2fs.h
../../../../../../kernel-4.14/fs/f2fs/segment.h
../../../../../../kernel-4.14/fs/f2fs/segment.c
../../../../../../kernel-4.14/fs/f2fs/super.c
```

Its relevant configuration includes:

```text
CONFIG_F2FS_FS=y
CONFIG_F2FS_STAT_FS=y
CONFIG_F2FS_TRAN_GC=y
```

The stock binary therefore remains the final authority for any vendor-specific divergence from public F2FS.
