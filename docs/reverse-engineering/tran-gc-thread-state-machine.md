# X683/H694 Transsion GC thread/state-machine reconstruction

This document is reconstructed/inferred from the stock X683/H694 boot image evidence. It is not recovered proprietary Transsion source.

## 1. Controller/state layout

| Offset | Reconstructed meaning | Evidence |
|---:|---|---|
| `+0x990` | 64-bit GC invocation/detector cycle counter | wrapper/detector reads and periodic modulo |
| `+0x998` | 32-bit controller: `0=normal`, `1=greedy`, `2=urgent` | consumed by `tran_f2fs_gc` |
| `+0x9c0` | controller-write guard | Stop 4/5 store is conditional on this byte |
| `+0x9d0` | loop/termination state | consulted on thread exit/continue paths |
| `+0x9d4` | detector state | observed values 1,2,3,4 |
| `+0x9d8` | repeated-detector counter | incremented on repeated detector path |
| `+0x9e0` | detector-cycle counter | incremented at detector entry |
| `+0x9f0` | running maximum/statistic | updated from calculated segment quantity |
| `+0x9f4` | saved baseline/statistic | participates in delta calculation |
| `+0x9f8` | stop result | `1` = Stop 4, `2` = Stop 5 |
| `+0x9fc` | stop condition | `1..3` for Stops 1..3 |
| `+0xa04` | cadence selector | `0 -> 50`, nonzero `-> 500` |
| `+0xa05` | loop-active/state byte | tested against 1 |
| `+0xa06` | detector enable/continue byte | set while active; cleared on abort/exit |
| `+0xa08` | signed baseline segment | used by periodic progress comparison |
| `+0xa0c` | baseline written-segment value | used by periodic progress comparison |

These are semantic labels; they are not claimed vendor field names.

## 2. Reconstructed control flow

```text
thread entry
    |
    +-- initialize/maintain detector state
    |
    +-- increment +0x9e0 detector-cycle counter
    |
    +-- update calculated segment/statistics
    |       |
    |       +-- +0x9f0 running maximum
    |       +-- +0x9f4 saved baseline
    |
    +-- evaluate Stop 1
    |       delta_seg > table-derived threshold
    |       -> +0x9fc = 1
    |
    +-- evaluate Stop 2
    |       second_delta > second table threshold
    |       -> +0x9fc = 2
    |
    +-- evaluate Stop 3
    |       scaled_movement < segment_reference
    |       -> +0x9fc = 3
    |
    +-- evaluate Stop 4
    |       dec/inc segment delta > vendor threshold
    |       -> log SSR switch
    |       -> if +0x9c0 == 0:
    |             +0x998 = 2
    |       -> +0x9f8 = 1
    |
    +-- periodic Stop 5
            interval = (+0xa04 == 0) ? 50 : 500
            if +0x990 % interval == 0:
                evaluate free-segment progress
                -> no sufficient progress:
                   +0x998 = 2
                   +0x9f8 = 2

controller consumption
    |
    +-- tran_f2fs_gc reads +0x998
          |
          +-- 0: normal f2fs_gc
          +-- 1: gc_mode=2 (GREEDY), f2fs_gc(...,-1), restore
          +-- 2: gc_mode=3 (URGENT), f2fs_gc(...,-1), restore
```

## 3. Stop-condition inputs

### Stop 1 — growth/delta threshold

Direct comparison:

```text
w25 = delta_seg
w9  = table[index] * w21
w9  = signed fixed-point scale using 0x51EB851F >> 37
if (w25 > w9) -> Stop 1
```

The multiplier/shift represents approximately `2.5%`. The exact table index and the semantic identity of `w21` are not yet proven, so the reconstruction must not name them more specifically.

Result:

```text
+0x9fc = 1
```

### Stop 2 — second delta threshold

Direct comparison:

```text
w9  = second calculated delta
w10 = second table-derived threshold
if (w9 > w10) -> Stop 2
```

Result:

```text
+0x9fc = 2
```

The exact source quantities producing `w9/w10` remain to be mapped to the helper at `0x37b5d4..0x37b8c0`.

### Stop 3 — movement/reference comparison

Direct comparison:

```text
x8 = 64-bit scaled movement/cost quantity
x9 = signed segment/reference quantity
if (x8 < x9) -> Stop 3
```

Result:

```text
+0x9fc = 3
```

### Stop 4 — SSR trigger

Direct comparison:

```text
x9 = calculated segment/write delta
threshold = vendor-state +0xd90
if (x9 > threshold) -> Stop 4
```

Then:

```c
if (!controller->write_blocked)
    controller->state = 2;
controller->stop_result = 1;
```

The stock log explicitly identifies this as switching to SSR.

This is the strongest proven transition:

```text
Stop 4 -> controller 2 -> URGENT wrapper -> gc_mode 3 -> f2fs_gc
```

### Stop 5 — periodic no-progress trigger

The compiled periodic selection is:

```c
interval = controller->cadence_selector ? 500 : 50;
```

The detector then evaluates the current segment/write movement against the baselines at `+0xa08` and `+0xa0c`.

When the periodic test indicates insufficient/no free-segment progress:

```text
controller +0x998 = 2
controller +0x9f8 = 2
```

The stock log says `every 400 times gc none free segment inc`; the machine code selects 50/500. Both are retained as independent evidence.

## 4. Detector-state interpretation

The observed writes/reads establish a small detector state machine but do not yet justify vendor names for each state:

```text
state 1 -> active detection / first-stage processing
state 2 -> intermediate/repeated detection path
state 3 -> later detection stage
state 4 -> terminal/alternate detector stage
```

`+0x9d8` is incremented on a repeated detector path and `+0x9e0` increments at detector entry. `+0xa05` and `+0xa06` gate loop continuation.

Exact transitions among 1/2/3/4 are **not** claimed complete until the missing surrounding basic blocks are recovered from the binary.

## 5. What is proven vs unresolved

### Proven

- `+0x998` is the controller consumed by `tran_f2fs_gc`.
- Controller `1` selects temporary `gc_mode=2` / GREEDY.
- Controller `2` selects temporary `gc_mode=3` / URGENT.
- Stop 4 writes controller `2` unless `+0x9c0` blocks the store.
- Stop 4 writes stop-result `1`.
- Stop 5 writes controller `2` and stop-result `2`.
- Stop 1/2/3 write stop-condition `1/2/3`.
- `+0x990` is used for periodic detector cadence.
- `+0xa04` selects 50 vs 500 cycles.
- `+0xa08/+0xa0c` are progress baselines.

### Still unresolved

- exact helper producing all Stop 1–3 operands;
- exact meaning of the table selected by Stop 1/2;
- exact semantic identity of `w21`;
- exact state transitions among detector states 1–4;
- exact scheduling/sleep interval of the kernel thread;
- exact `need_switch_ssr()` / `detect_charger_type()` / `tran_urgent_gc()` call ordering;
- exact wakelock ownership and release sequence;
- exact vendor-state `+0xd90` field name.

Those are deliberately left unresolved rather than invented.
