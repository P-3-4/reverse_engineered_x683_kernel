# X683/H694 Transsion GC — wait-flag producer deep pass

Source: stock `boot.img`, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.

Offsets below are decompressed-kernel offsets.

## `+0x974` producer

Function at `0x37acf8` is a callback-like event handler. Its first dispatch gate is:

```text
if (event != 9)
    return 0
```

For event `9`, it reads a status value from:

```text
*(event_data + 0x8)
```

The recovered branches are:

```text
status == 0:
    global +0x974 = 1
    optional notify(global +0x978, 3, 1, 0)

status == 4:
    global +0x974 = 0
    optional notify(global +0x978, 3, 1, 0)

other status values:
    no +0x974 write
```

The auxiliary notification is conditional on global `+0x898` being nonzero.

The public/vendor name of this callback is not proven by the current binary evidence, so no semantic name is assigned.

## GC consumer

The detector's state-3 path reads `global +0x974` at `0x3774c4`, `0x37751c`, and `0x377560`.

Recovered control meaning:

```text
+0x974 != 0
    skip/leave the current sleep sequence and re-enter the detector loop

+0x974 == 0
    continue through wait/recheck logic; metric collection follows only after
    the normal scheduler/task predicates permit it
```

Therefore `+0x974` is a producer-driven wake/recheck flag, not a persistent GC statistic.

## State-3 sequence now established

```text
+0x9d4 = 3
    -> timeout +0xd94 through 0xce58c
    -> NEED_RESCHED check 0x57554
    -> init_wait 0x9c688
    -> prepare_to_wait/event sleep 0x9c6e8
    -> NEED_RESCHED recheck
    -> controller/task state checks
    -> task/vendor predicate 0xcc774
    -> finish_wait 0x9c8d0
    -> NEED_RESCHED check
    -> +0x974 gate
    -> metric collection
```

## Related helper

`0xcc774` remains unresolved at the semantic-name level. Its direct body is task-state based and calls `0x1051a8` under one branch. `0x1051a8` inspects an object reached through `task + 0x950`, then tests bits `0x6` in a field at `+0xb8`. This is useful for further kernel-helper identification, but is not yet sufficient to assign a source-level Linux helper name.

## `+0xa10` correlation

The detector also caches a runtime quantity at global `+0xa10`. Around `0x3773dc..0x377458`, it compares this cache against the current value derived from the F2FS manager object. On mismatch it stores the new value and loops/rechecks; on equality it executes helper `0x1eca60` and proceeds through the subsequent branch path.

`0x1eca60` is not yet given a semantic name because its body is a generic per-object reference/locking primitive and its exact surrounding type is unresolved.

## Status

### High confidence
- `+0x974` is event-driven.
- Event number `9` is the producer dispatch value.
- State `0` sets `+0x974 = 1`.
- State `4` clears `+0x974 = 0`.
- The GC detector consumes `+0x974` in the state-3 sleep/re-entry path.

### Still unresolved
- Public/vendor name and registration site for the `0x37acf8` callback.
- Exact semantics/name of `0xcc774`.
- Exact semantic type of the object/operation behind `0x1eca60`.
- Exact meaning of controller-object `+0x20` in the scheduler loop.
