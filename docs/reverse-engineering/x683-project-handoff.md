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

## Detector / Stop conditions

At `0x377570` onward the detector derives `user_segments`, `sit_segments`, and recoverable/free-segment quantities.

### Stop 1

`0x377720` performs a signed `delta1 > threshold1` comparison and writes `+0x9fc=1`.

### Stop 2

`0x377770` performs a signed second-delta comparison and writes `+0x9fc=2`.

### Stop 3

`0x3777d0` performs a signed 64-bit scaled-movement `< reference` comparison and writes `+0x9fc=3`.

### Stop 4

`0x3777f0` compares the vendor delta against the threshold at vendor state `+0xd90`; true path writes `+0x998=2` unless `+0x9c0` blocks it, then `+0x9f8=1`, with the SSR-switch log.

### Stop 5

`+0xa04` selects 50/500 cycles; the periodic no-progress path writes `+0x998=2` and `+0x9f8=2`.

## Exact GC threshold/helper reconstruction

The direct disassembly of `0x37b580..0x37b8c0` is now reconstructed in:

`docs/reverse-engineering/gc-threshold-helper-exact-reconstruction.md`

and the corresponding reconstructed source is:

`fs/f2fs/tran_gc_threshold_reconstructed.c`

### `0x37b580`

Binary arithmetic:

```c
user_segments = user_block_count >> log_blocks_per_seg;
sit_segments  = sit_blocks >> log_blocks_per_seg;
span          = user_segments - sit_segments;
free_percent  = free_segments * 100 / span;
fragmentation = 100 - free_percent;
```

It logs:

```text
"f2fs alloc new segment and fragmentation is %lu"
```

Exact proprietary source name remains unproven; reconstructed name is `x683_calc_fragmentation_percent()`.

### `0x37b5d4..0x37b748`

This is one boolean policy helper, directly called from detector locations `0x377104` and `0x377de8`. It returns `1` on either of the Stop-2/Stop-3 predicates and otherwise reaches its diagnostic logging path and returns `0`.

Exact scale selection is:

```c
if ((user_segments >> 15) == 0)
    scale = table_0x4d4[user_segments >> 13];
else
    scale = 0x1800;
```

where table `Image+0x4d4` is:

```text
{0x800, 0xc00, 0x1000, 0x1000}
```

Vendor selector:

```c
selector = max(*(u8 *)0x1a13890, *(u8 *)0x1a13894);
```

Factor table at `Image+0x4e4`:

```text
{100,100,100,80,80,80,60,60}
```

Stop-2 predicate:

```c
delta = (s32)(free_segments - reserved_segments);
threshold = (factor * scale * 0x51EB851F) >> 37;
if (delta > threshold)
    return 1;
```

`0x51EB851F / 2^37 ~= 0.01`.

Stop-3 predicate:

```c
span = (s64)user_segments - (s64)sit_segments;
reference = (s64)(s32)(free_segments - reserved_segments);
p = table64[selector] * span;
high = smulh(p, 0xA3D70A3D70A3D70B);
scaled = (high + p) >> 6;
scaled += (p < 0);
if (scaled < reference)
    return 1;
```

64-bit table at `Image+0xe74610`:

```text
{80,80,80,70,70,70,60,60}
```

When both predicates fail, the helper computes diagnostic values and logs:

```text
"free_segment=%d, fix_size=%d, left_space=%d(precent:%d)"
```

The `0x37b6d8..0x37b748` sequence is a continuation of the same helper, not a separate function boundary.

### Post-helper range

`0x37b74c..0x37b8c0` transitions into generic vendor control/attribute helpers:

- `0x37b74c -> 0x38ba24`
- `0x37b76c -> 0x37bedc` descriptor/list helper
- `0x37b7cc` bit-state getter
- `0x37b7e0 / 0x37b844` type-gated operation wrappers
- `0x37b8a8 -> 0x03bb48`, return bit0
- `0x37b8c4` byte getter
- `0x37b8d8` byte/state update path

These are not being labeled as GC policy functions without a direct call/data-flow proof.

## Detector state 3

At `0x377494`, `+0x9d4=3`; the path performs the confirmed timed wait/recheck sequence using the waitqueue and scheduler helpers previously documented.

## Vendor controls

Named controls are registered through the common runtime object at `Image+0x1a13a20`:

```text
need_switch_ssr     -> descriptor 0x173b9d0
tran_urgent_gc      -> descriptor 0x173bbb0
detect_charger_type -> descriptor 0x173bf70
```

They are per-control data descriptors, not proven standalone function symbols. `tran_gc_usb_wakelock` remains separate/unresolved.

## stat_info

`sbi+0x568` is a pointer to a separate vendor-divergent statistics object.

Confirmed direct members:

```text
+0x164 incremented
+0x174 incremented
+0x184 accumulated
+0x178 incremented
+0x188 accumulated
+0x18c incremented
+0x190 incremented
+0x198 accumulated
```

Exact source member names remain unresolved. `sbi+0x570..0x5dc` are separate SBI fields, not stat_info members.

## Current next target

1. Complete the generic attribute/control helper path after `0x37b74c` and bind descriptor values to actual reads/writes.
2. Independently prove the six `dirty_info +0x68..0x7c` entries.
3. Function-level diff stock X683 `f2fs_gc()` against the closest historical 4.14 F2FS baseline.
4. Classify the exact X683 vendor delta and replace provisional source accordingly.

All reconstructed code remains explicitly reconstructed/inferred and is not claimed as proprietary Transsion source.
