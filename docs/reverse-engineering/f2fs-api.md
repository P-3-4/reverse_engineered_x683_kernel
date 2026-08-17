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

## Public comparison

Historical public F2FS trees demonstrate that the API evolved through several incompatible forms. A 4.14-era implementation exposes a four-argument form containing `sync`, `background`, and `segno`, while older trees also contain a three-argument form with a different semantic layout. Later kernels add `force` and eventually consolidate the arguments into `struct f2fs_gc_control`.

Therefore a three-argument call alone is not enough to select a public source tree by signature. The X683 call-site and the behavior around `sbi + 0x534` remain authoritative.

## Current conclusion

- `sbi + 0x4b8`: high-confidence `mount_opt.opt`.
- bit 14 of `0x4b8`: high-confidence correlation with `F2FS_MOUNT_FORCE_FG_GC` / foreground-GC behavior.
- `sbi + 0x534`: high-confidence GC-mode/state word; exact vendor-era member identity still needs source/assembly cross-validation.
- X683 `f2fs_gc` call: **three arguments are the current stock reconstruction**; third-argument semantic is vendor-specific and remains subject to final call-site verification.

Do not replace this ABI with a newer public F2FS prototype merely to make a source tree compile. The matching historical `gc.c` must be selected from structure layout, call behavior, and additional X683 binary evidence together.
