# X683/H694 GC reverse-engineering — five-pass status (sanity-checked)

This document records the recovered GC architecture and the requested passes #1–#5. It is a binary-derived reconstruction/scaffold, not recovered proprietary Transsion source and not yet a buildable replacement.

## Binary authority

Boot SHA-256:
`a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`

Decompressed Image SHA-256:
`96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

## Pass 1 — threshold/helper

The `0x37b580–0x37b8c0` region is reconstructed at the arithmetic/control-flow level. Confirmed elements include fragmentation arithmetic, selector/scale selection, the Stop-2 fixed-point threshold, and the Stop-3 signed fixed-point comparison.

The source implementation deliberately avoids pretending that unresolved vendor globals are ordinary `f2fs_sb_info` fields.

## Pass 2 — detector/thread

Recovered with high confidence at the state-machine level:

```text
arming → state 3 timed wait/recheck → metric collection → Stop 1–5 → controller/result updates
```

Recovered state fields include the `+0x990`, `+0x998`, `+0x9c0`, `+0x9d4`, `+0x9f8`, `+0x9fc`, `+0xa00`, `+0xa04`, `+0xa08`, and `+0xa0c` region.

The repository's thread source is intentionally a **detector-step scaffold**. The exact scheduler/re-entry/abort sequence is not claimed complete.

## Stop conditions

```text
Stop1 → +0x9fc = 1
Stop2 → +0x9fc = 2
Stop3 → +0x9fc = 3
Stop4 → controller = 2 unless +0x9c0; +0x9f8 = 1
Stop5 → controller = 2; +0x9f8 = 2
```

## Pass 3 — wrapper

Recovered controller semantics:

```text
controller 0 → normal f2fs_gc()
controller 1 → temporary gc_mode 2 → f2fs_gc() → restore
controller 2 → temporary gc_mode 3 → f2fs_gc() → restore
```

The four-argument stock `f2fs_gc(sbi, sync, background, segno)` ABI is confirmed.

## Pass 4 — historical 4.14 delta

Architectural separation is established:

```text
stock F2FS GC core
+
Transsion detector/controller
+
Stops 1–5
+
vendor statistics/control layer
```

This is **not yet a complete line-by-line X683-vs-stock patch**.

## Pass 5 — vendor controls

Proven registration/descriptors:

```text
need_switch_ssr       → descriptor 0x173b9d0
tran_urgent_gc        → descriptor 0x173bbb0
detect_charger_type   → descriptor 0x173bf70
```

These are not proven standalone implementation symbols. `tran_gc_usb_wakelock` remains unresolved.

## Sanity-check corrections

The previous scaffold contained two unsafe assumptions which have been removed:

1. `(timeout_ms + 3) >> 2` was incorrectly used as a fake `msecs_to_jiffies()` implementation. It is now explicitly unresolved.
2. The six-word sum at `dirty_info + 0x68` was not sufficiently proven and is no longer hard-coded as the recoverable-segment definition.

See `docs/reverse-engineering/full-sanity-check-2026-08-18.md` for the complete audit.

## Current blockers

1. Exact generic callback bodies/backing fields for the three named controls.
2. `tran_gc_usb_wakelock` implementation path.
3. Exact semantics of `0xcc774`, `+0x974`, and controller-object `+0x20`.
4. Exact state-3 scheduler/wait/re-entry sequence.
5. Exact vendor `stat_info` field names.
6. Complete line-by-line stock/X683 `gc.c` differential.
7. Compilation against the real X683/H694 4.14 source tree.
