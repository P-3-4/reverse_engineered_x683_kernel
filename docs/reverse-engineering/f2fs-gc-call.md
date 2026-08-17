# X683/H694 F2FS GC call reconstruction

## Stock evidence

The recovered X683/H694 binary shows the Transsion wrapper loading bit 14 from the F2FS option word and then calling `f2fs_gc` with `sync` and `force=true`. The wrapper temporarily changes the word at `sbi + 0x534` for `gc_type` 2 or other nonzero values and restores it afterward.

## Public 4.14 comparison

A public MT6768 4.14-era F2FS tree currently exposes a four-argument `f2fs_gc(sbi, sync, force, start_segno)` interface. That does not by itself prove the X683 binary uses the same ABI. The stock call-site must therefore remain authoritative.

## Current conclusion

- `sbi + 0x4b8`: high-confidence `mount_opt.opt`.
- bit 14 of `0x4b8`: high-confidence correlation with `FORCE_FG_GC` / foreground-GC sync behavior.
- `sbi + 0x534`: high-confidence F2FS GC-mode/state word; exact vendor-era member identity remains to be proven from the stock structure.
- Exact `f2fs_gc` prototype: unresolved until call-site register/stack evidence is checked against the stock symbol.

## Rule

Do not change the reconstructed prototype merely to match a newer/public source tree. First prove the X683 call ABI, then select or adapt the matching 4.14 F2FS implementation.
