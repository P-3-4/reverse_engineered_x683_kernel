# X683/H694 `tran_f2fs_gc()` policy-wrapper deep pass

Source: stock `boot.img`, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.

Target: decompressed Image `0x366cd4..0x3671c4`.

## Identity

The function at `0x366cd4` is the Transsion GC policy wrapper reached from the detector at `0x37742c`:

```c
__sb_start_write(sbi->sb, 1, false);
tran_f2fs_gc(sbi);
__sb_end_write(sbi->sb, 1);
```

The surrounding strings contain the literal `tran_f2fs_gc`, and the call site matches the reconstructed one-argument vendor wrapper rather than the stock four-argument `f2fs_gc()`.

## Entry guards

```text
sbi + 0x48 bit 3 set -> return

check helper 0x35cc18(sbi, 4)
  false -> helper 0x373108(sbi, 0x80)

check helper 0x35cc18(sbi, 1)
  false -> helper 0x35d22c(sbi, 455)

check helper 0x35cc18(sbi, 0)
  true  -> helper 0x362c40(sbi, 0, 0)
  false -> helper 0x363288(sbi, 3640)
```

The three `0x35cc18` calls are retained as opaque policy predicates. Their exact symbolic names are not required to preserve the wrapper branch structure.

## GC mode branch

The wrapper reads:

```c
mode = sbi->gc_mode; /* +0x534 */
```

`mode == 3` enters the urgent-GC policy path directly.

For the non-urgent path it evaluates the following filesystem counters:

```text
+0x444
+0x448
+0x44c
+0x450
+0x454
+0x458
+0x45c
```

Any active/nonzero guard routes into the common policy/GC path.

If those guards are all clear, the wrapper applies capacity/time guards based on the main/free segment manager and current F2FS counters. The fixed-point percentage constant is the same `0x51EB851F >> 37` sequence already recovered in the detector.

## Common GC-policy path

The wrapper then executes:

```text
0x35cc18(sbi, 1)
    false -> return

0x35cc18(sbi, 3)
    false -> return

compare dirty_info +0x84 against sm_info +0x64
    dirty PRE > reserved/limit -> return

repeat free/main percentage guard
repeat current block/segment guard
repeat elapsed-time guard
```

Only after these gates succeed does the wrapper enter the actual data-moving/GC machinery.

## Final execution tail

When the mount option byte at `sbi + 0x4b9` has bit 7 set, the wrapper performs an additional stack/object preparation sequence through:

```text
0x3e1014
0x34e224(sbi, 1)
0x3e1558
```

Then it calls:

```c
helper_341250(sbi->sb, 1);
```

Finally it increments the vendor GC statistic at:

```text
sbi->stat_info + 0x16c
```

## Important separation

The stock four-argument F2FS implementation remains at `0x3503a8`:

```c
f2fs_gc(sbi, sync, background, segno);
```

Therefore:

```text
0x366cd4 = vendor `tran_f2fs_gc()` policy/orchestration
0x3503a8 = stock `f2fs_gc()` implementation
```

The vendor wrapper decides whether/how to reach the underlying GC mechanisms; it is not a replacement for the stock migration/victim-selection body itself.

## Current confidence

High:
- `0x366cd4` is the vendor one-argument GC wrapper called by the detector;
- `0x3503a8` is the stock four-argument `f2fs_gc()` body;
- `gc_mode +0x534` is explicitly consumed by the vendor wrapper;
- the wrapper has distinct normal/urgent policy branches;
- the wrapper reuses the recovered percentage/time guards;
- successful completion updates vendor stat `+0x16c`.

Still unresolved:
- exact semantic names/bodies of helper `0x35cc18` policy tests;
- exact semantic names of `0x362c40`, `0x363288`, `0x341250`;
- exact vendor data-flow between those helpers and the named controls (`tran_urgent_gc`, `need_switch_ssr`, etc.).
