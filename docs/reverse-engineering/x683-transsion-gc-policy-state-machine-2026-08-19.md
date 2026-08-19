# X683/H694 Transsion GC Policy and State Machine Reconstruction — 2026-08-19

## Scope and evidence standard

Target:

```text
Linux 4.14.141+ / ARM64 / MT6768 / Infinix X683
Image base used for runtime mapping: 0xffffff92d0a80000
CONFIG_F2FS_TRAN_GC=y
```

This phase reconstructs the remaining Transsion-owned garbage-collection control layer around the already-reconstructed stock X683 F2FS collector. The victim-selection, scoring, SSR, and migration phase is treated as complete and is not reopened except where a mode change is needed to explain the controller.

Evidence hierarchy used here:

1. direct X683 instruction/control-flow evidence;
2. X683 kallsyms and configuration;
3. closest Linux/Android 4.14 F2FS implementation as naming/shape baseline;
4. inference only where the binary does not prove a source-level name.

The binary is authoritative. Unknown fields retain offset-based names rather than being assigned guessed C members.

## Executive result

The X683 Transsion GC subsystem is a controller/state machine layered above the stock F2FS collector. The vendor delta is concentrated in:

```text
external event / explicit request
        |
        v
vendor GC worker state
        |
        +-- charger / USB start-stop
        +-- framebuffer wakeup
        +-- wakelock gates
        +-- urgent-GC flag
        +-- free-segment policy
        +-- dirty/fragmentation thresholds
        +-- SSR override state
        |
        v
configured gc_type (0..2)
        |
        v
tran_do_f2fs_gc()
        |
        +-- gc_type 0: preserve current sbi->gc_mode
        +-- gc_type 1: temporary sbi->gc_mode = 2
        +-- gc_type 2: temporary sbi->gc_mode = 3
        |
        v
stock X683 f2fs_gc(sbi, sync, true, NULL_SEGNO)
        |
        +-- stock victim filtering
        +-- stock SSR / greedy / CB cost
        +-- stock age/mtime
        +-- stock node/data migration
```

The most important new result is that `gc_type` is a persistent vendor configuration value with a three-value domain, while the `gc_mode` written by `tran_do_f2fs_gc()` is a temporary per-call override of the existing stock `f2fs_sb_info` field.

The vendor worker is not itself a replacement collector. It decides when to enter the collector, which controller path is active, and which existing stock GC mode should be exposed to the stock collector.

## 1. Exact vendor symbols and X683 addresses

Runtime addresses and Image offsets:

| Function | Runtime address | Image offset | Size | Role |
|---|---:|---:|---:|---|
| `tran_gc_thread_func` | `0xffffff92d0df6ed0` | `0x376ed0` | `0xd60` | main Transsion GC worker |
| `need_switch_ssr_read` | `0xffffff92d0df87dc` | `0x3787dc` | `0x10c` | proc read of inverse SSR flag |
| `need_switch_ssr_write` | `0xffffff92d0df88e8` | `0x3788e8` | `0x130` | proc write of inverse SSR flag |
| `tran_urgent_gc_read` | `0xffffff92d0df8b30` | `0x378b30` | `0x118` | proc read of urgent flag |
| `tran_urgent_gc_write` | `0xffffff92d0df8c48` | `0x378c48` | `0x1e4` | proc write/start-stop urgent worker |
| `tran_do_f2fs_gc` | `0xffffff92d0dfada8` | `0x37ada8` | `0xf0` | vendor GC wrapper |
| `tran_gc_init` | `0xffffff92d0dfae98` | `0x37ae98` | `0x44c` | registration/proc/wakeup-source init |
| `tran_gc_stop` | `0xffffff92d0dfb2e4` | `0x37b2e4` | `0x29c` | worker/registration teardown |
| `is_f2fs_fragmentation` | `0xffffff92d0dfb580` | `0x37b580` | `0x54` | fragmentation diagnostic helper |
| `tran_has_enough_free_segment` | `0xffffff92d0dfb5d4` | `0x37b5d4` | `0x178` | vendor free-space predicate |
| `usb_charge_event` | `0xffffff92d0dfabd0` | `0x37abd0` | — | charger/USB event callback |
| `fb_event` | `0xffffff92d0dfacf8` | `0x37acf8` | — | framebuffer blank/unblank callback |
| `should_do_origin_gc` | `0xffffff92d0dfad9c` | `0x37ad9c` | — | stock-GC enable flag reader |
| `enable_origin_gc` | `0xffffff92d0df6e30` | `0x376e30` | — | stock-GC enable flag writer |
| `gc_thread_create` | `0xffffff92d0df6e44` | `0x376e44` | — | vendor worker creator |
| `gc_thread_destroy` | `0xffffff92d0df7c30` | `0x377c30` | — | vendor worker stopper |

The addresses above come from the X683 kallsyms plus the decompressed stock Image mapping. No newer kernel API is used to identify them.

## 2. Complete direct-call graph around the Transsion GC subsystem

A full ARM64 BL-target scan was performed over the X683 kernel Image. The direct-call relationships relevant to this subsystem are:

```text
f2fs_start_gc_thread
    +0xd8
      -> tran_gc_init

f2fs_stop_gc_thread
    +0x18
      -> tran_gc_stop

tran_gc_thread_func
    +0x234
      -> tran_has_enough_free_segment
    +0x544
      -> tran_do_f2fs_gc

has_enough_free_seg_read
    +0x40
      -> tran_has_enough_free_segment

gc_thread_create / usb_charge_event / tran_urgent_gc_write / fb_event
    -> tran_gc_thread_func through function-pointer/event/kthread creation,
       not direct BL from an ordinary caller

should_do_origin_gc
    <- stock gc_thread_func +0x214

enable_origin_gc
    -> writes the flag consumed by should_do_origin_gc
```

No direct BL caller of `is_f2fs_fragmentation()` was found in the Image. That negative result is significant and is treated separately below. The worker does not call it directly.

Proc read/write handlers likewise are generally reached through proc-ops function pointers rather than ordinary BL call sites, so absence of direct BL callers is not evidence that the handler is unused.

## 3. `tran_do_f2fs_gc()` — exact mode policy

### 3.1 Instruction-level behavior

`tran_do_f2fs_gc()` has one direct caller: `tran_gc_thread_func + 0x544`, runtime `0xffffff92d0df7414`.

The decoded body is:

```c
++global[0x990];
cfg = global[0x998];

if (cfg == 0) {
    ret = f2fs_gc(sbi,
                  ((*(u32 *)(sbi + 0x4b8)) >> 14) & 1,
                  true,
                  NULL_SEGNO);
} else {
    old_mode = *(u32 *)(sbi + 0x534);

    if (cfg == 2)
        *(u32 *)(sbi + 0x534) = 3;
    else
        *(u32 *)(sbi + 0x534) = 2;

    ret = f2fs_gc(sbi,
                  ((*(u32 *)(sbi + 0x4b8)) >> 14) & 1,
                  true,
                  NULL_SEGNO);

    *(u32 *)(sbi + 0x534) = old_mode;
}

++global[0x9a0];
return ret;
```

The exact branch at `0xffffff92d0df6ed0 + 0x544 = 0xffffff92d0df7414` supplies `sbi` and calls this wrapper.

### 3.2 Proven `gc_type` domain

`gc_type_write()` at `0xffffff92d0df9a64` accepts a parsed integer and stores it into `global+0x998` only when the value is `<= 2`. Values above 2 are not installed.

`gc_type_read()` reads the same field and formats one of three table-backed strings. The string table names were not needed to establish the numeric semantics.

Therefore the proven state domain is:

| `global + 0x998` (`gc_type`) | Action inside `tran_do_f2fs_gc()` |
|---:|---|
| `0` | call stock `f2fs_gc()` without changing `sbi->gc_mode` |
| `1` | save `sbi->gc_mode`, set `sbi->gc_mode = 2`, call stock GC, restore old mode |
| `2` | save `sbi->gc_mode`, set `sbi->gc_mode = 3`, call stock GC, restore old mode |

This distinguishes **persistent controller configuration** (`gc_type`) from the **temporary stock GC mode override** (`sbi+0x534`).

### 3.3 What `gc_mode == 3` means in this X683 build

The stock X683 picker already uses `sbi+0x534 == 3` to bypass the non-foreground `max_victim_search` cap. The stock X683 `f2fs_need_SSR()` also returns true for `sbi+0x534 == 3`.

Therefore Transsion is not introducing a new victim algorithm here. It is deliberately exposing the stock urgent-mode semantics by setting the existing field to 3 for the duration of a GC call.

### 3.4 Temporary versus persistent

The value written to `sbi+0x534` by `tran_do_f2fs_gc()` is not persistent across wrapper calls. The original value is saved and restored unconditionally on the nonzero-config path.

`global+0x998` is persistent controller state until changed by its proc write path or explicitly restored by the worker's own end-of-run path.

Confidence: **HIGH** for all of the above.

## 4. `tran_gc_thread_func()` — complete reconstructed worker

The worker is created with `kthread_create_on_node()` and receives `global+0x8a0` (the `f2fs_sb_info *`) as its argument.

### 4.1 Entry/setup state

On entry the worker:

- snapshots `global+0x998` to its stack (`[sp+0x24]`), so the initial `gc_type` can be restored later;
- calls `set_freezable()`;
- increments `global+0x9e0`;
- clears `global+0x9f8` and `global+0x9fc`;
- captures a free/dirty-related value into `global+0x9f4`;
- acquires the vendor wakeup-source/spinlock context associated with `global+0x8b0` / `+0x8b8`;
- enters the wait/state loop.

The exact C type of the `+0x8b0` object is not proven; the call pattern is nevertheless enough to identify it as part of the vendor wakeup-source control context.

### 4.2 Main wait loop

A phase value is written to `global+0x9d4` (`1` at the beginning of the main wait iteration). The worker also writes the shared `should_do_origin_gc` flag at an adjacent image/global location (`Image/BSS +0x13c5000 + 0x158`) before the wait cycle.

It initializes a wait entry on `global+0x978`, calls `prepare_to_wait_event()`, and uses `schedule_timeout()` with a **250 ms** timeout in the normal polling loop.

The loop checks:

- `kthread_should_stop()`;
- freezer state via `freezing_slow_path()`;
- the stored wait result/event;
- `tran_app_wakelock_status()`;
- `global+0x9d0` (urgent-GC request/status);
- framebuffer state through `global+0x974` at later stages.

The waitqueue is explicitly finalized with `finish_wait()`.

### 4.3 Urgent flag gate and free-space decision

After the wait path, if `global+0x9d0 != 0`, the worker calls:

```c
tran_has_enough_free_segment(sbi)
```

at worker offset `+0x234`.

If the helper returns true, the worker calls `tran_get_charger_type()` and proceeds into the special path only when its result is exactly `1`. Otherwise the worker returns to the wait loop.

The special path eventually reaches the same `gc_type`/`gc_mode` machinery rather than a second physical GC engine.

### 4.4 Capacity/pressure path

When the free-space helper is not sufficient to terminate the decision path, the worker builds a pressure metric from the X683 segment-manager state:

- `sbi+0x3d8` = `log_blocks_per_seg`;
- `sbi+0x408` = `user_block_count`;
- `sm_info = *(sbi+0x80)`;
- `sit_info = *(sm_info+0x00)`;
- `free_info = *(sm_info+0x08)`;
- `dirty_info = *(sm_info+0x10)`;
- `dirty_info + 0x68..0x7c` = first six entries of `nr_dirty[8]`;
- `sm_info + 0x60` = `reserved_segments`.

The first metric is derived from segment units and is used alongside several additional threshold tests.

### 4.5 Proven threshold ladder

When `global+0xa00` is not already set, the worker executes several static threshold tests before it treats the state as a positive vendor fragmentation/capacity condition.

Directly decoded conditions include:

1. total dirty segments must exceed approximately **40%** of the user-segment population (`2/5` implemented by reciprocal arithmetic);
2. the computed pressure ratio must be at least **351** (`0x15f`);
3. a derived segment count must remain below/above a **25%** relationship to a free-segment-related field;
4. a current-user-segment relationship uses an explicit **13%** threshold;
5. `sbi+0x3f0` is compared against an explicit **27%** threshold.

All five are direct binary facts. The source-level names of the compared fields are not assigned where the X683 binary only exposes offsets.

If all of these conditions pass:

```text
global + 0xa00 = 1
global + 0xa04 = 1
```

Otherwise:

```text
global + 0xa00 = 2
```

The worker records additional status/type values in `+0x9f8` and `+0x9fc` on the paths that follow. Their telemetry names can be correlated to the proc namespace, but their exact internal semantic enum names are not proven and therefore remain offset-backed in this reconstruction.

### 4.6 Wake-lock gate

A later worker stage is explicitly guarded by `global+0xa05 == 1`. When that byte is not 1, control returns to the earlier state path.

When it is 1, the worker queries `tran_kernel_wakelock_status()`. If that returns zero, it queries `tran_app_wakelock_status()`. If the combined gate is false, the worker returns to the loop.

This establishes that wakelock state is part of the vendor **trigger/admission policy**. It does not alter victim scoring.

### 4.7 Entering the collector

Immediately before calling `tran_do_f2fs_gc()`, the thread:

1. observes vendor state/counter relationships;
2. performs the X683 threshold/state tests;
3. enters the superblock write section with `__sb_start_write()`;
4. attempts `mutex_trylock(sbi+0x508)` where `sbi+0x508` is the GC mutex;
5. calls `tran_do_f2fs_gc(sbi)` only after the trylock succeeds.

After the vendor wrapper returns it invokes `f2fs_balance_fs_bg()` and then releases the superblock write section.

There is no `tran_*` migration helper between `tran_do_f2fs_gc()` and the stock `f2fs_gc()` branch target.

### 4.8 Repeat, timing and termination state

Several counters/state bytes are maintained during the worker run:

| Global offset | Proven use |
|---:|---|
| `+0x990` | incremented once per `tran_do_f2fs_gc()` invocation |
| `+0x9a0` | incremented after the vendor GC call completes |
| `+0x9c8` | incremented on one skip/retry loop path |
| `+0x9d0` | urgent-GC control/request flag |
| `+0x9d4` | worker phase (`1..5` observed) |
| `+0x9e0` | worker-create count |
| `+0x9e8` | worker-destroy count |
| `+0x9f0` | recorded free-segment metric |
| `+0x9f4` | recorded startup segment/dirty-related metric |
| `+0x9f8` | worker telemetry/type state; values 1/2/3 observed |
| `+0x9fc` | worker status state; values 1/2/3 observed |
| `+0xa00` | capacity/fragmentation state (`1` or `2`) |
| `+0xa04` | positive threshold-trigger byte |
| `+0xa05` | wakelock/detect gate byte |
| `+0xa06` | post-threshold/urgent continuation byte |
| `+0xa08/+0xa0c` | remembered segment/count values used by modulo/repeat tests |
| `+0xa10` | last recorded segment metric |
| `+0xa18` | remembered value used in worker delta comparison |

The exact public proc-file name corresponding to every internal telemetry field is not always provable from the binary because handler descriptors and state fields are separate objects. The documentation therefore does not equate `+0x9f8`, `+0x9fc`, `+0xa00`, `+0xa05`, etc. to a public enum without direct producer/consumer proof.

### 4.9 End-of-run path

When the worker decides to leave the main GC cycle it:

- clears `+0xa08/+0xa0c`;
- restores the initial `global+0x998` value from `[sp+0x24]`;
- writes phase `+0x9d4 = 4` on the normal termination path;
- sets the shared origin-GC flag;
- increments `+0x9e8`;
- continues a timed wait/re-entry path as required by the state machine.

On the terminal/stop path it writes:

```text
global + 0x9d4 = 5
global + 0xa06 = 0
```

and performs the wakeup-source release/unlock sequence before returning.

## 5. `tran_urgent_gc_read()` / `tran_urgent_gc_write()`

### `tran_urgent_gc_read()`

`tran_urgent_gc_read()` is a conventional proc read callback.

It:

- returns 0 for nonzero `*ppos`;
- reads `global+0x9d0`;
- formats `"%d\n"`;
- copies to user space;
- returns `-EFAULT` if the copy fails.

Therefore `+0x9d0` is an externally visible integer urgent-GC control/status value.

### `tran_urgent_gc_write()`

The write handler parses an integer.

For a nonzero value:

```text
global+0x9d0 = 1
if worker inactive:
    create kthread(tran_gc_thread_func, global+0x8a0, -1, vendor name)
    store task pointer at +0x8a8
    mark +0x898 active on the observed success path
```

For zero:

```text
global+0x9d0 = 0
if worker active:
    set shared origin-GC flag
    kthread_stop(global+0x8a8)
    global+0x898 = 0
```

The compiler-generated error-pointer handling around `kthread_create_on_node()` includes an observed `wake_up_process()` path; this is recorded as raw control flow rather than being given a higher-level semantic label not proven by the binary.

Therefore the urgent proc control does **not** directly invoke `f2fs_gc()`. It controls the lifetime/request state of the vendor worker that eventually decides whether and how to invoke `tran_do_f2fs_gc()`.

Confidence: **HIGH**.

## 6. `tran_has_enough_free_segment()` — exact algorithm

Input is `sbi`.

The helper resolves:

```text
sm = *(sbi + 0x80)
sit = *(sm + 0x00)
free = *(sm + 0x08)
```

Then computes:

```text
L = sbi + 0x3d8
U = (sbi + 0x408) >> L
X = *(sit + 0x10)
F = *(free + 0x04)
R = *(sm + 0x60)
D = U - (X >> L)
```

The exact source member represented by `sit+0x10` is not proven and is intentionally left as an offset-based SIT field.

### Static threshold tables

The first table is:

```text
[2048, 3072, 4096, 4096, 100, 100, 100, 80]
```

The second table is:

```text
[80, 80, 80, 70, 70, 70, 60, 60]
```

A row selector is:

```text
A = max(*(u8 *)(global + 0x890),
        *(u8 *)(global + 0x894));
```

Rows above 7 select zero in the decoded fallback path.

### First gate

The base value is:

```text
B = 6144                       if (U >> 15) !=0
    first_table[U >> 13]      otherwise
```

Then the helper returns true when:

```text
F - R > first_table[A] * B / 100
```

Division by 100 is implemented by reciprocal multiply using `0x51eb851f`.

### Second gate

If the first gate fails:

```text
C = second_table[A]
threshold = C * D / 100
```

and the helper returns true when:

```text
F - R > threshold
```

The second divide-by-100 is implemented with a signed 64-bit reciprocal sequence.

If the second gate fails the function computes/logs another percentage-like diagnostic and returns false.

### Caller set

Direct binary evidence gives:

```text
tran_gc_thread_func + 0x234
has_enough_free_seg_read + 0x40
```

as callers.

Therefore this is a genuine vendor policy predicate used by the worker, and a read-only telemetry interface exposes the same predicate.

This is **not** the stock F2FS `has_not_enough_free_secs()` check. It is a separate Transsion free-segment policy using two static tables, reserved-segment subtraction, and a row selected from vendor global bytes.

Confidence: **HIGH** for arithmetic, thresholds, fields, and callers; **MEDIUM** for source-level names of the unresolved SIT field.

## 7. `is_f2fs_fragmentation()` — corrected interpretation

The body is only 0x54 bytes and is completely decoded.

It obtains the `sbi` from `global+0x8a0`, then:

```text
sm = *(sbi + 0x80)
sit = *(sm + 0x00)
free = *(sm + 0x08)
X = *(sit + 0x10)
F = *(free + 0x04)
D = sbi + 0x408 - X
U = D >> (sbi + 0x3d8)
pct_free = (F * 100) / U
fragmentation = 100 - pct_free
```

It passes the calculated fragmentation value to `printk` and then returns `0`.

This has two important consequences:

1. the function demonstrably computes a fragmentation percentage for logging;
2. its decoded return value is **zero**, so the function itself is not a boolean trigger gate in this X683 binary.

A full direct BL scan found no direct caller for `is_f2fs_fragmentation()`. The public proc handlers `is_fragmentation_read` / `is_fragmentation_write` exist, but the mapping between those proc handlers and this exact function is not a direct-call relationship.

Therefore the current evidence does **not** support the earlier hypothesis that `is_f2fs_fragmentation()` directly decides GC urgency. The safe conclusion is:

```text
fragmentation calculation -> diagnostic/telemetry path
not proven -> victim selection
not proven -> direct urgency boolean
```

Confidence: **HIGH** for the decoded arithmetic and return value; **MEDIUM** for the higher-level purpose because indirect callback usage cannot be excluded from a direct-BL scan alone.

## 8. Charger / USB / framebuffer events

### USB/charger

`usb_charge_event()` is a real vendor event callback.

On event `1` it checks the charger-detection state at `global+0x970`. When detection is not already satisfied, it calls `tran_get_charger_type()` and proceeds only if that function returns exactly `1`.

If the worker is inactive it creates:

```text
kthread_create_on_node(
    tran_gc_thread_func,
    global + 0x8a0,   /* sbi */
    -1,
    vendor thread name)
```

and stores the task pointer at `global+0x8a8`, with active state at `+0x898`.

On event `2`, if active, it sets the shared origin flag, calls `kthread_stop(+0x8a8)`, and clears `+0x898`.

Thus charging/USB events are **genuinely involved in worker lifetime policy**.

They do not directly modify `gc_mode` and do not directly call `f2fs_gc()`.

### Framebuffer

`fb_event()` handles `FB_EVENT_BLANK` (`event == 9`).

It reads the blank state and:

```text
state == 0 -> global+0x974 = 1
state == 4 -> global+0x974 = 0
```

When the worker is active, both transitions wake `global+0x978` with `__wake_up(..., mode=3, nr=1, key=0)`.

Thus display blank/unblank is a genuine **worker wake trigger**, not a direct GC-mode selector.

## 9. Origin-GC switch and relationship to stock F2FS GC thread

`enable_origin_gc()` writes a shared integer flag to zero or one.

`should_do_origin_gc()` returns the same flag.

The only direct caller found for `should_do_origin_gc()` is the stock `gc_thread_func()` at `+0x214`.

Therefore the vendor subsystem does not replace the stock GC worker. It also maintains a control flag that the original F2FS GC thread can consult.

This produces two related paths:

```text
stock F2FS GC thread
    -> should_do_origin_gc()
    -> shared origin-GC flag

Transsion GC worker
    -> tran_do_f2fs_gc()
    -> stock f2fs_gc()
```

The binary does not prove that the Transsion worker permanently disables the stock worker. It proves a shared gating flag exists and that vendor lifecycle paths set it during worker transitions.

## 10. Vendor state context

The exact C structure declaration/name for the vendor state object is not present in kallsyms. The binary proves a contiguous global state context spanning at least these offsets:

| Offset | Proven role | Confidence |
|---:|---|---|
| `+0x890` | threshold-table selector byte | High |
| `+0x894` | threshold-table selector byte | High |
| `+0x898` | worker-active flag | High |
| `+0x8a0` | `struct f2fs_sb_info *` | High |
| `+0x8a8` | worker task pointer | High |
| `+0x8b0` | wakeup-source/spinlock-associated object | Medium |
| `+0x8b8` | wakeup source object | High |
| `+0x968` | wakeup-source/debug enable bit | High |
| `+0x970` | charger-detection state/control byte | High |
| `+0x974` | framebuffer/blank state | High |
| `+0x978` | waitqueue | High |
| `+0x990` | GC invocation counter | High |
| `+0x998` | persistent `gc_type` 0..2 | High |
| `+0x9a0` | post-GC counter | High |
| `+0x9c0` | inverse `need_switch_ssr` control byte | High |
| `+0x9c8` | loop/skip counter | High |
| `+0x9d0` | urgent-GC control flag | High |
| `+0x9d4` | worker phase | High |
| `+0x9e0` | thread-create count | High |
| `+0x9e8` | thread-destroy count | High |
| `+0x9f0` | free-segment metric snapshot | High |
| `+0x9f4` | startup segment/dirty metric snapshot | Medium |
| `+0x9f8` | vendor telemetry/type state | Medium |
| `+0x9fc` | vendor status state | Medium |
| `+0xa00` | capacity/fragmentation state | Medium |
| `+0xa04` | positive threshold-trigger byte | High |
| `+0xa05` | wakelock/detect admission flag | Medium |
| `+0xa06` | post-threshold continuation byte | Medium |
| `+0xa08` | remembered count/segment field | Medium |
| `+0xa0c` | remembered current segment/free metric | Medium |
| `+0xa10` | last recorded segment metric | High |
| `+0xa18` | remembered value for GC-delta comparison | Medium |
| `+0xa20` | `/proc/tran_gc_debug` directory pointer | High |

No exact vendor structure size is claimed. The table is a field map for the proven live state context, not a guessed C declaration.

## 11. Proc/debug surface proved by `tran_gc_init()`

`tran_gc_init()` returns immediately unless `sbi+0x758 == 1`.

When enabled it:

1. stores `sbi` at `global+0x8a0`;
2. registers a SRCU notifier client;
3. registers a framebuffer notifier client;
4. creates `/proc/tran_gc_debug` and stores the directory at `+0xa20`;
5. creates the vendor GC proc files;
6. initializes `global+0x978` as a waitqueue;
7. prepares/adds the wakeup source associated with `+0x8b8`.

The exact proc entry names recovered from the X683 Image string table include:

```text
is_fragmentation
detect_wakelock
emmc_gc_time
need_switch_ssr
wake_up_detect_time
ssr_gc_times
data_movement
has_enough_free_seg
tran_urgent_gc
invalid_segment
static_pass_times
gc_skip_times
thread_destroy_times
thread_create_times
detect_charger_type
gc_to_static_detect_times
last_phase
total_segment
percent_of_free_segment
gc_segment_info
inc_gc_seg_threshold
dec_gc_seg_threshold
life_time
written_data
bad_block
lvdf
```

The complete handler-to-name mapping is intentionally not reconstructed where only proc-ops descriptors prove the association.

`tran_gc_stop()` tears down the worker, unregisters the notifier clients, removes the proc entries, and tears down the proc/debug context.

## 12. Policy model answering the phase questions

### What exactly causes Transsion GC to trigger?

There is not one single trigger. The binary shows a multi-stage admission model:

```text
USB/charger event, framebuffer event, explicit urgent-GC control,
worker wake/retry state
        |
        v
vendor worker active
        |
        +-- urgent flag + charger-type test
        +-- free-segment policy
        +-- dirty/segment threshold ladder
        +-- wakelock admission
        +-- repeated/delta/timing state
        |
        v
try GC mutex
        |
        v
tran_do_f2fs_gc()
```

### What distinguishes normal from urgent GC?

The vendor has a dedicated `global+0x9d0` urgent-GC flag and a special worker path. When the worker uses nonzero configured `gc_type`, `tran_do_f2fs_gc()` maps the persistent configuration to stock mode 2 or 3. Mode 3 is the strongest urgent semantic because the stock X683 picker uses it to bypass the search cap and the stock SSR function returns true.

The worker therefore distinguishes urgency primarily through **controller state and the chosen stock `gc_mode`**, not through a second migration engine.

### What exact conditions cause `gc_mode` changes?

`tran_do_f2fs_gc()` changes `sbi+0x534` only when `global+0x998 != 0`:

```text
gc_type 1 -> gc_mode 2
gc_type 2 -> gc_mode 3
```

It always restores the prior mode afterward.

The worker also writes `global+0x998` back to its entry value on its terminal state transition. No evidence shows a permanent vendor overwrite of `sbi+0x534`.

### Does fragmentation affect victim selection?

No direct evidence was found. The stock victim scorer remains intact. `is_f2fs_fragmentation()` itself computes/logs a percentage and returns zero; no direct worker call was found. The proven vendor fragmentation-related state is therefore an admission/diagnostic layer, not a replacement victim score.

### Does free-segment pressure affect mode selection?

Yes at the controller level, but not by rewriting the victim score. `tran_has_enough_free_segment()` gates the worker path. The actual mode override is then selected by `global+0x998` in `tran_do_f2fs_gc()`.

### Are charging/display/USB/power events involved?

Yes, but in the controller lifecycle:

- USB/charger events create/stop the worker;
- framebuffer blank/unblank wakes the worker;
- charger type can gate the urgent/special path;
- wakeup-source and wakelock status can gate admission.

No direct evidence shows those events modifying `sbi+0x534` themselves.

### What persistent state does Transsion maintain?

A global vendor state context maintains at least the task pointer, active flag, waitqueue, wakeup source, `gc_type`, urgent flag, SSR toggle, threshold selectors, phase/status bytes, counters, segment metrics, and proc directory pointer. The exact source-level struct name/size remains unresolved.

### Are there vendor modifications between `tran_do_f2fs_gc()` and stock victim/migration?

No new vendor migration/cost helper was found. The direct wrapper immediately calls the stock four-argument `f2fs_gc()`. The previously completed binary proof still stands: victim filtering, scoring, SSR, age/mtime, and node/data migration remain stock-like.

## 13. Genuine Transsion modifications versus stock F2FS

### Proven Transsion-owned policy/control

- `tran_gc_thread_func()` worker state machine.
- explicit urgent-GC request/worker lifetime control.
- USB/charger-triggered worker creation/destruction.
- framebuffer wakeups.
- vendor wakelock admission gates.
- `tran_has_enough_free_segment()` two-stage free-segment predicate and its threshold tables.
- persistent `gc_type` 0..2 control.
- temporary `gc_mode` override in `tran_do_f2fs_gc()`.
- vendor counters/telemetry/proc surface.
- vendor worker phase and repeated/delta state.

### Stock X683 F2FS behavior retained

- four-argument `f2fs_gc()` ABI;
- victim filtering and `get_victim_by_default()`;
- SSR/greedy/cost-benefit scoring;
- age/mtime cost-benefit formula;
- stock `gc_mode == 3` implications inside the collector;
- stock node/data migration engine;
- stock collector retry/checkpoint/cleanup architecture.

### Unresolved or explicitly not claimed

- exact source-level names of all vendor global fields;
- exact formal vendor struct declaration and total size;
- exact strings/enums represented by some telemetry fields;
- exact indirect proc-handler mapping for every debug node;
- exact source-level identity of the SIT field at `sit+0x10` used by the free/fragmentation helpers;
- whether a remaining indirect call path invokes `is_f2fs_fragmentation()` as a callback.

## 14. Final reconstructed state machine

```text
                         +----------------------+
                         | external/event input |
                         |----------------------|
                         | USB/charger          |
                         | framebuffer          |
                         | proc urgent_gc       |
                         | proc/configuration   |
                         +----------+-----------+
                                    |
                                    v
                         +----------------------+
                         | vendor GC state      |
                         |----------------------|
                         | worker active        |
                         | waitqueue            |
                         | urgent_gc             |
                         | gc_type 0..2         |
                         | SSR toggle           |
                         | threshold selectors  |
                         | wakelock gates       |
                         | phase/counters       |
                         +----------+-----------+
                                    |
                                    v
                    +---------------+----------------+
                    | worker admission / policy     |
                    |-------------------------------|
                    | free-segment predicate        |
                    | dirty/segment thresholds      |
                    | charger-type checks           |
                    | wakelock checks               |
                    | timing/delta/retry state      |
                    +---------------+---------------+
                                    |
                                    v
                         +----------------------+
                         | GC mode selection    |
                         |----------------------|
                         | gc_type 0 -> old mode|
                         | gc_type 1 -> mode 2  |
                         | gc_type 2 -> mode 3  |
                         +----------+-----------+
                                    |
                                    v
                         +----------------------+
                         | tran_do_f2fs_gc()    |
                         | save/override/restore|
                         +----------+-----------+
                                    |
                                    v
                   f2fs_gc(sbi, sync, true, NULL_SEGNO)
                                    |
                                    v
                         +----------------------+
                         | stock X683 F2FS GC   |
                         |----------------------|
                         | victim filtering     |
                         | SSR/greedy/CB cost   |
                         | age/mtime            |
                         | node/data migration  |
                         | retry/checkpoint     |
                         +----------------------+
```

This model answers the central architectural question: the Transsion delta is a **policy/state machine around the stock collector**, with a temporary stock-mode override as its strongest direct influence on the core GC behavior.

## 15. Confidence summary

| Finding | Confidence |
|---|---|
| `tran_do_f2fs_gc()` exact `gc_type` 0/1/2 mapping | HIGH |
| temporary `gc_mode` save/override/restore | HIGH |
| worker wake/wait/timer structure | HIGH |
| 250 ms wait timeout in the main loop | HIGH |
| urgent flag at `+0x9d0` | HIGH |
| free-segment helper arithmetic/tables | HIGH |
| free-segment helper is vendor policy, not stock free-section check | HIGH |
| `is_f2fs_fragmentation()` computes percentage and returns 0 | HIGH |
| no direct BL caller for `is_f2fs_fragmentation()` | HIGH |
| USB/charger creates/stops worker | HIGH |
| framebuffer wakes worker | HIGH |
| wakelock state participates in admission | HIGH |
| full vendor global state object exists at least through `+0xa20` | HIGH |
| exact total vendor struct size | LOW / unresolved |
| exact source enum/string names for every telemetry field | MEDIUM/LOW |
| fragmentation helper may still be used indirectly | LOW / unresolved |
| hidden vendor victim/migration engine after wrapper | LOW probability; no binary evidence found |

## Phase conclusion

This phase closes the remaining directly visible Transsion GC policy/state-machine layer to the level supported by the X683 binary.

The durable conclusion is:

```text
Transsion changes trigger timing, worker lifetime, admission policy,
urgency state, and the stock GC mode exposed to f2fs_gc().

Transsion does not replace the proven stock victim scorer or migration engine.
```

The remaining unresolved items are source-naming/layout questions, not evidence of a missing second GC algorithm. Future work should proceed to the remaining X683 F2FS integration/layout areas rather than reopening victim-selection or migration unless new binary evidence contradicts this result.
