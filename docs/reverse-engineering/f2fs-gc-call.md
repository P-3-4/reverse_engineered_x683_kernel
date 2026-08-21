# X683/H694 F2FS GC call reconstruction

## Stock binary evidence

The recovered X683/H694 binary proves that `tran_do_f2fs_gc()` calls the stock `f2fs_gc()` using four arguments:

```c
f2fs_gc(sbi,
        ((sbi->mount_opt.opt >> 14) & 1),
        true,
        NULL_SEGNO);
```

X683 addresses:

```text
tran_do_f2fs_gc  0xffffff92d0dfada8
f2fs_gc          0xffffff92d0dd03a8
```

The three call sites inside the vendor wrapper are at wrapper offsets `+0x58`, `+0x94`, and `+0xc4`.

## `gc_type` / `gc_mode`

`global + 0x998` is persistent vendor `gc_type`, accepted in the domain `0..2`.

```text
gc_type 0 -> preserve sbi + 0x534
gc_type 1 -> temporary sbi + 0x534 = 2
gc_type 2 -> temporary sbi + 0x534 = 3
```

The original `sbi + 0x534` value is restored after the stock collector returns.

## Current conclusion

- `sbi + 0x4b8`: high-confidence `mount_opt.opt`.
- bit 14 of `0x4b8`: directly extracted as the stock `sync` argument.
- `sbi + 0x508`: GC mutex; vendor worker uses `mutex_trylock()` before admission.
- `sbi + 0x534`: stock GC mode/state word; vendor writes are temporary.
- Exact X683 `f2fs_gc` ABI: **proven four-argument**, not unresolved.
- No `tran_*` downstream call was found between `tran_do_f2fs_gc()` and the stock victim/migration path.

## Rule

Do not change the reconstructed prototype merely to match a newer/public source tree. The X683 call-site/register evidence is authoritative. Historical 4.14 Android/Linux source is used only for naming and semantic comparison.
