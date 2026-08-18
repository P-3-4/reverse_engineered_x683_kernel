# X683/H694 boot.img — reusable GC reverse-engineering artifacts

## Image identity

- boot.img size: 33,554,432 bytes
- boot.img SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- Android boot magic: `ANDROID!`
- page size: `0x800`
- kernel compressed offset: `0x800`
- kernel compressed size: `0x94dad4` (9,755,348)
- ramdisk size: `0x0e6528` (943,464)
- compressed kernel SHA-256: `6701980890b0b18d34e88369ef50d624e3f3bee0b5a481d833141b2d256e20bd`
- decompressed kernel size: 26,615,820 bytes
- decompressed kernel SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

The kernel is gzip-compressed. The compressed stream contains trailing data after the gzip member; extraction must stop at gzip EOF rather than treating the complete boot-kernel slot as one gzip member.

## Reusable evidence ranges

The following ranges are committed as raw hexadecimal artifacts and matching AArch64 disassembly:

- `0x37ada8..0x37af00` — `tran_f2fs_gc` wrapper / `gc_mode` override
- `0x377700..0x3779b0` — Transsion static-detection / Stop Conditions 1–5
- `0x37b5d4..0x37b8c0` — GC threshold/helper routine

These offsets are **decompressed-kernel offsets**, not boot.img file offsets.

## Proven controller fields

- `+0x990`: 64-bit invocation/cycle counter; wrapper increments it before GC.
- `+0x998`: 32-bit Transsion GC controller state. Values 0/1/2 select normal/GREEDY/URGENT wrapper behavior.
- `+0x9c0`: byte flag gating controller writes in the static detector.
- `+0x9d0`: 32-bit loop/termination state consulted on exit paths.
- `+0x9d4`: 32-bit detector state; observed writes include 1, 2, 3, and 4.
- `+0x9d8`: 64-bit counter incremented on a repeated detector path.
- `+0x9e0`: 64-bit detector-cycle counter; incremented at detector entry.
- `+0x9f0`: 32-bit running maximum/statistic updated from a calculated segment value.
- `+0x9f4`: 32-bit saved baseline/statistic used in delta calculation.
- `+0x9f8`: byte/word stop-result flag; observed values 0, 1, and 2.
- `+0x9fc`: 32-bit stop-condition code; observed values 1, 2, 3.
- `+0xa04`: byte controlling periodic cadence: zero selects 50, nonzero selects 500.
- `+0xa05`: byte loop state tested against 1 before continuing detection.
- `+0xa06`: byte enable/continue flag; detector clears it on abort/exit paths and sets it to 1 while active.
- `+0xa08`: signed 32-bit baseline segment value.
- `+0xa0c`: 32-bit baseline written-segment value.

Field names above are evidence-based semantic labels, not claimed vendor source names.

## Stop-condition evidence

### Condition 1

The predicate is a signed `b.gt` comparison at `0x377724` targeting the Condition-1 logging block at `0x377968`.

The compared value is `w25`, which is the calculated `delta_seg` passed to the log call. The threshold is computed from a table-selected value multiplied by `w21` and scaled by the constant `0x51EB851F` with a right shift of 37 (approximately 2.5%).

Observed behavior:

`delta_seg > calculated_threshold` -> Stop condition 1 -> `+0x9fc = 1`.

### Condition 2

The predicate is a signed `b.gt` at `0x377770` targeting the Condition-2 logging block at `0x377984`.

It compares the second calculated segment-delta quantity (`w9`) against a table-derived threshold (`w10`).

Observed behavior:

`second_delta > second_threshold` -> Stop condition 2 -> `+0x9fc = 2`.

### Condition 3

The predicate is a signed `b.lt` at `0x3777d0` targeting the Condition-3 logging block at `0x377998`.

It compares a 64-bit scaled movement/cost quantity (`x8`) against a signed segment-derived quantity (`x9`).

Observed behavior:

`scaled_movement < segment_reference` -> Stop condition 3 -> `+0x9fc = 3`.

### Condition 4

At `0x3777f0`, the detector compares a calculated segment/write delta (`x9`) against the configured threshold loaded from the global structure at `+0xd90` of another vendor state object.

If `x9 > threshold`, execution falls through to the Condition-4 logger at `0x3777fc`/`0x377808`.

Condition 4 then:

1. logs `match: Stop condition 4, dec_seg=%d, inc_written_seg=%d, switch to SSR`;
2. if `+0x9c0` is clear, writes controller `2` to `+0x998`;
3. writes `1` to `+0x9f8`.

This is the direct binary proof of the **SSR-switch trigger**.

### Condition 5

The detector uses `+0xa04` to choose a periodic interval:

- `+0xa04 == 0` -> 50 detector cycles
- `+0xa04 != 0` -> 500 detector cycles

It computes `+0x990 % interval`. Only when the remainder is zero does it evaluate the Condition-5 segment-progress test.

The test uses:

- `+0xa0c` as the previous written-segment baseline;
- `+0xa08` as the previous segment baseline;
- current segment/write values from the detector;
- signed comparison at `0x377858..0x377878`.

When the periodic test indicates no sufficient free-segment progress, the path writes controller `2` to `+0x998`, writes `2` to `+0x9f8`, and logs:

`match: Stop condition 5,every 400 times gc none free segment inc`

The literal says 400 while the compiled cadence selection is 50/500; this discrepancy is preserved as binary evidence rather than normalized away.

## SSR decision

The exact stock path is therefore:

`segment/write delta exceeds vendor threshold`
-> Condition 4
-> controller `+0x998 = 2` unless `+0x9c0` blocks the write
-> `+0x9f8 = 1`
-> later Transsion GC wrapper observes controller 2
-> temporarily sets `sbi + 0x534 = 3`
-> calls `f2fs_gc(sbi, sync, true, -1)`
-> restores the previous `gc_mode`.

Condition 5 is a separate periodic no-progress trigger that also drives controller 2 and records stop result 2.

## Important ABI correction

The stock X683 kernel uses the four-argument F2FS GC entry point:

`f2fs_gc(sbi, sync, background, segno)`

with the vendor wrapper passing `segno = -1` (`NULL_SEGNO`).

Do not use the older three-argument reconstruction when correlating this image.
