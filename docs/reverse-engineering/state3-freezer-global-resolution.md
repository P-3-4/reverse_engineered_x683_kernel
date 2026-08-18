# X683/H694 state-3 `0xcc774` / freezer-global resolution

Source: stock `boot.img`, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.

## Correction: `0x439b34` was not the helper

The earlier note that called `0x439b34` a helper was an address-type error.
At `0x439b34` the stock Image contains a `BL` instruction:

```asm
0x439b34  bl 0x4bb848
```

So `0x439b34` is a **call site**, not a function entry.

The function called from the `0xcc774` predicate is instead:

```text
0xcc7a8 -> BL 0x1051a8
```

## `0x1051a8` exact body

```c
static bool x683_task_freezer_state(struct task_struct *task)
{
    bool active;

    preempt_disable();
    /* task + 0x950 -> object; object + 0x30 -> state; state + 0xb8 */
    active = !!((*(u32 *)((char *)task + 0x950)
                 ? *(u32 *)(*(u64 *)((char *)task + 0x950) + 0x30 + 0xb8)
                 : 0) & 0x6);
    preempt_enable();
    return active;
}
```

The object/field semantic names are not asserted here; the exact machine-code fact is that the helper reads a pointer at `task + 0x950`, follows `+0x30`, tests bits `0x6` in the resulting `+0xb8` word, and returns that boolean under a preempt-disabled section.

## `0xcc774` exact predicate

The stock predicate is:

```c
if (*(u32 *)((char *)task + 0x44) & 0x80008000)
    return 0;

if (*(unsigned long *)task & (1UL << 18))
    return 0;

if (pm_nosig_freezing)
    return 0;

if (x683_task_freezer_state(task))
    return 0;

if (!pm_freezing)
    return 0;

if (*(u8 *)((char *)task + 0x46) & (1U << 5))
    return 0;

return 1;
```

The last two global bytes are therefore not anonymous vendor state.

## Global `+0x24` = `pm_freezing`

Exact producer/consumer chain:

```text
0xb0c40 (freeze_processes)
    -> +0x24 = 1
    -> try_to_freeze_tasks(...)

0xb1098 (thaw_processes)
    -> +0x24 = 0
```

The surrounding strings are the stock power-management messages:

```text
"Freezing user space processes ..."
"thaw_processes"
"Restarting tasks ..."
```

This matches the Linux 4.14 `kernel/power/process.c` freezer state model.

## Global `+0x28` = `pm_nosig_freezing`

Exact producer/consumer chain:

```text
0xb134c (freeze_kernel_threads)
    -> +0x28 = 1
    -> try_to_freeze_tasks(false)

0xb13c4 (error path / thaw_kernel_threads)
    -> +0x28 = 0
```

The corresponding strings are:

```text
"Freezing remaining freezable tasks ..."
"Restarting kernel threads ..."
```

This identifies `+0x28` as the kernel-thread/no-signal-freezing state.

## Resulting `cc774` semantic upgrade

`0xcc774` is therefore best described as a **freezer-aware task eligibility predicate**.

Its machine-code gating is now:

```text
task flags gate
    -> task bit 18 gate
    -> pm_nosig_freezing
    -> task freezer-state bits 0x6
    -> pm_freezing
    -> task byte +0x46 bit 5
```

The public symbolic name of the surrounding vendor wrapper is still not proven, but the two globals and the called helper are resolved sufficiently to integrate the state-3 predicate without inventing vendor flags.

## `0x439b34` disposition

No relationship between the `0x439b34` call site / `0x4bb848` and the `0xcc774` state-3 predicate is established. The earlier cross-reference should be removed from GC-specific notes.
