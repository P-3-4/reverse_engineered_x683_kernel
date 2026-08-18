# X683/H694 Transsion GC state-3 helper resolution

Source boot.img SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
Offsets are decompressed-kernel offsets.

## State-3 path: 0x377494..0x377570

Stock sequence:

```text
0x377494  +0x9d4 = 3
0x3774a8  call 0xce58c
0x3774b0  call 0x57554
0x3774dc  init wait entry
0x377500  prepare wait / set TASK_INTERRUPTIBLE
0x377508  recheck NEED_RESCHED
0x37753c  call 0xcc774 on task pointer
0x377548  wait cleanup
0x377554  recheck NEED_RESCHED
0x377570  metric collection
```

## Resolved kernel helpers

### 0xce58c

Machine code:

```asm
add x8, x0, #3
cmp w0, #0
lsr x8, x8, #2
csel x0, xzr, x8, lt
```

For the positive timeout path this is exactly the millisecond-to-jiffies conversion used by the stock `HZ=250` configuration:

```text
jiffies = (ms + 3) >> 2
```

Therefore `500 ms -> 125 jiffies`.

### 0x57554

Reads the current task's thread flags and extracts bit 1:

```asm
mrs  x19, SP_EL0
ldrb w8, [x19,#0x46]
ldr  x8, [x19,#0x6b8]
ldr  x8, [x8]
ubfx x0, x8, #1, #1
```

This is the stock `TIF_NEED_RESCHED` predicate. It is used immediately before/after the wait path to decide whether scheduler work is pending.

### 0x9c688

Constructs a waitqueue entry:

```asm
str  w1, [x0]
adrp x9, default_wake_function
mrs  x8, SP_EL0
stp  x8, x9, [x0,#8]
add  x10, x0, #0x18
str  x10, [x0,#0x18]
str  x10, [x0,#0x20]
```

This is the `init_wait()` / wait-entry initialization primitive.

### 0x9c6e8

Called as:

```c
helper(waitqueue_head, &wait, TASK_INTERRUPTIBLE);
```

and contains the waitqueue insertion/state machinery. This is the `prepare_to_wait_event()` lineage used to enqueue the current task and set its sleep state.

### 0x9c8d0

Called on exit with the same wait entry and contains the list-unlink cleanup. This is the `finish_wait()` lineage.

### 0xe06684

Stock body is the mutex acquire fastpath:

```asm
ldxr  x11, [x0]
eor   x10, x11, current
...
stlxr ...
```

with a slowpath that walks the mutex waiter list. It is therefore `mutex_lock()`-lineage code.

### 0xe0693c

Stock body performs the owner/flag compare-exchange and returns 1 only when the current task acquires the encoded owner word. This is the `mutex_trylock()` lineage. Linux mutex owners encode the owner pointer plus low-bit flags, which matches the recovered implementation. citeturn167246search0turn167246search9

### 0xcc774

This remains the only unresolved helper in the immediate wait path. Its body:

```asm
ldr  w8, [x0,#0x44]
tst  w8, #0x80008000
ldr  x8, [x0]
tbnz w8, #0x12, ...
...
ldrb w8, [global+#0x28]
...
ldrb w8, [global+#0x24]
ldrb w8, [x19,#0x46]
tbnz w8, #5, ...
return 1/0
```

The caller passes the current-task pointer and uses the boolean to determine whether to leave/retry the wait path. The available machine code does not justify assigning a stronger vendor-specific name yet.

## Corrected state-3 interpretation

The state-3 path is therefore:

```text
+0x9d4 = 3
    |
    +-- timeout source +0xd94
    |      |
    |      +-- 0xce58c -> jiffies
    |
    +-- scheduler check 0x57554 (NEED_RESCHED)
    |
    +-- init_wait
    |
    +-- prepare_to_wait(..., TASK_INTERRUPTIBLE)
    |
    +-- task/vendor wake/abort predicate 0xcc774
    |
    +-- finish_wait
    |
    +-- mutex lock / trylock around detector work
    |       0xe06684 / 0xe0693c
    |
    +-- 0x377570 metric collection
    |
    +-- Stop 1..5 evaluation
```

The previous description of `0xcc774` as a generic scheduler-timeout helper is corrected and should no longer be used.
