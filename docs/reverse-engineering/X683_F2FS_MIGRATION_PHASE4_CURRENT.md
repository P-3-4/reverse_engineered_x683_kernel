# X683/H694 F2FS Phase-4 Migration — Current State

## Authority

Stock X683/H694 binary is authoritative. Historical Linux 4.14 F2FS is correspondence only.

## Confirmed anchors

```text
f2fs_gc             ~0x3503a8
tran_gc_thread_func ~0x376ed0
tran_do_f2fs_gc     ~0x37ada8
```

Phase-4 migration anchors:

```text
0x36bf78  ra_data_block()-type post-read helper, high confidence
0x36bea0  generic page/object state operation; NOT currently move_data_block/page
0x35443c  unresolved X683 migration orchestration helper
0x355210  data/dnode block-address update path
0x36be28  f2fs_replace_block()-type wrapper
0x36b4fc  f2fs_do_replace_block()-type primitive, very high confidence
0x373e5c  extent update boundary, very high confidence
```

## `0x35443c` caller

At the phase-4 caller:

```asm
351f44: ldr  w8, [sp,#0x1b4]
351f48: mov  x10,#1
351f4c: sub  x9,x29,#0xd0
351f50: movk x10,#0x800,lsl #32
351f54: sub  x0,x29,#0xd0
351f58: stur x10,[x9,#0x14]
351f5c: stur w8,[x29,#-0xb4]
351f60: bl   0x35443c
```

Therefore the helper receives the local context at `sp-0xd0`, and this call writes:

```text
ctx + 0x14 = 0x0000080000000001
```

The body of `0x35443c` was not available in the incomplete disassembly chunk used during the last pass, so its source identity remains unresolved.

## `sp-0xd0` context

Confirmed access patterns only:

```text
ctx = sp-0xd0
ctx+0x14 = packed value 0x0000080000000001 at phase-4 setup
ctx+0x1c = initialized zero
ctx+0x6c = byte access
ctx+0x6d = halfword access
ctx+0xa0 = synchronization/lock-like object use
ctx+0xe0 = pointer access
```

Earlier claim that `ctx+0x00` was definitively `sbi` is withdrawn pending the actual body of `0x35443c`.

Do not treat the context as ordinary `struct f2fs_io_info` yet.

## Migration architecture

```text
GC victim data
    -> summary/liveness processing
    -> phase-3 preparation / optional ra_data_block()
    -> phase-4 migration context
    -> 0x35443c (unresolved orchestration)
    -> replacement/accounting paths
    -> 0x36be28 / 0x36b4fc
    -> dnode/block-address update
    -> extent update
```

Compiler inlining means historical function boundaries may not exist one-to-one in the X683 Image.

## Important corrections

- `0x35443c` is NOT currently proven to be `f2fs_submit_page_write()`.
- `0x35443c` is NOT equated with `f2fs_submit_merged_write()`.
- `0x36bea0` is NOT currently identified as `move_data_block()` or `move_data_page()`.
- Do not force a linear call chain when branches are compiler-merged.

## Next exact operation

Recover the raw function body:

```text
0x35443c -> function end
```

Then map every context offset and identify:

1. source page/block;
2. destination block allocation;
3. I/O descriptor construction;
4. write submission;
5. replacement invocation;
6. vendor-specific migration behavior.

Only after this should the X683 migration structure be converted into compilable C.
