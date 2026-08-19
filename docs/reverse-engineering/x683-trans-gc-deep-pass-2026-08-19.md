# X683 Transsion F2FS GC deep reconstruction

Date: 2026-08-19
Target: stock Infinix X683/H694 kernel, Linux 4.14.141+.

## Authority

Primary authority is the decompressed stock X683 Image plus X683 kallsyms/config. Historical F2FS source is used only to identify the stock baseline and distinguish vendor behavior. `CONFIG_F2FS_TRAN_GC=y` is present in the recovered X683 config.

## Symbols reconstructed

```text
gc_thread_create          0xffffff92d0df6e44
gc_thread_destroy         0xffffff92d0df7c30
tran_gc_thread_func       0xffffff92d0df6ed0
tran_urgent_gc_read       0xffffff92d0df8b30
tran_urgent_gc_write      0xffffff92d0df8c48
tran_do_f2fs_gc           0xffffff92d0dfada8
tran_gc_init              0xffffff92d0dfae98
tran_gc_stop              0xffffff92d0dfb2e4
```

## 1. `tran_do_f2fs_gc()` — exact policy wrapper

The function starts by incrementing a 64-bit vendor counter at global offset `+0x990`.

It then reads the vendor GC type at `+0x998` and temporarily changes the normal X683 `sbi->gc_mode` at `sbi + 0x534`:

```text
vendor gc type == 0:
    call f2fs_gc() with the normal/default vendor-selected mode

vendor gc type == 2:
    temporarily force sbi->gc_mode = 3

other non-zero vendor gc type:
    temporarily force sbi->gc_mode = 2
```

The call uses:

```text
x0 = sbi
x1 = bit 14 of sbi->mount_opt.opt (+0x4b8)
x2 = 1
x3 = -1
```

and restores the original `sbi->gc_mode` afterward.

Therefore this is not a replacement for F2FS's collector. It is a **policy wrapper around the stock `f2fs_gc()` entry point**, selecting the GC mode and preserving the original mode after the operation.

The function also increments a second vendor counter at `+0x9a0` on the forced-mode path.

## 2. `tran_gc_thread_func()` — vendor thread architecture

The X683 thread is substantially larger than the stock 4.14 `gc_thread_func()`.

The stock baseline has the familiar sleep/wakeup → frozen/write-lock/idle checks → `f2fs_gc()` → `f2fs_balance_fs_bg()` pattern. Historical Android/common F2FS sources show this architecture directly. citeturn2search2turn2search13

X683 retains the stock collector and balance function but inserts a large policy/telemetry layer around them.

### Vendor state machine

The thread repeatedly:

1. Computes and logs storage/free-segment state.
2. Maintains vendor thresholds and timing state.
3. Detects charger/USB/display state.
4. Checks fragmentation and discard state.
5. Decides whether to skip GC, run normal GC, or escalate.
6. Calls `tran_do_f2fs_gc()` for vendor-selected GC mode.
7. Calls stock `f2fs_balance_fs_bg()` afterward in the normal GC path.
8. Updates counters/phase/state exported through the vendor sysfs attributes.

The thread has explicit branches for `tran_urgent_gc`, charger detection, fragmentation, discard activity, and the vendor `gc_type`.

## 3. Exact vendor decisions recovered from the main loop

### Free-segment threshold calculation

The thread calculates free/available segment quantities from the reconstructed F2FS segment-manager fields. It uses:

```text
sbi + 0x3d8
sbi + 0x3e0
sbi + 0x3e8
sbi + 0x3f0
sbi + 0x408
sbi + 0x444..0x45c
sbi + 0x4b8
sbi + 0x534
sbi + 0x568
sbi + 0x80 -> sm_info
```

The calculation combines the F2FS segment geometry, dirty counters, reserved/available space and discard state. It is therefore a **vendor free-space pressure model layered over stock F2FS accounting**, not a new segment allocator.

### Dirty-I/O pressure gate

The thread checks the seven X683 counters at:

```text
sbi + 0x444
sbi + 0x448
sbi + 0x44c
sbi + 0x450
sbi + 0x454
sbi + 0x458
sbi + 0x45c
```

and enters the vendor GC-pressure path when any relevant activity is non-zero.

These are the same counters previously established by `f2fs_balance_fs_bg()` and stats export, so the GC thread is consuming the same X683 IO-pressure telemetry rather than maintaining a duplicate structure.

### Discard-pressure gate

The thread explicitly follows:

```text
sm_info + 0x98 -> fcc_info
sm_info + 0xa0 -> dcc_info

dcc_info + 0x2090
```

A non-zero discard state causes the vendor GC path to defer/escalate rather than blindly run another GC operation.

This directly links the previously reconstructed `discard_cmd_control` layout to the Transsion GC policy.

## 4. Fixed-point threshold policy

The main loop contains several fixed-point calculations using the exact multiplier:

```text
0x51EB851F >> 37
```

This converts integer ratios/percent-like quantities without floating point.

One branch requires the computed GC pressure to exceed a threshold derived from the root-dentry-related quantity and another branch requires a percentage relationship involving `sbi` geometry.

A separate calculation uses:

```text
0x66666667
```

with a shift of 34, equivalent to a division-by-100 style fixed-point operation.

These calculations are **genuine vendor policy**, not stock F2FS `gc_thread_func()` mechanics.

## 5. 250/500-ms timing policy

The thread uses an explicit:

```text
250
```

and later chooses between:

```text
250
500
```

for its periodic policy evaluation.

This is separate from the normal F2FS GC thread's longer background sleep periods. The vendor loop therefore has a much faster internal decision cadence while still using the stock collector for actual segment migration.

## 6. Vendor state globals

The main binary establishes a vendor state block around the global base represented by the code as `0x1a130000`.

The following offsets are now useful stable names:

```text
+0x898  vendor GC state/counter used by thread lifecycle/statistics
+0x8a0  vendor kobject/task-related pointer
+0x8a8  vendor auxiliary pointer
+0x890  charger/display state byte A
+0x894  charger/display state byte B

+0x990  GC invocation counter
+0x998  gc_type
+0x9a0  forced/secondary GC counter
+0x9b0  vendor GC wake/event counter
+0x9b8  vendor GC event counter
+0x9c0  detect-wakelock / policy byte
+0x9c8  f2fs_status
+0x9d0  last_phase
+0x9d4  thread/phase state
+0x9d8  data_movement
+0x9e0  gc_segment_info
+0x9e8  inc_gc_seg_threshold
+0x9f0  current computed free-segment metric
+0x9f4  threshold input used by the policy calculation
+0x9f8  vendor decision/result state
+0x9fc  vendor counter/threshold state
+a00    GC decision state: 0/1/2 are used as policy results
+a04    urgent-GC boolean
+a05    charger/display mode byte
+a06    vendor phase/decision byte
+a08    invalid-segment position/value
+a0c    invalid-segment companion value
+a10    cached F2FS free/segment value
+a18    previous vendor timing/counter value
+a20    vendor GC thread task pointer
+a88    enclosing vendor object pointer used for lifetime checks
```

Offsets whose semantic names are not independently proven are intentionally described as state/counter rather than being forced into a source-level struct member name.

## 7. Sysfs control surface proves the intended vendor model

The kernel exposes these vendor GC attributes:

```text
tran_gc_debug
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
gc_times
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
```

The existence and consumers of these attributes establish that the vendor GC implementation is not merely a different `gc_mode` value: it has persistent telemetry, thresholds, charger/display awareness, fragmentation detection, urgent GC, SSR switching and lifecycle accounting.

## 8. `tran_urgent_gc`

The write handler parses the userspace value and stores a boolean at global `+0xa04`.

The GC thread reads this byte directly in its main loop.

Therefore:

```c
tran_urgent_gc = true
```

is a direct asynchronous trigger/control flag for the vendor GC policy.

The read handler returns the same state.

## 9. `tran_gc_init()`

Initialization is conditional on an X683 state field at `sbi + 0x758`.

The function then:

1. Stores the current context in global `+0x8a0`.
2. Creates/initializes the vendor GC control object.
3. Starts the vendor GC task/thread.
4. Stores the resulting task pointer at `+0xa20`.
5. Registers a large set of vendor sysfs attributes.
6. Initializes the vendor state block.

The attribute registration names correspond exactly to the strings in the X683 Image, including `tran_gc_debug`, `tran_urgent_gc`, `gc_type`, `data_movement`, threshold controls, and lifecycle counters.

## 10. `tran_gc_stop()`

Shutdown is the inverse lifecycle operation:

```text
check vendor state
→ stop vendor task
→ destroy/free associated object(s)
→ clear task/object pointers
→ reset lifecycle state/counters where required
→ unregister vendor attributes
```

The function consumes the same global state block and `sbi` GC fields, confirming that `+0xa20` is the vendor thread/task pointer and that the vendor object lifetime is tied to the F2FS mount lifecycle.

## 11. `gc_thread_create()` / `gc_thread_destroy()`

The X683 keeps the normal F2FS GC-thread entry points but the vendor implementation introduces its own `tran_gc_thread_func` and lifecycle wrappers.

The standard F2FS baseline uses a `struct f2fs_gc_kthread`, initializes its wait queue, and starts a kernel thread; stopping the thread calls `kthread_stop()` and releases the allocation. citeturn2search2turn2search13

The X683 vendor layer adds its own state/attribute/lifecycle bookkeeping around that baseline.

## 12. Genuine Transsion delta vs stock F2FS

### Stock F2FS retained

```text
f2fs_gc()
segment migration
victim selection
GC mode field in sbi
sbi->gc_mutex / GC thread infrastructure
f2fs_balance_fs_bg()
segment-manager accounting
SIT/free/dirty/curseg structures
flush/discard control objects
```

The normal F2FS GC thread architecture and `f2fs_gc()` entry point are present in the stock baseline. citeturn2search2turn2search13

### Genuine X683/Transsion additions

```text
tran_gc_thread_func
tran_do_f2fs_gc
tran_gc_init
tran_gc_stop
tran_urgent_gc_{read,write}
charger/display state detection
fragmentation policy
urgent GC flag
GC type selection
SSR switching state
free-segment pressure model
fixed-point threshold calculations
GC skip counters
GC lifecycle counters
GC timing/wakelock telemetry
sysfs control/telemetry surface
additional IO/discard-pressure gates
```

### Most important architectural conclusion

Transsion did **not** replace the F2FS collector wholesale.

The architecture is:

```text
                    ┌─────────────────────┐
                    │ tran_gc_thread_func  │
                    └──────────┬──────────┘
                               │
                 vendor policy/telemetry
                               │
          ┌────────────────────┼──────────────────┐
          │                    │                  │
      skip/defer          urgent/SSR        normal GC
          │                    │                  │
          └────────────────────┼──────────────────┘
                               │
                       tran_do_f2fs_gc()
                               │
                     temporary gc_mode
                               │
                         f2fs_gc()
                               │
                    stock F2FS migration
                               │
                    f2fs_balance_fs_bg()
```

That is the core Transsion delta we needed to recover.

## 13. Phase completion status

This phase is considered complete at the architecture level.

We have recovered:

- vendor entry points;
- vendor thread architecture;
- vendor GC-mode wrapper;
- urgent-GC control path;
- initialization/shutdown lifecycle;
- major vendor global state offsets;
- free-space pressure model;
- IO/discard pressure gates;
- fixed-point threshold calculations;
- charger/display/fragmentation/SSR policy involvement;
- relationship to stock `f2fs_gc()` and `f2fs_balance_fs_bg()`;
- vendor sysfs control/telemetry surface.

The remaining unknowns are individual semantic names for some telemetry globals, not missing GC architecture.

## Next phase

Do not continue expanding `sm_info` or the standard F2FS child structures.

The next reconstruction phase should target **victim selection and migration policy**, specifically:

```text
select_policy()
get_victim_by_default()
get_victim_by_age()
get_victim_by_cost()
get_victim_by_ssr()
GC migration mode selection
victim-segment filtering
Transsion modifications to victim scoring
```

The objective is to determine exactly where the vendor policy feeds into stock victim selection and migration, and whether `CONFIG_F2FS_TRAN_GC` modifies scoring, selection, or only the outer trigger policy.
