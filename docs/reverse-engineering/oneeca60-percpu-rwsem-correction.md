# X683/H694 `0x1eca60` helper correction

Source: stock `boot(8).img`, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.

## Correction

The earlier identification of `0x1eca60` as `__sb_start_write()` was incorrect.

Direct stock disassembly shows:

```text
0x1eca60:
    preempt/task read-count bookkeeping
    compute base + (index << 7)
    inspect semaphore/read-state field at +0x1e0
    update current-CPU counter
    undo bookkeeping and return
    or enter slowpath when the semaphore is blocked
```

This is the `percpu_rwsem` / `percpu_down_read` family of kernel machinery.

The companion function beginning at `0x1ecad8` is the trylock-returning variant and returns a boolean result.

The detector call site previously discussed passes:

```c
helper(sbi_or_object, 1, 0);
```

The exact vendor-side object type is still unresolved, but the helper itself is kernel `percpu_rwsem` machinery, not the superblock writer API.

## Correct superblock writer helper

`0x3412c8` is the superblock writer-start path. Its body accesses the superblock `s_writers.rw_sem` area and matches the 4.14-era `__sb_start_write()` implementation family.

The X683 vendor wrapper invokes it as:

```c
__sb_start_write(sb, 1, ...)
```

The exact third-argument form is version/build dependent; the raw call site does not justify claiming a three-argument prototype from this call alone.

## Consequence

Remove the old statement:

```text
0x1eca60 = __sb_start_write
```

Use instead:

```text
0x1eca60 = percpu_rwsem/percpu_down_read lineage
0x1ecad8 = percpu_rwsem trylock lineage
0x3412c8 = __sb_start_write lineage
```
