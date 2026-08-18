# X683/H694 F2FS Phase 1 — Binary Findings Final 2026-08-18

Binary authority: X683/H694 `boot(8).img`; decompressed Image SHA-256 `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`.

## Exact Transsion GC xrefs

- `tran_f2fs_gc()` implementation: `0x37ada8`; only direct ARM64 BL xref found: `0x377414`.
- `0x377410..0x37742c`: detector call site, `tran_f2fs_gc(sbi)` followed by `0x366cd4(sbi)`.
- `tran_gc` string `Image+0x10a4313`: kernel parameter object reference at `Image+0x1617058`.
- `gc_urgent_sleep_time` `+0x10a521c`: parameter object `+0x1617790`.
- `gc_min_sleep_time` `+0x10a52bd`: parameter object `+0x16177d8`.
- `gc_max_sleep_time` `+0x10a52cf`: parameter object `+0x1617820`.
- `gc_no_gc_sleep_time` `+0x10a52e1`: parameter object `+0x1617868`.
- `gc_urgent` `+0x10a526b`: parameter object `+0x16178f8`; code uses at `0x374d78`, `0x374f48`.
- `gc_idle_interval` `+0x10a53ee`: parameter object `+0x1617e98`.
- `tran_gc emmc life time: A=%d, B=%d`: format xref `0x376e10`.
- `tran_f2fs_gc`: format/diagnostic xrefs `0x376e74`, `0x378d44`, `0x37ac28`.
- `&tran_gc_wait_q`: xref `0x37b298`.
- `tran_gc_usb_wakelock`: xref `0x37b2c0`.
- `tran_gc_debug`: xrefs `0x37aee8`, `0x37b284`.

Proc/control registration and data globals:

- `emmc_gc_time`: registrations `0x37af5c`, `0x37b3cc`; data `+0x173b8e0`.
- `ssr_gc_times`: `0x37afb0`, `0x37b41c`; data `+0x173b9d0`.
- `gc_skip_times`: `0x37b0c8`, `0x37b4bc`; data `+0x173b610`.
- `gc_to_static_detect_times`: `0x37b154`, `0x37b50c`; data `+0x173bac0`.
- `percent_of_free_segment`: `0x37b1a8`, `0x37b42c`; data `+0x173c330`.
- `inc_gc_seg_threshold`: `0x37b1e0`, `0x37b53c`; data `+0x173c600`.
- `dec_gc_seg_threshold`: `0x37b1fc`, `0x37b54c`; data `+0x173c6f0`.
- `gc_segment_info`: registration `0x37b220`/`0x37b224`; data `+0x173cc90`.
- `life_time`: data reference `+0x1679578`.

`tran_gc_thread_func` is an inferred function label, not a recovered string. The kthread-like function begins at `0x376ed0`; no direct BL xref exists because it is installed as a function pointer.

## SBI layout correction

The binary does **not** support treating the seven I/O counters as vendor-added `f2fs_sb_info` fields. `+0x428..+0x45c` is the contiguous `nr_pages[NR_COUNT_TYPE]` area.

Confirmed X683 policy mapping:

```text
+0x444 nr_wb_cp_data
+0x448 nr_wb_data
+0x44c nr_rd_data
+0x450 nr_rd_node
+0x454 nr_rd_meta
+0x458 nr_dio_write
+0x45c nr_dio_read
```

The complete `+0x428..+0x45c` range is explicitly zeroed by SBI initialization at `0x344ee0..0x344f14`.

The suspected GC-manager fields around `+0x530..+0x564` also align with historical F2FS GC/ATGC state and must not be renamed as vendor-only fields without further layout proof.

**Current binary conclusion:** no unique vendor-only `f2fs_sb_info` field has been proven. The vendor layer primarily adds controller/global state, policy code, statistics/control registration, and branch logic around standard F2FS fields.

## Exact threshold/policy predicate formulas

Helper: `0x35cc18`, selectors 0..5. It samples a time-domain delta from the stack timing object prepared by `0x17f068` and multiplies it by `SM_I(sbi)+0x10`.

Define:

```text
R = ((delta * *(u32 *)(sm+0x10)) >> 2)
P = umulh(R, 0x28f5c28f5c28f5c3)
```

The reciprocal is the fixed-point 0.16 multiplier. The common terminal comparison uses `P >> 3`, i.e. approximately 2% of `R`.

Selector 0:

```text
((3 * sm[0xa8]) >> 9) < (P >> 3)
```

Selector 1:

```text
P > (sm[0x7c] >> 7) && (sm[0x7c] >> 5) < 0xc35
```

Selector 2:

```text
*(u32 *)(sbi->sb->s_fs_info + 0x1c8) == 0
```

Selector 3:

```text
S = sbi[0x220] + sbi[0x250] + sbi[0x280] + sbi[0x2b0] + sbi[0x2e0]
((3 * S) >> 9) < (P >> 3)
```

Selector 4:

```text
T = ((80 * (s32)sbi[0x3a4]) + (64 * (s32)sbi[0x3bc])) >> 12
T < (P >> 3)
```

Selector 5:

```text
(s32)sbi[0x43c] < floor(0.2 * delta)
```

The selector dispatch is a six-entry byte jump table at `Image+0xe74030`. Values greater than 5 use the default path, which is the same body as selector 2.

## Segment/allocation branch modifications

The policy helper `0x35cc18` is called from the segment/allocation path at:

```text
0x3595f8  selector 2
0x359ba0  selector 5
0x35bd44  selector 6/default
```

For `0x3595f8`, a true predicate continues at `0x35960c`; false branches to `0x3590ec`.

For `0x359ba0`, a true predicate branches to `0x35a3ec`; false continues normally.

At `0x35bd44`, selector value 6 is outside the six-entry jump table and therefore executes the helper default body, not a seventh predicate.

These are binary-proven branch modifications. Exact historical source-function names are not promoted unless source comparison establishes them.

## Post-GC policy

`0x366cd4` calls the same predicate helper at `0x366d00`, `0x366d1c`, `0x366d38`, `0x366dec`, and `0x366dfc`.

For `gc_mode != 3`, any nonzero value among `+0x444,+0x448,+0x44c,+0x450,+0x454,+0x458,+0x45c` selects the active path; all seven zero selects the clean path. `gc_mode==3` bypasses this discriminator.

The terminal sequence remains:

```text
0x3e1014 -> 0x34e224(sbi,1) -> 0x3e1558
0x341250(sbi->sb,1)
stat+0x16c++
```

`0x34e224` remains a high-confidence `f2fs_balance_fs(sbi, true)` candidate. `0x341250` remains only a medium-confidence public `f2fs_sync_fs(sb,1)` attribution because the binary body contains additional vendor state/callback machinery.

## Reconstruction boundary

A source-integrated, binary-faithful patch cannot honestly be claimed complete yet. Remaining proof gaps are:

1. exact original names/layout for several GC-manager fields;
2. exact public attribution of `0x341250`;
3. full source recovery of the detector/controller state machine and generic proc handlers;
4. exact insertion points in the historical X683-era source tree.

A compilable semantic reconstruction can be built without inventing SBI fields by retaining the historical 4.14 layout, adding a separate Transsion controller state object, implementing `0x35cc18` from the proven formulas, adding the three proven segment branches, and implementing the confirmed post-GC I/O gate. This is a reconstruction, not proprietary-source recovery and not a claim of byte-for-byte source identity.
