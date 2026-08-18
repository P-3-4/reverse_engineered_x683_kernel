# X683/H694 `0xcc774` trace

Directly disassembled from the supplied boot.img (SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`).

## Function

`0xcc774` returns a boolean.

Observed logic:

```c
u32 f = *(u32 *)((char *)task + 0x44);
if (f & 0x80008000)
    return 1;

if (*(u64 *)task & (1ULL << 18))
    return 1;

if (!global_19f0000_28) {
    if (!helper_0x1051a8(task))
        return 1;

    if (!global_19f0000_24)
        return 0;

    if (*(u8 *)((char *)task + 0x46) & (1 << 5))
        return 0;
}
return 1;
```

The exact vendor semantic name is intentionally not assigned. It is a **task-state / execution-abort predicate**, not a Transsion-specific charger predicate.

## Relevant X683 callers

### `0x377034`

```text
x0 = current-task pointer saved on stack
call 0xcc774
if bit0 of return is set -> 0x3770e8
else -> wait path
```

This is part of the detector's initial wait/recheck loop.

### `0x3770bc`

Again called with the current-task pointer. Its boolean is combined with the remaining timeout and controls the loop that repeatedly waits/rechecks before detector work continues.

### `0x377540`

Called with the current-task pointer after the timed detector wait has prepared its waitqueue entry. If true, execution skips the remaining wait path and reaches cleanup/recheck.

### `0x3778f4`

Called from the Stop-5/loop-side abort path with the current-task pointer. A true result branches to `0x377554`, ending the wait/re-entry path.

## Important correction

`0xcc774` is therefore **not** the scheduler timeout conversion and is not itself `need_switch_ssr()`, `detect_charger_type()`, or `tran_urgent_gc()`.

Its role is a generic kernel/task predicate used repeatedly by the vendor detector to decide whether a wait/sleep loop should abort or proceed.

The vendor-specific predicates remain in the surrounding calls/branches, notably `0x4cbe38`, `0xb271c`, `0x366cd4`, `0x1ec9e4`, `0x1eca60`, and the paths reached from `0x377540..0x377570`.

## `0x57554`

`0x57554` reads the current task's `+0x46` byte. When bit 5 is set it reads a task field at `+0x6b8` and returns bit 1 of the referenced value; otherwise it returns a kernel-internal fallback value after a one-time warning/log path.

This is another task-state predicate used alongside `0xcc774`; it is not a Transsion charger/SSR helper.

## `0x1eca60`

The function operates on an indexed per-object lock/state slot and manipulates per-CPU preemption/locking accounting. It returns a boolean indicating the state transition result. It is a synchronization primitive rather than a charger/SSR detector.

## `0xe0693c`

This function is an atomic owner/state acquisition operation. It compares the encoded owner against the current task and attempts to install the current task identity while preserving low flag bits. The return value is success/failure. It is a synchronization fast path, not a charger/SSR semantic function.

## Consequence for the GC reconstruction

The state-3 path is now partitioned correctly:

```text
vendor detector
  -> generic task-state predicate (0xcc774)
  -> task flag predicate (0x57554)
  -> synchronization primitive(s) (0x1eca60 / 0xe0693c)
  -> only then vendor/F2FS decision helpers
```

Do not label `0xcc774` as `need_switch_ssr` or `detect_charger_type` without a direct symbol/string/xref proving that identification.
