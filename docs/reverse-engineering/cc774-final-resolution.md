# X683/H694 `0xcc774` final resolution

Source: stock `boot.img`, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.

## Important address correction

The previous project note saying `0x439b34` was the helper used by `0xcc774` was incorrect.

`0x439b34` is an instruction inside another function:

```asm
0x439b34: bl 0x4bb848
```

The helper actually called by `0xcc774` is:

```asm
0xcc7a8: bl 0x1051a8
```

No proven GC-specific connection exists between `0x439b34` and `0xcc774`.

## `0x1051a8`

Exact behavior:

```text
preempt_disable
load task + 0x950
follow object + 0x30
load state + 0xb8
return ((flags & 0x6) != 0)
preempt_enable
```

This is a task freezer-state test. The exact vendor/object symbolic type remains unresolved.

## `0xcc774`

Exact boolean sequence:

```c
if (task->raw_0x44 & 0x80008000)
    return 0;

if (task->raw_flags & (1UL << 18))
    return 0;

if (pm_nosig_freezing)
    return 0;

if (task_freezer_state(task))
    return 0;

if (!pm_freezing)
    return 0;

if (task->raw_0x46 & (1U << 5))
    return 0;

return 1;
```

Therefore this is a **freezer-aware task eligibility predicate**.

## `+0x24` exact identity

The BSS/global storage targeted by `ADRP 0x19f0000` + `#0x24` is `pm_freezing`.

Producers:

```text
0xb0c40 = freeze_processes()
    -> +0x24 = 1

0xb1098 = thaw_processes()
    -> +0x24 = 0
```

The functions are confirmed by the adjacent stock power-management strings:

```text
Freezing user space processes ...
thaw_processes
Restarting tasks ...
```

## `+0x28` exact identity

The BSS/global storage targeted by `ADRP 0x19f0000` + `#0x28` is `pm_nosig_freezing`.

Producers:

```text
0xb134c = freeze_kernel_threads()
    -> +0x28 = 1

0xb13c4 = thaw_kernel_threads() error cleanup
    -> +0x28 = 0
```

Adjacent strings confirm the kernel-thread freezer path:

```text
Freezing remaining freezable tasks ...
Restarting kernel threads ...
```

## Integration consequence

The GC state-3 reconstruction must no longer model the `0xcc774` global inputs as vendor-specific anonymous bytes.

The recovered chain is:

```text
0xcc774
  |
  +-- task flags +0x44 mask 0x80008000
  +-- task flags bit 18
  +-- pm_nosig_freezing
  +-- helper 0x1051a8 / task freezer state bits 0x6
  +-- pm_freezing
  +-- task byte +0x46 bit 5
  +-- return 1
```

This resolves requested targets #1 and #2 at the binary/data-flow level, with only the public symbolic name of `0x1051a8` and its nested object type still open.
