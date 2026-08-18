# X683 / H694 Kernel Reverse-Engineering — Canonical Project Handoff

Last consolidated: 2026-08-18

## Continuation rule

Work from `main` in `P-3-4/reverse_engineered_x683_kernel`.

**Binary authority:** uploaded stock X683/H694 `boot(8).img`, its standalone compressed kernel, and the decompressed Image. Public Android/Linux/F2FS source is comparison evidence only.

Verified binary identity:

```text
boot SHA-256  = a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180
Image SHA-256 = 96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba
Image size    = 26,615,820 bytes
```

## End-to-end GC architecture

```text
Transsion detector/thread
    -> freezer/per-CPU synchronization gate
    -> tran_f2fs_gc() wrapper @ 0x37ada8
    -> X683 four-argument f2fs_gc() @ 0x3503a8
    -> historical F2FS victim/migration/accounting core
    -> post-GC vendor policy @ 0x366cd4
    -> optional balance/sync side effects
```

Detector call site around `0x377410..0x37742c` proves that `tran_f2fs_gc(sbi)` returns successfully and is followed by `0x366cd4(sbi)`.

## `tran_f2fs_gc()` controller mapping

```text
controller 0 -> direct f2fs_gc(); gc_mode unchanged
controller 1 -> temporary sbi->gc_mode = 2
controller 2 -> temporary sbi->gc_mode = 3
```

Vendor mode values around the X683 implementation establish `2=URGENT`, `3=GREEDY`.

Therefore Stop-4/5 raw controller value `2` selects temporary `gc_mode=3`.

## `0x366cd4..0x366f2c` policy

`0x366f2c` is the canary-failure tail; next normal function begins at `0x366f30`.

Entry ladder:

```text
sbi+0x48 bit3 -> return
policy(sbi,4) false -> 0x373108(sbi,0x80)
policy(sbi,1) false -> 0x35d22c(sbi,455)
policy(sbi,0) true  -> 0x362c40(sbi,0,0)
policy(sbi,0) false -> 0x363288(sbi,0xe38)
```

For `gc_mode != 3`, the seven-field discriminator is:

```text
sbi+0x444  nr_wb_cp_data
sbi+0x448  nr_wb_data
sbi+0x44c  nr_rd_data
sbi+0x450  nr_rd_node
sbi+0x454  nr_rd_meta
sbi+0x458  nr_dio_write
sbi+0x45c  nr_dio_read
```

Semantics:

```text
any nonzero -> 0x366da4 active guarded path
all zero    -> 0x366ee0 clean/alternate path
gc_mode=3   -> bypass discriminator -> 0x366de4 shared stage
```

These semantics are established by the X683 statistics-copy routine at `0x375ed8..0x375f0c`.

Runtime SBI writers proven:

```text
0x32b0e0 -> +0x450
0x338f58 -> +0x450
0x338f5c -> +0x448
0x338f70 -> +0x458
```

All seven are zeroed during SBI initialization at `0x344efc..0x344f14`.

Remaining non-initialization writers not yet proven:

```text
+0x444 +0x44c +0x454 +0x45c
```

Offset-only matches in unrelated structures are excluded.

## Main policy stages

Active path `0x366da4`:

```text
obj = *(sbi+0x70)
scaled = obj[0x04] * obj[0x18] * 0x51EB851F >> 37
current = obj[0x80]
current < scaled AND sbi+0x434 < 8*sbi+0x3dc -> return
otherwise -> shared stage
```

Shared stage `0x366de4`:

```text
policy(sbi,1)
policy(sbi,3)
nested reservation/dirty comparison
second fixed-point check
jiffies-domain gate
```

Clean path `0x366ee0`:

```text
nested manager child+0x2090 != 0 -> active path
list+0x24 != 0 -> active path
else compare 250*(sbi+0x1d0)+(sbi+0x1a0) against jiffies
    >= jiffies -> active path
    <  jiffies -> shared stage
```

## `Image+0x16c6980`

This is now resolved with high confidence as the kernel `jiffies_64`/`jiffies` backing storage.

Image bytes:

```text
08 db fe ff 00 00 00 00
```

Low 32-bit value = `-75000`.

The X683 build uses HZ=250, and Linux defines:

```c
INITIAL_JIFFIES = -300 * HZ
```

so `INITIAL_JIFFIES = -75000`. Linux timer code initializes `jiffies_64` from that value. citeturn472669search0turn605116search2

Consumers in `0x366cd4` are at `0x366e64` and `0x366f10`.

The old neutral label `x683_gc_policy_global_16c6980` is superseded.

## Terminal path

At `0x366e7c`:

```text
if mount byte +0x4b9 bit7:
    0x3e1014(stack GC/TLS object)
    0x34e224(sbi,1)
    0x3e1558(stack GC/TLS object)

always after terminal entry:
    0x341250(sbi->sb,1)
    F2FS_STAT(sbi)->bg_cp_count++
```

`0x34e224` is a high-confidence `f2fs_balance_fs(sbi, true)` candidate. Historical F2FS exposes that exact API and describes it as dirty-page balancing/GC control. citeturn380364search0turn380364search1

`0x341250` has the `(super_block *, int)` ABI of `f2fs_sync_fs()`, but its X683 body includes additional vendor callback/state machinery; keep the public symbol as a medium-confidence candidate only. Historical F2FS `f2fs_sync_fs()` uses that ABI. citeturn472669search7

## `f2fs_stat_info` final model

```text
sbi+0x568 -> f2fs_stat_info *
allocation size = 0x238
```

Binary-backed core:

```text
+0x14c rsvd_segs
+0x150 overp_segs
+0x154 dirty_count
+0x158 node_pages
+0x15c meta_pages
+0x160 prefree_count
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
+0x1a0/+0x1a8 skipped_atomic_files[2]
+0x1b0..0x1c4 curseg[6]
+0x1c8..0x1dc cursec[6]
+0x1e0..0x1f4 curzone[6]
+0x1f8..0x218 SBI/meta/segment/block/inplace snapshot
```

Historical 4.14 structure ordering and debug output independently match the CP/GC/segment/block fields. citeturn906146search0turn906146search9

The likely memory-accounting tail at `+0x220/+0x228/+0x230` remains unpromoted because no direct X683 stat-base store was found there.

## Helper classification

```text
0x35cc18 = multi-mode boolean vendor policy predicate
0x362c40 = heavy vendor/F2FS GC execution path surrounding the core
0x363288 = dirty/list drain helper
0x373108 = vendor accounting/threshold helper behind mount-bit gate
0x3e1014 = temporary GC/TLS object init
0x3e1558 = temporary GC/TLS object reset
0x34e224 = f2fs_balance_fs(sbi,true) candidate, high confidence
0x341250 = f2fs_sync_fs(sb,1) candidate, medium confidence
```

Do not replace `0x3503a8` with `0x362c40`; the former is the actual four-argument X683 `f2fs_gc()` entry.

## X683-vs-stock 4.14 delta

The stock-derived GC core remains recognizable historical F2FS. X683 adds:

```text
Transsion detector/state machine
controller-driven gc_mode overrides
five stop conditions
I/O-activity discriminator
fixed-point/reservation policy
jiffies-based policy gate
vendor dirty/list cleanup
balance/sync side effects
extended statistics snapshot/counters
vendor debug/control registration
```

Therefore the correct reconstruction strategy is:

```text
historical X683-era F2FS gc.c core
        +
Transsion detector/controller/policy layer
        +
X683 statistics/control extensions
```

## Authoritative follow-up artifacts

```text
docs/reverse-engineering/x683-gc-synthesis-final-2026-08-18.md
docs/reverse-engineering/x683-gc-final-status-2026-08-18.md
docs/reverse-engineering/x683-stat-info-final-v2-2026-08-18.md
docs/reverse-engineering/x683-policy-field-producer-consumer-final-v2.md
docs/reverse-engineering/x683-vendor-delta-final-v2.md
fs/f2fs/tran_gc_policy_semantic_reconstructed.c
```

## Remaining real unknowns

```text
non-initialization writers for +0x444/+0x44c/+0x454/+0x45c
original semantic names for selector modes 0..5
exact public symbol for 0x341250
exact source names for a few vendor SBI threshold fields
final names of +0x220/+0x228/+0x230 if they are present in the exact X683 struct
exact source-tree integration/buildability
```

Binary control flow and the main vendor/statistics semantics are now considered high confidence.
