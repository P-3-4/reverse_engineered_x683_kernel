# X683/H694 GC subsystem — full synthesis

Date: 2026-08-18

Binary authority: uploaded stock `boot(8).img` and standalone compressed kernel.

Verified:

```text
boot SHA-256  = a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180
Image SHA-256 = 96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba
Image size    = 26,615,820 bytes
```

## 1. Semantic reconstruction of the seven-field I/O gate

The seven `f2fs_sb_info` fields consumed by `0x366cd4` are now tied to the X683 statistics-copy path:

```text
sbi +0x444 -> stat +0x0c0  nr_wb_cp_data
sbi +0x448 -> stat +0x0c4  nr_wb_data
sbi +0x44c -> stat +0x0c8  nr_rd_data
sbi +0x450 -> stat +0x0cc  nr_rd_node
sbi +0x454 -> stat +0x0d0  nr_rd_meta
sbi +0x458 -> stat +0x0d8  nr_dio_write
sbi +0x45c -> stat +0x0d4  nr_dio_read
```

Therefore the policy discriminator is an **I/O activity gate**.

Exact branch behavior for `gc_mode != 3`:

```text
any nonzero I/O counter -> 0x366da4 active guarded path
all seven zero          -> 0x366ee0 clean/alternate path
```

`gc_mode == 3` bypasses the discriminator and enters the common stage at `0x366de4`.

This changes the interpretation of the policy substantially: the gate is not seven proprietary GC booleans. It asks whether relevant writeback/read/direct-I/O statistics are active.

### Runtime writers proven

```text
0x32b0e0 -> [sbi+0x450]
0x338f58 -> [sbi+0x450]
0x338f5c -> [sbi+0x448]
0x338f70 -> [sbi+0x458]
```

Initialization at `0x344efc..0x344f14` clears all seven fields.

The remaining fields have not yet acquired a binary-proven non-initialization writer:

```text
+0x444 +0x44c +0x454 +0x45c
```

Raw offset matches in unrelated structures were excluded by base-register provenance.

## 2. Helper semantics around `0x366cd4`

### `0x35cc18` — multi-mode boolean policy predicate

Signature at call sites:

```c
bool helper(struct f2fs_sb_info *sbi, unsigned int selector);
```

Selectors used by `0x366cd4` are `4, 1, 0, 1, 3`.

Dispatch table is at `Image+0xe74030+0x30` and maps selectors `0..5` to six distinct bodies.

The bodies are vendor policy tests over F2FS segment-manager state and X683 thresholds. No proprietary mode names are invented.

### `0x362c40` — vendor GC/segment execution path

Called as:

```text
0x362c40(sbi, 0, 0)
```

The function takes GC-related locks and performs dirty/free/victim segment work. It is a heavy vendor/F2FS path surrounding the actual four-argument stock X683 `f2fs_gc()` entry at `0x3503a8`.

Do not replace `0x3503a8` with `0x362c40`; both are real execution layers.

### `0x363288` — dirty/list drain helper

Called as:

```text
0x363288(sbi, 0xe38)
```

It validates segment-space state, locks the segment-manager list, drains entries from a linked list rooted at `sm_info+0x98`, updates the associated count, then unlocks. This is cleanup/pressure handling, not the main GC executor.

### `0x34e224` — strong `f2fs_balance_fs(sbi, bool)` candidate

The call from `0x366cd4` is:

```c
0x34e224(sbi, 1);
```

The two-argument ABI exactly matches the historical F2FS API:

```c
void f2fs_balance_fs(struct f2fs_sb_info *sbi, bool need);
```

Historical F2FS explicitly documents `f2fs_balance_fs()` as the routine that balances dirty node/dentry pages and controls GC. citeturn380364search0turn380364search1

Binary classification: **high-confidence public-symbol candidate; exact X683 body may be vendor-modified.**

### `0x341250` — superblock synchronization/checkpoint helper, exact symbol unresolved

Call:

```c
0x341250(sbi->sb, 1);
```

Historical F2FS has `f2fs_sync_fs(struct super_block *sb, int sync)`, and the ABI matches. citeturn472669search7

However, the X683 body at `0x341250` contains a vendor callback-list dispatch and X683-specific state/locking around the superblock, rather than the short public implementation. Therefore the safe classification is:

```text
X683 superblock synchronization/checkpoint/operation helper
public candidate: f2fs_sync_fs()
confidence: medium
```

The source symbol is not promoted as proven.

### `0x3e1014` / `0x3e1558`

These are a paired temporary GC/TLS object lifecycle around the stack object at `sp+0x8`:

```text
0x3e1014(stack_object) -> initialize/prepare
0x34e224(sbi,1)
0x3e1558(stack_object) -> reset/release
```

They are not the filesystem synchronizer themselves.

## 3. Final X683 `f2fs_stat_info` model

`sbi+0x568` points to the statistics object.

Allocator evidence gives an exact allocation size of `0x238` bytes.

Binary-confirmed GC/stat region:

```text
+0x160  prefree_count
+0x164  call_count
+0x168  cp_count
+0x16c  bg_cp_count

+0x170  tot_segs
+0x174  node_segs
+0x178  data_segs
+0x17c  free_segs
+0x180  free_secs
+0x184  bg_node_segs
+0x188  bg_data_segs

+0x18c  tot_blks
+0x190  data_blks
+0x194  node_blks
+0x198  bg_data_blks
+0x19c  bg_node_blks

+0x1a0/+0x1a8  skipped_atomic_files[2]

+0x1b0..0x1c4  curseg[6]
+0x1c8..0x1dc  cursec[6]
+0x1e0..0x1f4  curzone[6]

+0x1f8..0x218  copied SBI/meta/segment/block/inplace statistics
```

The `+0x16c` interpretation is now definitive:

```c
stat->bg_cp_count++;
```

Earlier project descriptions calling it `dirty_count` or a generic completion counter are superseded.

Historical F2FS statistics formatting also matches the observed binary strings for total/data/node segments and blocks, supporting the final field mapping.

### Tail

The final `+0x1b0..+0x218` region is a vendor/X683 statistics snapshot:

```text
curseg[6]
cursec[6]
curzone[6]
meta_count[META_MAX]
segment_count[2]
block_count[2]
inplace_count
```

The exact source member names for the last nine copied SBI fields remain source-tree dependent, but their offsets and producers are fixed.

## 4. `Image+0x16c6980` resolved as `jiffies`

The GC policy reads the same 64-bit global at:

```text
0x366e64
0x366f10
```

The global contains:

```text
08 db fe ff 00 00 00 00
= 0x00000000fffedb08
= low 32-bit value -75000
```

The whole Image contains a very large number of unrelated direct references to the same address, and embedded kernel strings explicitly include the symbol name `jiffies` nearby.

The decisive value match is:

```text
INITIAL_JIFFIES = -300 * HZ
```

Linux documents `INITIAL_JIFFIES` exactly this way. citeturn472669search0

For the X683 build's HZ=250:

```text
-300 * 250 = -75000
```

which exactly matches the Image data at `+0x16c6980`.

Kernel timer code initializes `jiffies_64` from `INITIAL_JIFFIES`. citeturn605116search2

Therefore:

```text
Image +0x16c6980 = jiffies_64 / jiffies backing storage
```

This is **high confidence** and supersedes the earlier neutral label `x683_shared_time_global_16c6980`.

The GC policy is therefore doing a jiffies-domain time comparison, not comparing against a Transsion-specific threshold.

### Exact GC-side arithmetic

Active/shared path:

```text
value = 250 * sbi+0x1c8 + sbi+0x198
return if value >= jiffies
```

Clean/alternate path:

```text
value = 250 * sbi+0x1d0 + sbi+0x1a0
value >= jiffies -> escalate to active path
value <  jiffies -> shared stage
```

This is consistent with the historical kernel's use of jiffies for absolute time comparisons. The raw binary now gives the exact global identity.

## 5. Complete reconstructed vendor GC path

The combined X683 execution model is:

```text
Transsion detector/thread
        |
        | state/arming/stop predicates
        v
controller +0x998
        |
        +--> 0 normal
        +--> 1 -> temporary gc_mode=2
        +--> 2 -> temporary gc_mode=3
        |
        v
tran_f2fs_gc() wrapper @ 0x37ada8
        |
        v
X683 four-argument f2fs_gc() @ 0x3503a8
        |
        +--> stock-style GC type selection
        +--> checkpoint/free-space checks
        +--> victim selection
        +--> section migration
        +--> block/segment statistics
        +--> repeat/checkpoint
        +--> cleanup/return
        |
        v
vendor post-GC policy @ 0x366cd4
        |
        +--> policy selector 4
        +--> policy selector 1
        +--> policy selector 0
        +--> optional vendor execution/cleanup
        |
        +--> gc_mode == 3 ?
        |       |
        |       +--> shared stage (urgent/alternate behavior)
        |       |
        |       +--> otherwise seven-field I/O activity gate
        |               |
        |               +--> any active I/O -> guarded capacity path
        |               +--> no active I/O  -> clean/alternate path
        |
        +--> fixed-point free/capacity guard
        +--> policy selector 1
        +--> policy selector 3
        +--> dirty/reservation comparison
        +--> repeated fixed-point guard
        +--> jiffies time gate
        |
        +--> terminal path
                |
                +--> optional TLS/GC-list lifecycle
                +--> f2fs_balance_fs(sbi, true) candidate
                +--> X683 superblock sync/checkpoint helper candidate
                +--> stat->bg_cp_count++
```

## 6. X683 vs historical 4.14 F2FS delta

### Stock-like core retained

The X683 `0x3503a8` GC core remains recognizably historical F2FS:

```text
GC type selection
-> mounted/checkpoint checks
-> free-section pressure
-> victim selection
-> summary access
-> node/data migration
-> merged writes
-> accounting
-> repeat/checkpoint
```

Historical 4.14 F2FS exposes the same four-argument GC entry family and the same general migration/accounting architecture. 

### X683 additions

```text
1. Transsion detector thread/state machine
2. five vendor stop predicates
3. controller state at +0x998
4. temporary gc_mode overrides
5. I/O-activity discriminator on seven SBI counters
6. multiple vendor fixed-point threshold tests
7. vendor dirty/list cleanup path
8. jiffies-based post-GC policy gate
9. optional balance/sync/checkpoint side effects
10. extended/statistics snapshot data
11. vendor debug/control registration
```

The strongest source-level model is therefore:

```text
historical F2FS GC core
        +
X683/Transsion detector/controller/policy layer
        +
X683 statistics/accounting extensions
```

not a wholesale rewrite of `fs/f2fs/gc.c`.

## 7. Sanity checks

### ABI

The wrapper and the core agree on:

```c
f2fs_gc(sbi, sync, background, segno)
```

with vendor wrapper `segno = NULL_SEGNO`.

### Statistics

The seven I/O counters are copied from SBI into the statistics object and simultaneously consumed by the policy gate. No unrelated structure is required to explain their behavior.

### Time reference

The `+0x16c6980` data value exactly matches `INITIAL_JIFFIES` at HZ=250, and the symbol string table contains `jiffies`. This is stronger than the earlier heuristic “shared time global” classification.

### Terminal accounting

The terminal increment at `+0x16c` is consistent with `bg_cp_count` and the X683 statistics print path. It must not be called `dirty_count`.

### Helper identities

Only the following have enough evidence for public-symbol candidates:

```text
0x34e224 -> f2fs_balance_fs(sbi, true)       high confidence
0x341250 -> f2fs_sync_fs(sb, 1)              medium confidence
```

The latter remains anonymous until its full X683 body is matched to the exact source revision.

### Remaining unresolved items

```text
+0x444/+0x44c/+0x454/+0x45c nonzero runtime writers
exact proprietary selector names for 0..5
exact X683 source names of the last copied statistics fields
exact source identity of 0x341250
exact names for a few vendor SBI threshold fields
```

These are source-attribution gaps, not unresolved GC control-flow gaps.

## Final assessment

The X683/H694 Transsion GC subsystem is now reconstructed at the architecture/control-flow/statistics level with high confidence.

The actual filesystem GC engine remains stock-derived F2FS with vendor orchestration wrapped around it. The biggest earlier ambiguities—the seven-field gate, the statistics counter at `+0x16c`, and the global at `Image+0x16c6980`—are now semantically resolved:

```text
seven fields = X683 I/O activity counters
stat+0x16c   = bg_cp_count
Image+0x16c6980 = jiffies_64/jiffies backing storage
```
