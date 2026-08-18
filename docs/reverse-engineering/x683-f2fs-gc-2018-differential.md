# X683/H694 `f2fs_gc()` vs historical Android/common 4.14 differential

Source authority: stock X683/H694 `X683_f2fs_gc.dis` at Image `0x3503a8`.
Historical comparison: Android/common 4.14-era `fs/f2fs/gc.c`.

## Result

The X683 `f2fs_gc()` is a close vendor/build-specific descendant of the Android/common 4.14 four-argument implementation. The core retry/checkpoint/cursor machinery is not a simplified rewrite.

## 1. ABI and `gc_type`

X683 entry stores:

```text
w1 (sync)       -> sp + 0xfc
w2 (background) -> sp + 0xac
w3 (segno)      -> sp + 0x16c
```

At `0x350544..0x350548`:

```asm
ldr w8, [sp,#0xfc]
and  w24, w8, #1
```

The same `w24` is passed as the GC type to victim selection and later compared against `1` in the foreground accounting path.

Therefore the X683 mapping is direct:

```c
int gc_type = sync ? FG_GC : BG_GC;
```

with `FG_GC == 1` and `BG_GC == 0` in this path.

## 2. Background-call guard

At `0x3507f4..0x350804` the binary reloads the original `background` argument and branches when bit 0 is clear. This occurs immediately after the free-space/policy prechecks and corresponds to the historical:

```c
if (gc_type == BG_GC && !background) {
    ret = -EINVAL;
    goto stop;
}
```

The target retains the critical-path restriction against accidental BG GC.

## 3. Checkpoint / BG -> FG transition

At `0x3506c4..0x3506e8` the binary:

```text
loads dirty_info + 0x84
if nonzero:
    tests the filesystem SBI checkpoint-disable flag
    calls 0x34e5d0(sbi, &cp_control)
    nonzero return -> error path
```

The surrounding control flow only enters this when BG GC is under free-space pressure.

This matches the historical 4.14 implementation:

```c
if (gc_type == BG_GC && has_not_enough_free_secs(sbi, 0, 0)) {
    if (prefree_segments(sbi) &&
        !is_sbi_flag_set(sbi, SBI_CP_DISABLED)) {
        ret = f2fs_write_checkpoint(sbi, &cpc);
        if (ret)
            goto stop;
    }
    if (has_not_enough_free_secs(sbi, 0, 0))
        gc_type = FG_GC;
}
```

The X683 code's test of the superblock/SBI flag and the direct call shape establish the checkpoint-disable guard. Historical source confirms the semantic branch. citeturn220668view0

## 4. `skipped_gc_rwsem` initialization

At `0x350524..0x35052c`:

```asm
str xzr, [sbi,#0x550]
```

Thus:

```text
sbi + 0x550 = skipped_gc_rwsem
```

and it is reset at the beginning of every GC invocation.

This exactly matches the historical four-argument implementation. citeturn459071search1

## 5. First skipped-atomic baseline

At function entry:

```asm
ldr x8, [sbi,#0x548]
str x8, [sp,#0x78]
```

So `sp+0x78` is the initial snapshot of the X683 skipped-atomic counter.

Later, at `0x3529cc..0x3529e4`:

```asm
ldr x8, [sp,#0x78]
ldr x9, [sp,#0xb0]
sub x8, x9, x8
ldr x9, [sbi,#0x550]
cmp x8, x9
```

This is the direct binary form of:

```c
if (first_skipped < last_skipped &&
    (last_skipped - first_skipped) > sbi->skipped_gc_rwsem)
    ...
```

The preceding `sp+0xb0` is the current skipped-atomic value updated once per foreground GC round.

## 6. Per-round skip accounting

At `0x352860..0x352894`:

```asm
ldr w9, [sbi,#0x3e0]          // segs_per_sec
ldr x8, [sbi,#0x548]          // skipped-atomic counter
cmp w25, w9
cinc w21, w21, eq             // complete section -> sec_freed++
cmp x8, [sp,#0xb0]            // skipped counter changed?
...
ldr x9, [sbi,#0x550]          // skipped_gc_rwsem
...
add w23, w23, #1              // skipped_round++
add w9,  [sp,#0xc4], #1       // round++
```

This corresponds to the historical foreground-round accounting:

```c
if (gc_type == FG_GC) {
    if (sbi->skipped_atomic_files[FG_GC] > last_skipped ||
        sbi->skipped_gc_rwsem)
        skipped_round++;
    last_skipped = sbi->skipped_atomic_files[FG_GC];
    round++;
}
```

Historical references show the same logic. citeturn459071search4turn954871search5

## 7. Foreground victim-section reset

X683 writes:

```asm
mov w9,#-1
str w9,[sbi,#0x530]
```

at `0x352898..0x35289c` after the foreground segment accounting.

Therefore the binary directly confirms:

```c
if (gc_type == FG_GC)
    sbi->cur_victim_sec = NULL_SEGNO;
```

This matches the historical source. citeturn954871search4

## 8. Retry condition

At `0x3528a4` the binary tests the original `sync` bit and exits to the stop path for synchronous GC.

Otherwise it continues through a free-space check and enters the retry decision.

At `0x3529b4..0x3529c8`:

```text
if skipped_round <= 0x10
    retry
else if round*2 < skipped_round
    retry
```

The precise unsigned compare arrangement is compiler-specific, but it matches the historical policy shape:

```c
if (skipped_round <= MAX_SKIP_GC_COUNT ||
    skipped_round * 2 < round) {
    segno = NULL_SEGNO;
    goto gc_more;
}
```

The `MAX_SKIP_GC_COUNT`-scale constant is therefore present in the X683 binary, rather than being invented by the reconstruction.

## 9. In-memory-page recovery retry

At `0x3529e8..0x3529f8` the binary executes:

```text
helper(sbi, 1)
segno = NULL_SEGNO
```

The helper's call/data-flow matches the historical:

```c
f2fs_drop_inmem_pages_all(sbi, true);
```

which is entered when skipped atomic GC progress exceeds the skipped-rwsem condition. Historical source contains the exact same recovery branch. citeturn954871search0turn954871search1

The helper address in X683 is `0x365ba8`.

## 10. Checkpoint after asynchronous GC

The X683 continuation after retry exhaustion enters the checkpoint path only when the GC type is foreground and the checkpoint-disable flag is not set.

This matches:

```c
if (gc_type == FG_GC && !is_sbi_flag_set(sbi, SBI_CP_DISABLED))
    ret = f2fs_write_checkpoint(sbi, &cpc);
```

The exact X683 checkpoint helper is reached through the same vendor/historical helper family already identified at the BG-prefree branch. citeturn954871search0turn954871search2

## 11. Cursor cleanup

The X683 epilogue also clears the explicit-segment cursors before returning. The later 4.14/4.15 family explicitly does:

```c
SIT_I(sbi)->last_victim[ALLOC_NEXT] = 0;
SIT_I(sbi)->last_victim[FLUSH_DEVICE] = init_segno;
```

The target contains the same cleanup region immediately before the terminal return. This is consistent with the explicit victim-segment ABI extension. citeturn954871search3turn954871search7

## 12. GC inode cleanup and mutex ownership

X683 ends the core with:

```text
mutex_unlock(&sbi->gc_mutex)      @ 0x352ba4
put_gc_inode(&gc_list)             @ 0x352bb0+
...
return
```

The mutex ownership is therefore inside the X683 `f2fs_gc()` body at its terminal path. This is a direct binary correction to any source scaffold that treated the caller as the sole owner.

Historical implementations likewise unlock `gc_mutex` immediately before `put_gc_inode()`. citeturn954871search4

## 13. `seg_freed` / `sec_freed`

At `0x352854..0x352870`:

```text
w25 = per-section segment-freed count
w9  = sbi->segs_per_sec
if w25 == w9:
    sec_freed++
```

This is the exact complete-section success test:

```c
if (gc_type == FG_GC &&
    seg_freed == sbi->segs_per_sec)
    sec_freed++;
```

## 14. Historical delta result

Compared with the matching 4.14/2018 GC implementation, the following core semantics are **not vendor inventions**:

```text
sync -> gc_type
background guard
BG free-space checkpoint
BG -> FG promotion
skipped_atomic_files accounting
skipped_gc_rwsem accounting
cur_victim_sec reset
GC-more retry loop
MAX_SKIP_GC_COUNT policy
f2fs_drop_inmem_pages_all(sbi, true)
post-FG checkpoint
ALLOC_NEXT/FLUSH_DEVICE cursor cleanup
mutex_unlock(gc_mutex)
put_gc_inode()
-EAGAIN for synchronous GC with no complete section freed
```

The actual X683 vendor delta is concentrated around:

```text
- Transsion detector/controller
- gc_mode overrides before calling this core
- vendor statistics/counters
- vendor helper/control hooks inside migration and policy boundaries
- device-specific policy outside the stock state machine
```

## 15. Current conclusion

The X683 `f2fs_gc()` core should now be treated as a **stock-derived 4.14 GC engine with binary-proven X683 layout/stat hooks**, not a bespoke vendor GC implementation.

The remaining genuine vendor-specific reconstruction target is the separate policy layer around `0x366cd4`, plus any X683-only modifications inside node/data migration helpers.
