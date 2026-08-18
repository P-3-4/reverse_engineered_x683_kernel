# X683/H694 state-3 freezer predicate — final resolution

Source: stock `boot.img`, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.

## `0x1051a8`

`0x1051a8` is the kernel cgroup freezer predicate `cgroup_freezing(struct task_struct *)`.

Stock 4.14 `cgroup_freezing()` acquires an RCU/read-side protection, obtains the task's freezer cgroup state, tests for `CGROUP_FREEZING`/`CGROUP_FROZEN`, then releases the read-side protection. The X683 implementation matches this structure: its helpers at `0xc367c`/`0xc3690` implement the read-side protection, followed by the task cgroup/freezer-state chain and a `state & 0x6` test.

The Android 4.14 freezer source confirms `cgroup_freezing()` and its role in `freezing_slow_path()`. citeturn416145search3turn392391search7

## `0xcc774`

The exact predicate is:

```c
if (task->flags & (PF_NOFREEZE | PF_SUSPEND_TASK))
    return false;

if (task->flags & PF_KSWAPD)
    return false;

if (pm_nosig_freezing)
    return false;

if (cgroup_freezing(task))
    return false;

if (!pm_freezing)
    return false;

if (task->flags & PF_KTHREAD)
    return false;

return true;
```

The first condition is the direct `0x80008000` mask. In the X683 task flag layout, this is `PF_NOFREEZE | PF_SUSPEND_TASK`.

The second condition is a direct bit-18 test of `task->flags`, which maps to `PF_KSWAPD` in the matching 4.14 Android/MSM scheduler headers. citeturn937350search2

The `+0x28` global is `pm_nosig_freezing`; `+0x24` is `pm_freezing`. The `task + 0x46` bit-5 test is bit 21 of `task->flags`, i.e. `PF_KTHREAD`. citeturn937350search2

## Relationship to upstream `freezing_slow_path()`

Android/common 4.14 implements the standard logic as:

```c
if (p->flags & (PF_NOFREEZE | PF_SUSPEND_TASK))
    return false;

if (test_tsk_thread_flag(p, TIF_MEMDIE))
    return false;

if (pm_nosig_freezing || cgroup_freezing(p))
    return true;

if (pm_freezing && !(p->flags & PF_KTHREAD))
    return true;

return false;
```

The X683 detector contains the same freezer decision structure but is not byte-identical: it directly rejects `PF_KSWAPD` instead of testing `TIF_MEMDIE`, and because the vendor detector wants the inverse condition, it returns `true` only when the task is eligible under `pm_freezing` and all reject conditions are clear. citeturn392391search2

## `+0x20` resolution

The object previously described as `controller-object +0x20` is actually the global freezer state object at `Image + 0x19f0000`.

Its relevant fields are:

```text
+0x20 = system_freezing_cnt
+0x24 = pm_freezing
+0x28 = pm_nosig_freezing
```

The state-3 code performs:

```c
if (system_freezing_cnt)
    exit/recheck;
```

The global layout is independently confirmed by the Android 4.14 freezer implementation, which declares `system_freezing_cnt`, `pm_freezing`, and `pm_nosig_freezing` as the freezer-state globals. citeturn392391search2turn623192search1

## Result

The state-3 wake/abort path is now semantically resolved as a freezer-aware scheduler path, not proprietary opaque task logic:

```text
system_freezing_cnt
pm_nosig_freezing
cgroup_freezing(task)
pm_freezing
PF_NOFREEZE / PF_SUSPEND_TASK
PF_KSWAPD (X683-specific deviation)
PF_KTHREAD
        |
        v
vendor detector wait/re-entry decision
```

Remaining unresolved state-3 items are now reduced to the vendor event callback identity for `0x37acf8` and the exact surrounding `0x377494..0x377570` control-flow integration. The freezer predicate itself is resolved.
