# X683/H694 `tran_f2fs_gc()` exact wrapper reconstruction

Source: stock `boot.img`, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.

Function range: `0x37ada8..0x37ae94`.

## Exact behavior

The stock function increments the vendor invocation counter at image/global `+0x990`, reads the vendor controller at `+0x998`, and dispatches as follows.

### Controller 0

```c
old_mode = sbi->gc_mode;
force_fg_gc = (sbi->mount_opt.opt >> 14) & 1;
return f2fs_gc(sbi, force_fg_gc, true, NULL_SEGNO);
```

No temporary `gc_mode` change occurs.

### Controller 1

```c
old_mode = sbi->gc_mode;
sbi->gc_mode = 2;
ret = f2fs_gc(sbi, force_fg_gc, true, NULL_SEGNO);
sbi->gc_mode = old_mode;
return ret;
```

### Controller 2

```c
old_mode = sbi->gc_mode;
sbi->gc_mode = 3;
ret = f2fs_gc(sbi, force_fg_gc, true, NULL_SEGNO);
sbi->gc_mode = old_mode;
return ret;
```

The matching 4.14 F2FS source identifies mount-option bit 14 as `F2FS_MOUNT_FORCE_FG_GC`, so the recovered first argument is the FORCE_FG_GC bit. citeturn296107search4

## Vendor mode meanings

Nearby stock strings are:

```text
"gc mode is COST"
"gc mode is URGENT"
"gc mode is GREEDY"
```

The wrapper therefore uses the vendor mode values:

```text
1 = COST
2 = URGENT
3 = GREEDY
```

This is vendor `sbi->gc_mode` state, not the controller state itself.

Controller-to-mode mapping:

```text
controller 0 -> preserve current gc_mode
controller 1 -> force gc_mode = 2 (URGENT)
controller 2 -> force gc_mode = 3 (GREEDY)
```

## Call ABI

The direct call is:

```c
f2fs_gc(sbi, force_fg_gc, true, NULL_SEGNO);
```

where:

```text
sync       = mount_opt bit 14
background = true
segno      = 0xffffffff / NULL_SEGNO
```

The decompressed stock image contains the expected four-argument F2FS GC call at `0x3503a8`.

## Important separation

`0x37ada8` is the actual Transsion `tran_f2fs_gc()` controller wrapper.

`0x366cd4` is a separate F2FS/GC policy routine that invokes lower GC machinery; it must not be merged with the controller wrapper.

## Confidence

High / binary-direct:

- controller read at `Image+0x1a13998`;
- invocation counter at `Image+0x1a13990`;
- controller 0/1/2 branches;
- temporary `gc_mode` values 2 and 3;
- restore-after-call behavior;
- `f2fs_gc()` arguments;
- `NULL_SEGNO`;
- FORCE_FG_GC bit 14;
- vendor mode strings COST/URGENT/GREEDY.

The remaining task is to integrate this exact wrapper with the already reconstructed detector and the stock `f2fs_gc()` body.
