# X683 F2FS API correlation

## Stock call ABI

The current X683/H694 reverse-engineering record identifies a three-argument call to `f2fs_gc` from the Transsion GC wrapper:

```c
f2fs_gc(sbi, sync, true);
```

The AArch64 call record is treated as:

```text
x0 = sbi
x1 = sync
x2 = true
```

The important point is that the third argument is **not** being assumed to have the same semantic name as a newer public F2FS parameter. The reconstruction currently treats it as the stock wrapper's force/override argument because the surrounding wrapper behavior temporarily changes the GC state word at `sbi + 0x534`.

## Historical public correlation

Android Common 4.14 history is a much better structural baseline than current MT6768 trees. In the 2018 Android 4.14 lineage, `struct f2fs_sb_info` contains the same important sequence recovered from X683: `user_block_count`, `total_valid_block_count`, `discard_blks`, `last_valid_block_count`, `reserved_blocks`, `current_reserved_blocks`, `unusable_block_count`, and `nquota_files`. The same lineage also places `gc_mode` in the GC state area. citeturn9search0turn7search1

The Android Common 4.14 tree merged in March 2019 still exposes the historical four-argument form:

```c
f2fs_gc(sbi, sync, background, segno);
```

and explicitly uses `sbi->gc_mode` in the GC thread. This confirms that the X683 tree is not simply a stock copy of that public revision if the recovered three-argument call is correct. citeturn12search0

A separate 2015 F2FS change established bit `0x00004000` as `F2FS_MOUNT_FORCE_FG_GC` and used that option to make background GC run with foreground semantics. This is the historical origin of the exact bit-14 behavior recovered in X683. citeturn6search0

## Current conclusion

- `sbi + 0x4b8`: high-confidence `mount_opt.opt`.
- bit 14 of `0x4b8`: high-confidence correlation with `F2FS_MOUNT_FORCE_FG_GC` / foreground-GC behavior.
- `sbi + 0x534`: high-confidence GC-mode/state word; public 4.14 history independently confirms `gc_mode` exists in this GC-state region. citeturn7search1
- X683 `f2fs_gc` call: **three arguments are the current stock reconstruction**; third-argument semantic is vendor-specific and remains subject to final call-site verification.

## Baseline selection

The best public source window to investigate next is the **2018–2019 Android Common 4.14 F2FS lineage**, before later 2020 cleanups and feature additions. The public 2019 API is still not the X683 API; it is a structural/reference baseline only.

Do not replace the X683 ABI with a newer public F2FS prototype merely to make a source tree compile. The matching historical `gc.c`, `segment.c`, `segment.h`, and `f2fs.h` must be selected from structure layout, call behavior, and additional X683 binary evidence together.
