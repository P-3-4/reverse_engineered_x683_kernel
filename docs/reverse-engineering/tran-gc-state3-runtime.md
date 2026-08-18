# X683/H694 Transsion GC state-3 runtime path

Reconstructed/inferred from the supplied stock boot.img. Not proprietary source recovery.

## State-3 entry

At `0x377494` the detector writes:

```c
controller->detector_state = 3;
```

Then it stores `1` at a separate global byte/word around `0x173b000 + 0x158`, calls the kernel task-state helper at `0xce58c`, and uses the returned timeout value to prepare the wait path.

## Timeout conversion helper

`0xce58c` is a small scalar conversion routine:

```c
static unsigned long x683_timeout_from_ms(int ms)
{
	if (ms >= 0)
		return (ms + 3) >> 2;
	return 0x3ffffffffffffffeUL;
}
```

Its other code paths implement separate signed timeout/rate conversions and are not part of the state-3 wait call used here.

The stock state-3 path loads `controller_object + 0xd94`; the normal branch writes the value loaded from another global `+0xd84`, while the alternate branch writes literal `500` to `+0xd94` before conversion. Thus the sleep timeout source is a vendor runtime parameter with a stock fallback of `500`.

## Waitqueue construction

At `0x3774dc..0x377504` the code constructs a waitqueue entry on the stack using the same helper family as the kernel waitqueue primitives:

```text
0x3774dc  add x0, sp, #0x40
0x3774e0  mov w1, #0
0x3774e4  bl  0x9c688       // wait-entry initialization
...
0x3774f4  add x1, sp, #0x40
0x3774f8  mov w2, #1
0x3774fc  mov x0, x27
0x377500  bl  0x9c6e8       // queue/sleep helper
```

`0x9c688` initializes a waitqueue entry; `0x9c6e8` performs the queueing/scheduling operation and handles task-state interactions before returning.

## Wake/recheck predicates

After queueing, the thread calls `0x57554`. Its implementation reads the current task's flags and returns bit 1 from the task-state word. This is the Linux ARM64 `need_resched()`-style fast check used by the surrounding scheduler path.

The sequence is:

```text
need_resched/test
    -> if set, exit wait loop

controller/object +0x20
    -> if nonzero, exit toward detector termination path

vendor global +0x974
    -> if nonzero, exit/skip timed wait

otherwise:
    queue timed wait
    -> recheck +0x974 / scheduler state
    -> if wait entry remains queued, repeat
```

The exact vendor semantic name for global `+0x974` is still unresolved; it is a direct loop-abort/disable gate.

## State-3 runtime shape

```text
state 3
  |
  +-- load timeout parameter
  |
  +-- convert timeout to scheduler units
  |
  +-- initialize waitqueue entry
  |
  +-- enqueue timed wait
  |
  +-- scheduler / need_resched check
  |
  +-- check controller-object +0x20
  |
  +-- check vendor global +0x974
  |
  +-- wake / timeout / recheck
  |
  +-- return to detector at 0x377570
```

## 0x377570 metric-entry point

When the loop returns to `0x377570`, the stock code begins the detector measurement stage by following:

```text
sbi + 0x80
    -> sm_info

sm_info + 0x00
    -> sit_info

sm_info + 0x08
    -> free_info

sm_info + 0x10
    -> dirty_info

then:
    user_block_count  = sbi + 0x408
    log_blocks_per_seg = sbi + 0x3d8
    sit_blocks        = sit_info + 0x10
    free/recoverable counts from free/dirty structures
```

The resulting values feed the Stop-1..5 producer chain already reconstructed.

## Confidence

High:
- state 3 is written at `0x377494`;
- timeout is sourced from `+0xd94`, with a `500` fallback path;
- `0xce58c` is the timeout conversion helper used by this path;
- a stack waitqueue entry is initialized and queued;
- `0x57554` is the task-state reschedule check used during the wait loop;
- vendor global `+0x974` is a loop-abort/disable gate;
- `0x377570` is the beginning of the metric collection stage.

Unresolved:
- exact vendor symbolic name of `+0x974`;
- exact semantic name of controller-object `+0x20`;
- exact wake event producer(s) that set those gates.
