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
- `sbi + 0x568` = `struct f2fs_stat_info *`
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

### Stop 2

```c
delta2 = recoverable - reserved_segments;
threshold2 = factor[bucket] * selected_base * 0x51EB851F >> 37;
if ((s32)delta2 > (s32)threshold2)
    +0x9fc = 2;
```

This path uses **unsigned** multiply before the shift.

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

## Detector arming / state 3

At `0x377120..0x377494`, static arming performs filesystem-capacity and ratio checks. State 3 is entered with `+0x9d4=3`; the subsequent runtime path performs a timed wait/recheck before returning to metric collection.

Relevant direct helpers remain:

- `0xce58c`: timeout conversion helper
- `0x57554`: current-task scheduler/reschedule flag check
- `0x9c688`: wait-entry initializer
- `0x9c6e8`: waitqueue insertion/setup
- `0x9c8d0`: wait completion/removal
- `0xe06684`: mutex lock path
- `0xe0693c`: mutex trylock path
- `0xcc774`: vendor/task-state predicate, exact semantics unresolved

Runtime guards include controller-object `+0x20` and vendor global `+0x974`.

## Stop 4 / Stop 5

### Stop 4

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

## Vendor control registration

Named controls are registered through a common runtime object at `Image + 0x1a13a20` and common registration layer `0x274ea0 -> 0x274dac`.

Confirmed bindings:

```text
need_switch_ssr
  registration 0x37af88
  descriptor 0x173b9d0

tran_urgent_gc
  registration 0x37b068
  descriptor 0x173bbb0

detect_charger_type
  registration 0x37b184
  descriptor 0x173bf70
```

These are control/data descriptors, not proven standalone implementation functions. `tran_gc_usb_wakelock` remains on a separate, unresolved path.

## `stat_info` reconstruction

A new binary-derived map is committed in `docs/reverse-engineering/x683-stat-info-reconstruction.md`.

Direct GC evidence establishes:

```text
sbi + 0x568 -> stat_info

stat + 0x164:
    load, +1, store

stat + 0x174:
    load, +1, store
stat + 0x184:
    load, add local w11, store

stat + 0x178:
    load, +1, store
stat + 0x188:
    load, add local w11, store

stat + 0x18c:
    load, +1, store
stat + 0x190:
    load, +1, store
stat + 0x198:
    load, add local w12, store
```

`+0x170` is part of the same preceding segment-accounting family, but its exact update semantics still require the immediately preceding basic block to be mapped. Exact historical member names remain intentionally unresolved.

Important: `sbi + 0x570..0x5dc` are separate SBI fields, not `stat_info` members. Older hypotheses for `0x5d4/0x5d8/0x5dc` remain candidates only and have not been promoted to facts.

## Current unresolved high-value targets

1. Trace reads/logging of `stat_info + 0x164..0x198` and bind them to the vendor debug control names (`gc_times`, `gc_segment_info`, `written_data`, etc.).
2. Recover the remaining X683 `stat_info` layout before assigning historical F2FS member names.
3. Resolve `tran_gc_usb_wakelock`'s separate registration/use path.
4. Finish the `0x37b5d4..0x37b8c0` GC threshold/helper reconstruction.
5. Compare complete stock `f2fs_gc()` against the closest historical 4.14 F2FS revision and classify the vendor delta.

All reconstructed source remains explicitly reconstructed/inferred and must not be represented as proprietary Transsion source.
