# X683/H694 F2FS 4.14 vendor delta — final v2

Binary authority: uploaded stock X683/H694 Image.

## Stock-derived core

The actual four-argument X683 `f2fs_gc()` entry remains at `0x3503a8`:

```c
f2fs_gc(sbi, sync, background, segno)
```

The core retains the historical F2FS architecture:

```text
GC type selection
-> checkpoint / mounted-state checks
-> free-space pressure handling
-> victim selection
-> summary access
-> node/data migration
-> write submission
-> segment/block statistics
-> repeat/checkpoint
-> cleanup
```

Historical Android/common 4.14 source uses the same four-argument family and standard F2FS GC/balance architecture.

## X683 vendor layers

### Detector/controller

The Transsion layer adds:

```text
controller state +0x998
cycle counter +0x990
stop state/result +0x9d4/+0x9f8/+0x9fc
periodic cadence +0xa04
progress baselines +0xa08/+0xa0c
```

The wrapper maps controller state to temporary internal `gc_mode` overrides.

```text
controller 1 -> gc_mode 2
controller 2 -> gc_mode 3
```

### I/O activity policy

The vendor post-GC gate at `0x366cd4` reads seven X683 I/O counters:

```text
writeback checkpoint data
writeback data
read data
read node
read metadata
direct-I/O write
direct-I/O read
```

Any active counter selects the guarded path; all zero selects the clean path. `gc_mode==3` bypasses the discriminator.

### Capacity/threshold policy

The policy includes repeated percentage/fixed-point checks using the reciprocal-multiply family `0x51EB851F >> 37`, reservation/current-segment comparisons, selector-mode predicates, and a vendor dirty/list cleanup branch.

### Time policy

The previously anonymous global at `Image+0x16c6980` is now resolved with high confidence as the kernel `jiffies_64/jiffies` backing variable:

```text
value at Image+0x16c6980 = 0x00000000fffedb08
low u32 = -75000
```

The X683 build uses HZ=250, and Linux defines:

```c
INITIAL_JIFFIES = -300 * HZ
```

so `-300 * 250 = -75000`. Linux initializes `jiffies_64` from `INITIAL_JIFFIES`. citeturn472669search0turn605116search2

Thus the vendor GC policy contains a real jiffies-domain time gate rather than a private Transsion timing global.

### Balance/synchronization side effects

The terminal policy path performs:

```text
optional temporary GC/TLS list lifecycle
-> 0x34e224(sbi, true)
-> 0x341250(sb, 1)
-> stat->bg_cp_count++
```

`0x34e224` is a high-confidence `f2fs_balance_fs(sbi, bool need)` candidate, consistent with the historical F2FS API and its documented role in balancing dirty pages and controlling GC. citeturn380364search0turn380364search1

`0x341250` has the exact `(super_block *, int)` shape of `f2fs_sync_fs()`, but its X683 body contains additional vendor callback/state machinery, so the public-symbol attribution remains medium confidence. Historical F2FS `f2fs_sync_fs()` uses that ABI and performs checkpoint work for `sync != 0`. citeturn472669search7

## Statistics delta

The X683 `f2fs_stat_info` object is `0x238` bytes and preserves the historical GC statistics family through at least `+0x218`, while adding an X683 snapshot of I/O/SBI fields.

Binary-backed fields include:

```text
+0x164 call_count
+0x168 cp_count
+0x16c bg_cp_count
+0x170 tot_segs
+0x174 node_segs
+0x178 data_segs
+0x17c free_segs
+0x180 free_secs
+0x184 bg_node_segs
+0x188 bg_data_segs
+0x18c tot_blks
+0x190 data_blks
+0x194 node_blks
+0x198 bg_data_blks
+0x19c bg_node_blks
+0x1b0..0x1f4 current segment/section/zone snapshots
+0x1f8..0x218 SBI statistics snapshot
```

The seven vendor policy fields are mirrored into the standard I/O statistics area:

```text
+0x444 -> nr_wb_cp_data
+0x448 -> nr_wb_data
+0x44c -> nr_rd_data
+0x450 -> nr_rd_node
+0x454 -> nr_rd_meta
+0x458 -> nr_dio_write
+0x45c -> nr_dio_read
```

## Final architecture

```text
                   X683/Transsion detector
                              |
                 stop conditions / controller
                              |
                              v
                   tran_f2fs_gc wrapper
                              |
                              v
                    X683 f2fs_gc @ 0x3503a8
                              |
            historical F2FS GC/victim/migration core
                              |
                              v
                   post-GC vendor policy
                           @ 0x366cd4
                              |
         +--------------------+--------------------+
         |                    |                    |
      I/O gate            fixed-point          jiffies gate
         |                 reservation             |
         |                 policy                  |
         +--------------------+--------------------+
                              |
                  optional balance/sync
                              |
                   bg_cp_count accounting
```

## Final delta conclusion

X683 does not replace F2FS GC wholesale. The binary shows a historical F2FS GC engine surrounded by a substantial Transsion orchestration/policy/statistics layer.

The most important vendor additions are:

```text
controller-driven gc_mode overrides
static detector/stop-condition engine
I/O-activity gate before/after GC
vendor fixed-point/reservation policy
jiffies-based time gate
balance/sync side effects
extended statistics snapshot/counters
vendor control registration
```
