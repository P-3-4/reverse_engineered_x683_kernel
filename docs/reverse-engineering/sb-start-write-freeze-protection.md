# X683/H694 state-3 `0x1eca60` / freeze-protection resolution

Source: stock X683/H694 Image, SHA-256 `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`.

## `0x1eca60`

The function is the compiler-emitted form of the superblock freeze-protection helper corresponding to:

```c
int __sb_start_write(struct super_block *sb, int level, bool wait);
```

Its recovered interface is:

```text
x0 = super_block *sb
w1 = level
w2 = wait
```

It derives the `sb->s_writers.rw_sem[level - 1]` object, disables preemption, increments the per-CPU reader counter, and selects the blocking/nonblocking path from `wait`.

The caller at `0x3773f4` passes:

```text
sb = *(sbi + 0x0)
level = 1
wait = 0
```

Therefore this exact detector operation is a **nonblocking freeze-protection acquisition** equivalent to:

```c
__sb_start_write(sb, SB_FREEZE_WRITE, false);
```

A return value of zero takes the detector's alternate/retry path. A nonzero return permits the subsequent GC-policy/lock sequence.

## `0x1ec9e4`

The adjacent helper is the matching superblock writer-release operation:

```c
__sb_end_write(sb, level);
```

The X683 detector calls it at `0x377438` with `level = 1`, after the GC policy path completes.

## Detector consequence

The state-2/GC path is therefore freezer-aware before entering the vendor GC policy:

```text
check runtime guards
    |
    +-- __sb_start_write(sb, 1, false)
    |      |
    |      +-- fail -> alternate/retry path
    |      |
    |      +-- success
    |             |
    |             +-- mutex_trylock(gc-related object)
    |             +-- vendor F2FS policy gate 0x366cd4
    |             +-- __sb_end_write(sb, 1)
```

This is kernel/VFS freeze-protection logic, not a proprietary controller primitive.

## Confidence

High:
- `0x1eca60` is the superblock freeze-protection acquisition helper used with `(level=1, wait=0)`.
- `0x1ec9e4` is its matching release helper.
- The detector's `+0x20` state gate and the `0xcc774` task predicate belong to the same Linux freezer/write-freeze control family.

Reference: Android/common 4.14 `fs/super.c` defines `__sb_start_write()` in terms of `percpu_down_read()` / `percpu_down_read_trylock()` and `__sb_end_write()` in terms of `percpu_up_read()`.