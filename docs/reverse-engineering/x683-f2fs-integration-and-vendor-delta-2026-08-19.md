# X683 F2FS Integration and Vendor Delta — 2026-08-19

## 1. Executive conclusion

This phase closes the remaining X683 F2FS integration work around the already-completed segment-manager reconstruction and Transsion GC policy reconstruction.

Direct X683 binary evidence establishes the following boundary:

```text
f2fs_start_gc_thread()
        |
        +--> tran_gc_init()
        |
        v
   vendor worker lifecycle
        |
        v
tran_gc_thread_func()
        |
        +--> tran_has_enough_free_segment()
        |
        +--> policy / threshold / wakelock / charger gates
        |
        +--> mutex_trylock(sbi + 0x508)
        |
        v
tran_do_f2fs_gc(sbi)
        |
        +--> persistent global +0x998 selects temporary mode
        |
        v
f2fs_gc(sbi, sync, true, NULL_SEGNO)
        |
        +--> stock X683 GC state machine
        +--> __get_victim()/get_victim_by_default()
        +--> stock scoring / SSR / CB / age
        +--> stock node/data migration
        +--> freed-segment accounting
        +--> retry/checkpoint/cleanup
```

The exhaustive direct ARM64 `BL` scan of the decompressed X683 Image found **no `tran_*` call between `tran_do_f2fs_gc()` and any downstream stock GC helper**. The only direct calls made by `tran_do_f2fs_gc()` are the stock `f2fs_gc()` calls. No vendor migration engine, vendor victim scorer, vendor checkpoint helper, or vendor dirty-list mutation helper was identified in the downstream GC body.

This strengthens the earlier conclusion: Transsion's demonstrated F2FS delta is concentrated above the stock collector, in scheduling/admission and temporary `gc_mode` selection. The downstream stock collector remains the boundary after `tran_do_f2fs_gc()`.

The binary itself identifies the kernel as Linux `4.14.141+`, built with Android clang 9.0.3 on 2021-11-05. The exact vendor source git revision is not recoverable from the supplied binary alone. The closest public comparison remains the Android/Linux 4.14-era F2FS implementation; later public implementations must not be substituted for binary facts.

Confidence: HIGH for the vendor/stock boundary and four-argument call ABI; MEDIUM for source-level helper naming where helpers are inlined; LOW for unresolved vendor field names that are only observable as offsets.

## 2. Complete vendor F2FS/GC call graph

### 2.1 Direct symbol graph

The X683 kallsyms contains the following F2FS/GC-related Transsion symbols:

```text
tran_get_lifetime
tran_gc_thread_func
tran_urgent_gc_read
tran_urgent_gc_write
gc_thread_create
gc_thread_destroy
tran_do_f2fs_gc
tran_gc_init
tran_gc_stop
tran_has_enough_free_segment
has_enough_free_seg_read
need_switch_ssr_read
need_switch_ssr_write
gc_type_read
gc_type_write
usb_charge_event
fb_event
should_do_origin_gc
enable_origin_gc
```

`f2fs_start_gc_thread` and `f2fs_stop_gc_thread` are stock-named entry points but contain the Transsion integration calls.

### 2.2 Proven direct callers

```text
f2fs_start_gc_thread +0xd8
    -> tran_gc_init

f2fs_stop_gc_thread +0x18
    -> tran_gc_stop

tran_gc_thread_func +0x234
    -> tran_has_enough_free_segment

tran_gc_thread_func +0x544
    -> tran_do_f2fs_gc

has_enough_free_seg_read +0x40
    -> tran_has_enough_free_segment

tran_do_f2fs_gc +0x58 / +0x94 / +0xc4
    -> f2fs_gc

tran_gc_thread_func +0x55c
    -> f2fs_balance_fs_bg
tran_gc_thread_func +0xc94
    -> f2fs_balance_fs_bg

gc_thread_func +0x214
    -> should_do_origin_gc
```

The stock `f2fs_gc()` has ten direct callers in the supplied Image. Three are the three branches inside `tran_do_f2fs_gc()`; the other seven are stock F2FS paths including the normal GC thread, balance/disable-checkpoint paths and ioctl paths.

### 2.3 Worker creation/event paths

`gc_thread_create` has no ordinary direct `BL` caller because it is used as a worker/event-facing helper. The binary shows it creating the Transsion kthread using `kthread_create_on_node()` with `tran_gc_thread_func` as the start routine and the vendor `f2fs_sb_info *` state as the argument, then waking it with `wake_up_process()`.

`tran_urgent_gc_write`, `usb_charge_event`, and `fb_event` participate in worker control through callback/event registration and wake/stop logic rather than ordinary direct calls to `tran_gc_thread_func`.

The absence of direct `BL` references for callback handlers is therefore not treated as proof of inactivity. The callback relationships are supported by the registration/event code and state accesses, while the exact callback container layout is not promoted to a guessed C type.

### 2.4 Initialization and teardown

`f2fs_start_gc_thread` calls the vendor initializer after the stock GC thread state is prepared. `tran_gc_init` stores the supplied `f2fs_sb_info *` at global `+0x8a0`, creates the vendor proc directory at `+0xa20`, creates the vendor proc entries, initializes the vendor waitqueue at `+0x978`, and initializes the wakeup-source context around `+0x8b0/+0x8b8`.

`f2fs_stop_gc_thread` calls `tran_gc_stop`. The teardown checks `global +0x898`; if active it obtains the worker task at `+0x8a8`, stops it, clears the active flag, removes the proc hierarchy, and releases the stored resources.

## 3. Hidden downstream modification check

The downstream region from `f2fs_gc` through the migration/cleanup area was inspected by direct ARM64 branch-target analysis.

### 3.1 `f2fs_gc()` boundary

X683 address: `0xffffff92d0dd03a8`.

The Transsion wrapper calls this exact symbol with:

```text
x0 = sbi
w1 = ((sbi + 0x4b8) >> 14) & 1
w2 = 1
w3 = -1                 // NULL_SEGNO
```

No vendor function is called between wrapper entry and the stock `f2fs_gc()` entry.

### 3.2 Victim selection

`get_victim_by_default` is standalone at `0xffffff92d0dd2e74` and has no direct `BL` callers because the selector is reached through the stock victim-selection operation structure/function-pointer path.

The previously completed victim phase established that the X683 implementation retains stock dirty-list locking, policy selection, SSR/LFS distinction, search limits, victim bookkeeping, SIT filtering and minimum-cost selection. This phase did not reopen those algorithms; it verified that no Transsion function is inserted between the wrapper and the stock selector.

### 3.3 Migration and cleanup

The X683 `f2fs_gc()` body contains the historical migration sequence and direct calls to stock F2FS helpers. The kallsyms contains no vendor `tran_*` migration/data/node-GC helper in the relevant F2FS region. Direct `BL` target scanning likewise found no vendor target in the downstream body.

The logical historical helpers `do_garbage_collect()` and `gc_data_segment()` are not separately exported in kallsyms and their logical boundaries are represented by inlined code in the X683 `f2fs_gc()` body. They must therefore remain documented as logical/source-correlated boundaries, not falsely declared standalone X683 functions.

### 3.4 Result

**Conclusion A applies:** no downstream Transsion modification was found between `tran_do_f2fs_gc()` and the stock victim-selection/migration/freeing path.

This is a binary-evidence conclusion, not an assumption from symbol names.

Confidence: HIGH for direct-call absence; MEDIUM for indirect callback absence because optimized function-pointer relationships cannot all be reduced to direct `BL` references.

## 4. Vendor global-state field table

The vendor state is not proven to be one conventional C structure. The following is an offset map only.

| Offset | Access observed | Current identification | Confidence |
|---:|---|---|---|
| `0x890` | byte reads | selector used by free-segment policy | HIGH |
| `0x894` | byte reads | selector used by free-segment policy | HIGH |
| `0x898` | u32 read/write | worker active flag | HIGH |
| `0x8a0` | pointer read/write | `f2fs_sb_info *` | HIGH |
| `0x8a8` | pointer read | worker `task_struct *` | HIGH |
| `0x8b0` | address passed to init/locking | wakeup-source control context | MEDIUM |
| `0x8b8` | address passed to wakeup-source helpers | wakeup source object | HIGH |
| `0x968` | byte reads | vendor wakelock/detection control byte | HIGH |
| `0x970` | external/event-side state | charger/USB control state | HIGH |
| `0x974` | u32 reads | framebuffer/blank state | HIGH |
| `0x978` | address passed to wait APIs | waitqueue | HIGH |
| `0x990` | u64 increment | GC invocation counter | HIGH |
| `0x998` | u32 read/write | persistent `gc_type`, domain 0..2 | HIGH |
| `0x9a0` | u64 increment | post-GC invocation counter | HIGH |
| `0x9b0` | u64 increment | vendor retry/static-detect counter | MEDIUM |
| `0x9b8` | u64 increment | vendor special-path counter | MEDIUM |
| `0x9c0` | byte reads | inverse `need_switch_ssr` state | HIGH |
| `0x9c8` | u64 read/write | remembered retry/state value | MEDIUM |
| `0x9d0` | u32 read/write | urgent-GC state | HIGH |
| `0x9d4` | u32 writes | worker phase | HIGH |
| `0x9d8` | u64 read/write | remembered worker metric/state | MEDIUM |
| `0x9e0` | u64 increment | worker-create count | HIGH |
| `0x9e8` | u64 increment | worker-destroy count | HIGH |
| `0x9f0` | u32 read/write | free-segment metric | HIGH |
| `0x9f4` | u32 read/write | startup segment/dirty metric | MEDIUM |
| `0x9f8` | u32 read/write | telemetry/type state | MEDIUM |
| `0x9fc` | u32 read/write | status state | MEDIUM |
| `0xa00` | u32 read/write | capacity/fragmentation decision state: `1` positive, `2` negative | HIGH |
| `0xa04` | byte read/write | positive threshold-trigger byte | HIGH |
| `0xa05` | byte read | wakelock/detect admission gate | HIGH |
| `0xa06` | byte read/write | continuation flag | HIGH |
| `0xa08` | signed/u32 read/write | remembered metric | MEDIUM |
| `0xa0c` | u32 read/write | remembered metric | MEDIUM |
| `0xa10` | u64 read/write | last observed segment metric | HIGH |
| `0xa18` | u64 read/write | remembered GC/delta metric | MEDIUM |
| `0xa20` | pointer read/write | proc directory pointer | HIGH |

The earlier state map omitted `+0x8b0`, `+0x890/+0x894`, `+0x9b0/+0x9b8`, `+0x9c8`, and `+0x9d8` because their source-level names were not needed by the previous policy phase. They are now retained explicitly as unresolved/offset-backed state rather than being silently assigned conventional member names.

## 5. `f2fs_sb_info` integration

The following fields are directly connected to the vendor GC path:

| `sbi` offset | X683 identification | Consumer |
|---:|---|---|
| `0x80` | `sm_info *` | worker and stock segment accounting |
| `0x3d8` | `log_blocks_per_seg` | vendor pressure/free-segment arithmetic |
| `0x3dc` | `blocks_per_seg` | stock F2FS arithmetic |
| `0x3e0` | `segs_per_sec` | stock section arithmetic |
| `0x408` | `user_block_count` | vendor segment-population arithmetic |
| `0x428` | `reserved_blocks` | stock accounting |
| `0x430` | `current_reserved_blocks` | stock accounting |
| `0x438` | `unusable_block_count` | stock accounting |
| `0x440` | `nquota_files` | stock structure correlation |
| `0x4b8` | `mount_opt.opt` | vendor GC sync bit extraction |
| `0x508` | `gc_mutex` | vendor admission `mutex_trylock()` |
| `0x534` | `gc_mode` | vendor temporary override and stock picker |
| `0x538` | `next_victim_seg[0]` | stock victim bookkeeping |
| `0x53c` | `next_victim_seg[1]` | stock victim bookkeeping |
| `0x560` | `max_victim_search` | stock picker search limit |
| `0x564` | `migration_granularity` | stock migration |
| `0x568` | `stat_info *` | stock GC accounting |

The segment-manager graph is proven as:

```text
sbi + 0x80
    |
    v
f2fs_sm_info
    +0x00 -> sit_info
    +0x08 -> free_segmap_info
    +0x10 -> dirty_seglist_info
    +0x60 -> reserved_segments
    +0x98 -> flush_cmd_control
    +0xa0 -> discard_cmd_control
```

The reconstructed component sizes remain:

```text
f2fs_sm_info        0xA8
sit_info            0xA8
free_segmap_info    0x20
dirty_seglist_info  0x90
curseg_info         0x70 x 6
curseg array        0x2A0
discard_cmd_control 0x20B0
```

The worker directly consumes `dirty_info + 0x68..0x7c` as the first six `nr_dirty[8]` entries. It also consumes `sm_info + 0x60` as reserved segments and the `free_info` path for free segments. This establishes a live runtime pointer relationship, not just a copied historical layout.

The SIT field at `sit_info + 0x10` used by `tran_has_enough_free_segment()` remains unresolved at source-member-name level. The binary proves its use in the segment-unit arithmetic; it does not prove the historical C member name, so it remains offset-backed.

## 6. Exact vendor-to-stock boundary

| Boundary | X683 address | Inputs | Outputs/side effects | Confidence |
|---|---:|---|---|---|
| worker -> free-space predicate | `0xffffff92d0df7104` | `sbi` | boolean admission result | HIGH |
| worker -> GC mutex | `0xffffff92d0df7408` | `sbi+0x508` | trylock result | HIGH |
| worker -> vendor wrapper | `0xffffff92d0df7414` | `sbi` | GC return code | HIGH |
| vendor wrapper -> stock GC | `0xffffff92d0dfae00`, `+0x94`, `+0xc4` | `sbi`, sync, `true`, `-1` | stock GC result | HIGH |
| stock GC -> victim selection | `f2fs_gc @ 0xffffff92d0dd03a8` | stock GC state | victim | HIGH |
| victim -> migration | inlined/logical stock path | victim + summaries | migrated nodes/data | MEDIUM |
| migration -> freeing/checkpoint | inlined/logical stock path | segment state | free accounting/checkpoint | MEDIUM |

The only Transsion-controlled value crossing into the stock collector beyond normal `sbi` state is the temporary `gc_mode` change at `sbi+0x534`; the ABI arguments themselves remain stock-compatible.

## 7. Exact `tran_do_f2fs_gc()` behavior

At `0xffffff92d0dfada8`:

```c
global[0x990]++;
gc_type = global[0x998];

if (gc_type == 0) {
    f2fs_gc(sbi,
            ((sbi->mount_opt.opt >> 14) & 1),
            true,
            NULL_SEGNO);
} else {
    old_mode = sbi->gc_mode;
    sbi->gc_mode = (gc_type == 2) ? 3 : 2;
    f2fs_gc(sbi,
            ((sbi->mount_opt.opt >> 14) & 1),
            true,
            NULL_SEGNO);
    sbi->gc_mode = old_mode;
}

global[0x9a0]++;
```

The result is stored in a local 32-bit return register and returned. The original `gc_mode` is restored after the nonzero `gc_type` paths.

## 8. Vendor thresholds and constants

`tran_has_enough_free_segment()` at `0xffffff92d0dfb5d4` directly contains:

```text
selector = max(global + 0x890, global + 0x894)

A = [2048, 3072, 4096, 4096, 100, 100, 100, 80]
B = [80, 80, 80, 70, 70, 70, 60, 60]
```

It converts `user_block_count` into segment units using `log_blocks_per_seg`, converts the unresolved SIT quantity into segment units, and compares `free_segments - reserved_segments` against percentage-derived thresholds. The compiler implements division by 100 with reciprocal arithmetic in the binary.

The worker's pressure ladder directly contains:

```text
40% dirty-segment relationship
351 pressure threshold (0x15f)
25% relationship
13% relationship
27% comparison against sbi + 0x3f0
```

Passing the complete ladder writes:

```text
global + 0xa00 = 1
global + 0xa04 = 1
```

Failing it writes:

```text
global + 0xa00 = 2
```

The worker uses `global + 0xa05 == 1` as a later admission gate and checks kernel/app wakelock state before proceeding.

The main worker polling timeout is exactly `250 ms`. A later state path writes a `500 ms` timeout value into the worker-local timing state.

## 9. Investigated and disproven vendor modifications

### Separate vendor victim scorer

**Disproven for the direct downstream path.** No `tran_*` function is called by the X683 stock `f2fs_gc()`/victim path. The standalone `get_victim_by_default()` and its stock operation-table relationship remain intact.

### Separate vendor migration engine

**Disproven.** No vendor migration function appears in the downstream direct call-target scan. The logical node/data GC helpers are stock/inlined.

### Vendor modification of stock GC scoring

**Not found.** The completed victim-selection phase showed stock SSR/greedy/cost-benefit and age/mtime behavior, and this phase found no vendor downstream insertion point.

### Fragmentation as a direct boolean urgency gate

**Disproven at the direct-call level.** `is_f2fs_fragmentation()` computes/logs fragmentation and returns zero; no direct `BL` caller exists in the Image. An indirect callback relationship cannot be completely excluded, so the function remains documented as diagnostic/proc-related rather than an active proven gate.

### Permanent vendor `gc_mode` mutation

**Disproven.** `tran_do_f2fs_gc()` saves and restores `sbi+0x534` on every nonzero `gc_type` call. `gc_type` is persistent; `gc_mode` is temporary.

### Vendor replacement of stock free-section test

**Disproven.** `tran_has_enough_free_segment()` is an additional vendor predicate. The stock `f2fs_gc()` still contains its own stock free-section/retry logic.

## 10. Stock helper mapping

The X683 kallsyms gives these important standalone boundaries:

```text
f2fs_gc                 0xffffff92d0dd03a8
get_victim_by_default   0xffffff92d0dd2e74
f2fs_need_SSR           0xffffff92d0de58f8
f2fs_balance_fs         0xffffff92d0de6a48
f2fs_balance_fs_bg      0xffffff92d0de6cd4
```

The following historical helpers are not separately represented as standalone kallsyms functions in this build and are therefore treated as logical/inlined boundaries:

```text
__get_victim
 do_garbage_collect
gc_data_segment
gc_node_segment
segment-free accounting/retry fragments
```

This distinction matters: absence from kallsyms is not evidence that the algorithm is absent; it is evidence that the compiler did not preserve it as a separately named symbol in this image.

The closest historical public Android/Linux F2FS sources show the same conceptual chain from `f2fs_gc()` through `__get_victim()`/`get_victim_by_default()` into `do_garbage_collect()` and node/data migration. Those sources are comparison/naming baselines only. The X683 binary remains authoritative.

## 11. Source baseline

The supplied X683 Image contains the exact build identification:

```text
Linux version 4.14.141+ (nobody@android-build)
Android clang 9.0.3
#1 SMP PREEMPT
Fri Nov 5 15:56:25 CST 2021
```

Configuration also identifies MT6768 and `CONFIG_F2FS_TRAN_GC=y`.

Therefore the strongest source-baseline statement supported by the evidence is:

1. the stock kernel is based on Linux 4.14.141 with vendor additions/backports;
2. the F2FS GC implementation is an Android/vendor 4.14-era implementation;
3. the exact Transsion source revision cannot be named from the binary alone;
4. public Android/common 4.14 F2FS trees are valid comparison sources but are not proof of the exact vendor source revision;
5. the X683 four-argument `f2fs_gc(sbi, sync, background, segno)` call-site is authoritative even where a public source tree differs.

## 12. Integrated architecture

```text
                           X683 F2FS
                              |
              +---------------+----------------+
              |                                |
        stock F2FS state                 Transsion state
              |                                |
        f2fs_sb_info                  vendor GC context
              |                                |
        +-----+--------+              +--------+---------+
        |              |              |                  |
    sm_info         GC fields       events           policy state
        |              |              |                  |
   +----+----+      +--+--+       USB/charger       gc_type 0..2
   |    |    |      |     |       framebuffer        urgent flag
  SIT free dirty  mutex gc_mode    proc writes       thresholds
   |    |    |      |     |         wakeups           wakelocks
   |    |    |      +--+--+              |
   |    |    |         |                 |
   +----+----+---------+-----------------+
                        |
                 tran_gc_thread_func
                        |
                 admission / gating
                        |
                 tran_do_f2fs_gc
                        |
                 temporary gc_mode
                        |
                        v
             f2fs_gc(sbi,sync,true,-1)
                        |
               stock GC state machine
                        |
             +----------+-----------+
             |                      |
        victim selection        migration
             |                      |
      stock SSR/greedy/CB    stock node/data GC
      age/mtime/cost          accounting/checkpoint
             |                      |
             +----------+-----------+
                        |
                  retry / cleanup
```

## 13. Remaining genuinely unresolved questions

1. The formal C declaration and exact total size of the vendor global state remain unproven.
2. Source-level names for `+0x8b0`, `+0x9b0`, `+0x9b8`, `+0x9c8`, `+0x9d8` remain unresolved.
3. The exact historical C member corresponding to the SIT quantity at `sit_info + 0x10` remains unresolved.
4. Some proc-operation/callback containers are reached indirectly; the exact compiler/linker representation is not fully reconstructed as C structures.
5. The exact Transsion source git revision corresponding to the `4.14.141+` vendor tree is not present in the supplied binary metadata.
6. The logical boundaries of inlined migration helpers cannot be assigned exact standalone X683 addresses because they are not preserved as symbols.

None of these unresolved items currently weakens the proven vendor-to-stock GC boundary.

## 14. Recommended next phase

Move away from GC policy reconstruction and finish the broader X683 F2FS `f2fs_sb_info`/segment-manager runtime graph, including the remaining adjacent superblock fields and the exact pointer/lock relationships used by checkpointing, segment allocation, discard/flush, and mount/remount paths. Preserve the completed GC boundary as a fixed reference point.
