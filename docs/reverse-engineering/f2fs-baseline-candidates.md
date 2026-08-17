# X683/H694 F2FS historical baseline candidates

## Result

The strongest public ancestor class identified so far is the Android/common Linux 4.14 F2FS generation represented by the `ASB-2018-11-05_4.14` history and its associated upstream F2FS maintenance commits.

This is a structural ancestor candidate, not a claim that X683 used Android/common directly.

## Why this generation matches

The historical tree contains the same GC-manager sequence seen in X683:

```text
gc_mutex
gc_thread
cur_victim_sec
gc_mode
next_victim_seg[2]
skipped_atomic_files[2]
skipped_gc_rwsem
gc_pin_file_threshold
max_victim_search
migration_granularity
stat_info
```

It also contains the four-argument GC ABI:

```c
f2fs_gc(sbi, sync, background, segno);
```

The Android/common 2018 maintenance history includes the background-GC skip-accounting change that adds:

```c
stat_inc_bggc_count(sbi);
stat_io_skip_bggc_count(sbi);
stat_other_skip_bggc_count(sbi);
```

and the corresponding `bg_gc`, `io_skip_bggc`, and `other_skip_bggc` statistics fields.

## Strong commit-level anchor

The Android/common 4.14 history identifies the background-GC statistics change as:

```text
c95f10ed9fe0
f2fs: add to account skip count of background GC
```

The associated Android/common history also contains the later 4.14 F2FS merge containing this feature set.

## X683 comparison

The stock X683 layout independently gives:

```text
0x508  gc_mutex
0x528  gc_thread
0x530  cur_victim_sec
0x534  gc_mode
0x538  next_victim_seg[2]
0x540  skipped_atomic_files[2]
0x550  skipped_gc_rwsem
0x558  gc_pin_file_threshold
0x560  max_victim_search
0x564  migration_granularity
0x568  stat_info
0x5d4  bg_gc                  candidate
0x5d8  io_skip_bggc           candidate
0x5dc  other_skip_bggc        candidate
```

The structural correspondence is strong enough to use this historical family as the first comparison baseline.

## Why it is not yet the final source

The stock X683 kernel was built in 2021 and contains MediaTek and Transsion modifications. Therefore the correct reconstruction must compare machine code rather than simply import the Android/common snapshot.

Required comparison files:

```text
fs/f2fs/f2fs.h
fs/f2fs/gc.c
fs/f2fs/debug.c
fs/f2fs/segment.c
fs/f2fs/super.c
```

## Reconstruction procedure

```text
Android/common 4.14 candidate
        +
X683 binary structure offsets
        +
X683 GC call/control flow
        +
MediaTek differences
        +
Transsion differences
        =
X683-specific reconstructed F2FS
```

The stock binary remains authoritative whenever a public source differs from observed behavior.
