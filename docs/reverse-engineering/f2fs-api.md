# X683 F2FS API correlation

A public MT6768 F2FS ancestor at commit `06af20490f7b49387504cbe1500d4cddecc62936` uses the following background-GC call shape in `fs/f2fs/gc.c`:

```c
f2fs_gc(sbi, sync_mode, true, NULL_SEGNO);
```

Its GC thread also uses `sbi->gc_mode`, `sbi->gc_lock`, and `f2fs_balance_fs_bg(sbi, true)`.

This is significant because earlier binary reconstruction notes treated the stock X683 `f2fs_gc` call as a three-argument function. That discrepancy is now an explicit unresolved ABI question rather than something to silently reconcile.

## Required resolution

Use the stock X683 call-site machine code to determine whether the fourth argument register is populated before the branch-and-link. The final `tran_gc.c` prototype must match the stock X683 symbol exactly before it is integrated into `fs/f2fs/gc.c`.

Do not copy a newer Android Common F2FS signature into the X683 tree merely because it is available publicly.
