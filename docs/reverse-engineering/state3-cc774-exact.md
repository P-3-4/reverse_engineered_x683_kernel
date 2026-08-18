# X683/H694 state-3 `0xcc774` / `+0x974` exact evidence

Source: stock `boot.img`, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.

## `+0x974` producer

Function at `0x37acf8` accepts an event number and event-data pointer.

For event `9`:

```c
state = *(u32 *)(event_data + 8);

if (state == 0)
    vendor_global_974 = 1;
else if (state == 4)
    vendor_global_974 = 0;
```

Other states leave the flag unchanged.

When auxiliary vendor state `+0x898` is nonzero, both branches also notify the object at `+0x978` with arguments equivalent to `(3, 1, 0)`.

The callback's public registration/name is not yet proven.

## State-3 consumption

The detector enters state 3 at `0x377494` and reads `+0x974` around the timed-wait/recheck path.

Observed behavior:

```text
+0x974 != 0 -> skip/leave the timed wait path
+0x974 == 0 -> allow the timed wait/re-entry path
                 -> re-read +0x974
                 -> nonzero causes exit from the wait loop
```

Therefore `+0x974` is a producer-driven wait/re-entry gate, not a GC statistic.

## `0xcc774` exact machine-code predicate

Input: current `task_struct *t`.

Recovered control flow:

```c
if (*(u32 *)((char *)t + 0x44) & 0x80008000)
    return 0;

if (*(unsigned long *)t & (1UL << 18))
    return 0;

if (global_byte_28)
    return 0;

if (helper_439b34(t))
    return 0;

if (!global_byte_24)
    return 0;

if (*(u8 *)((char *)t + 0x46) & (1U << 5))
    return 0;

return 1;
```

This is a compound vendor/task abort-or-continue predicate. The exact symbolic names of the two global bytes and helper `0x439b34` are unresolved.

Do not label `0xcc774` as `need_resched`, `signal_pending`, or a timeout helper.

## Controller object `+0x20`

A separate check around the same state-3 region exits toward detector re-entry/termination when controller-object `+0x20` is nonzero. Its producer and semantic name remain unresolved.

## Remaining targets

1. Resolve the registration path/name for callback `0x37acf8`.
2. Resolve `0x439b34` and the two global bytes consumed by `0xcc774`.
3. Resolve the producer/meaning of controller `+0x20`.
4. Reconstruct the full wait/re-entry loop around these predicates.
