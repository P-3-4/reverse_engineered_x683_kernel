# X683 / H694 Kernel Reverse-Engineering — Project Handoff

This is the canonical continuation document for moving the project to a new chat. Work from `main` and treat this document plus the referenced evidence artifacts as the current project state.

## Current state

The supplied stock `boot.img` has been directly verified and is now the authority for all absolute-offset work:

- SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- size: 33,554,432 bytes
- kernel slot: `0x94dad4` = 9,755,348 bytes
- decompressed Image: 26,615,820 bytes
- decompressed Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`
- gzip trailing bytes after member: 114,696

The project targets the Infinix X683/H694 MT6768 stock-equivalent 4.14.141-era kernel. Recovered code is reconstructed/inferred, not proprietary-source recovery.

## Canonical repository

`P-3-4/reverse_engineered_x683_kernel`, branch `main`.

## Critical F2FS facts

- `sbi + 0x3d8` = `log_blocks_per_seg`
- `sbi + 0x408` = `user_block_count`
- `sbi + 0x4b8` = `mount_opt.opt`
- `sbi + 0x534` = `gc_mode`
- `sm_info + 0x00` = `sit_info`
- `sm_info + 0x08` = `free_info`
- `sm_info + 0x10` = `dirty_info`
- `sm_info + 0x5c` = `main_segments`
- `sm_info + 0x60` = `reserved_segments`

Stock ABI:

```c
int f2fs_gc(struct f2fs_sb_info *sbi,
            bool sync,
            bool background,
            unsigned int segno);
```

Transsion wrapper passes `NULL_SEGNO` (`-1`).

## Transsion controller

- `+0x990`: 64-bit invocation/cycle counter
- `+0x998`: controller: `0=normal`, `1=GREEDY`, `2=URGENT`
- `+0x9c0`: controller-write guard
- `+0x9d0`: loop/termination state
- `+0x9d4`: detector state
- `+0x9d8`: repeated-detector counter
- `+0x9e0`: detector-cycle counter
- `+0x9f0`: running maximum/statistic
- `+0x9f4`: saved recoverable-segment baseline
- `+0x9f8`: stop-result (`1=Stop4`, `2=Stop5`)
- `+0x9fc`: stop-condition (`1..3`)
- `+0xa00`: detector mode/state gate
- `+0xa04`: cadence selector (`0->50`, nonzero->500)
- `+0xa05`: loop-active/state byte
- `+0xa06`: detector-active/continue byte
- `+0xa08`: signed segment baseline
- `+0xa0c`: recoverable/written baseline

## Detector input reconstruction

At `0x377570` onward:

```c
sm = SM_I(sbi);
sit = sm->sit_info;
free_i = sm->free_info;
dirty_i = sm->dirty_info;

user_segments = sbi->user_block_count >> sbi->log_blocks_per_seg;
sit_segments = (*(u32 *)((char *)sit + 0x10)) >> sbi->log_blocks_per_seg;

recoverable = 0;
for (i = 0; i < 6; i++)
    recoverable += *(u32 *)((char *)dirty_i + 0x68 + i * 4);
```

The detector derives a capacity bucket:

```c
bucket = (user_segments >> 13) & 0x7ffff;
```

Stop-1 and Stop-2 use the table at image offset `+0x4e4`, with entries:

```text
{100,100,100,80,80,80,60,60}
```

Stop-3 uses a separate 64-bit table at image address corresponding to `0xe74000 + 0x610`, with entries:

```text
{80,80,80,70,70,70,60,60}
```

### Stop 1

```c
delta1 = recoverable - controller->saved_baseline;
selected_scale = vendor_global_d8c * {1,2,3,4};
threshold1 = factor[bucket] * selected_scale * 0x51EB851F >> 37;
if ((s32)delta1 > (s32)threshold1)
    +0x9fc = 1;
```

The multiplier branch is:

- bucket 0 -> `0x800`, `d8c << 1`
- bucket 1 -> `0xc00`, `d8c * 3`
- bucket 2..3 -> `0x1000`, `d8c << 1`
- bucket >=4 -> `0x1800`, `d8c << 2`

### Stop 2

```c
delta2 = recoverable - reserved_segments;
threshold2 = factor[bucket] * selected_base * 0x51EB851F >> 37;
if ((s32)delta2 > (s32)threshold2)
    +0x9fc = 2;
```

This path uses **unsigned** multiply (`umull`) before the `>>37` shift.

### Stop 3

```c
span = (s64)(user_segments - sit_segments);
prod = table2[bucket] * span;
high = smulh(prod, 0xA3D70A3D70A3D70B);
scaled = (high + prod) >> 6;
scaled += (prod < 0);
reference = (s64)(s32)(recoverable - reserved_segments);
if (scaled < reference)
    +0x9fc = 3;
```

## Detector arming

At `0x377120..0x377494`, the static detector performs filesystem-capacity and ratio checks and moves the detector into active state. The exact control-field stores are preserved in:

`docs/reverse-engineering/tran-gc-detector-arming-deep-pass.md`

The common continuation enables detection and reaches state 3.

## State 3 runtime — exact current reconstruction

At `0x377494`:

```text
+0x9d4 = 3
vendor-state +0x158 = 1
```

Then:

1. Load timeout source at controller/vendor `+0xd94`.
2. If the alternate runtime branch is taken, overwrite `+0xd94` with literal `500`.
3. Call `0xce58c` to convert the timeout value to scheduler units.
4. Initialize a waitqueue entry on the stack through `0x9c688`.
5. Queue the wait through `0x9c6e8`.
6. Recheck task scheduler state with `0x57554`.
7. Check controller-object `+0x20` and vendor global `+0x974` as exit/abort gates.
8. Return to `0x377570` for metric collection after timeout/wake.

### Timeout helper `0xce58c`

For nonnegative values used by this state-3 path:

```c
return (ms + 3) >> 2;
```

Thus the literal `500` path converts to `125` scheduler-time units in the recovered helper.

### `0x57554`

This helper reads the current task flags and extracts a scheduler/reschedule bit. It is the standard kernel task-state fast-check used around the timed wait. The exact vendor semantic meaning of the surrounding wait-loop gate remains unresolved.

### Waitqueue helpers

`0x9c688` is the wait-entry initializer.

`0x9c6e8` is the enqueue/sleep helper. The state-3 block is therefore a real timed wait/scheduler path, not a busy loop.

### Runtime guards

At `0x3774b8..0x37754c`:

- controller-object `+0x20 != 0` diverts to an exit/alternate path;
- vendor global `+0x974 != 0` diverts to the exit path;
- otherwise the timed wait is maintained and rechecked;
- helper `0xcc774` is used on a nested filesystem/task-state branch before returning to the wait condition.

The exact semantic names of these two vendor gates are still unresolved.

## Stop 4 / Stop 5

### Stop 4

Direct stock behavior:

```text
threshold predicate true
-> controller +0x998 = 2 unless +0x9c0 blocks
-> +0x9f8 = 1
-> SSR-trigger log
```

### Stop 5

```c
interval = +0xa04 ? 500 : 50;
if (cycle % interval == 0) {
    progress = current_sit_component +
               (current_recoverable - baseline_recoverable);
    if (progress <= baseline_segment) {
        controller = 2;
        +0x9f8 = 2;
    }
}
```

## Transsion wrapper

At `0x37ada8`:

```text
controller 0 -> normal f2fs_gc
controller 1 -> temporary gc_mode=2 (GREEDY), call f2fs_gc(...,-1), restore
controller 2 -> temporary gc_mode=3 (URGENT), call f2fs_gc(...,-1), restore
```

## Next target

Resolve the helper call targets and the remaining state-3 predicates:

```text
0x377494..0x377570
    -> 0x57554 task-state helper
    -> 0xcc774 nested filesystem/task-state check
    -> 0xe06684 / 0xe0693c metric helpers
    -> 0x1eca60 scheduler bookkeeping helper
    -> exact wake sources and vendor semantic names for +0x974 / controller +0x20
```

Then integrate these exact runtime transitions into `tran_gc_thread_reconstructed.c` and proceed to `0x37b5d4..0x37b8c0` threshold/helper reconstruction.
