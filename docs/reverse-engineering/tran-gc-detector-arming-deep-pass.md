# X683/H694 Transsion GC detector arming/state transition deep pass

Reconstructed directly from the supplied stock boot.img. This is inferred/reconstructed code, not recovered proprietary source.

## Target

Stock decompressed-kernel range: `0x377120..0x377494`.

## 1. `+0xa00` is an explicit detector-mode/state gate

The detector begins by reading `controller + 0xa00`.

```text
+a00 == 0:
    execute static arming predicates
    on success:
        +0xa00 = 1
        +0xa04 = 1
    on failure:
        +0xa00 = 2

+a00 != 0:
    skip the arming predicates
```

The common path then enters detector state 2:

```text
+0x9d4 = 2
+0xa06 = 1
```

`+0xa06` is therefore directly established as the detector-active/continue flag in this path.

## 2. Static arming predicates

The stock code first computes the following values from F2FS manager state:

```c
sm        = SM_I(sbi)
sit       = sm->sit_info
free_i    = sm->free_info
dirty_i   = sm->dirty_info

recoverable =
    sum(dirty_i->type_counts[0..5]);

user_segments =
    sbi->user_block_count >> sbi->log_blocks_per_seg;

ratio =
    ((sm->free_segments + recoverable - sm->main_segments)
       << sbi->log_blocks_per_seg)
    + (sit->sit_blocks - sbi->user_block_count);

w21 = ratio / (recoverable + free_i->free_segments);
```

The compiled arithmetic is direct AArch64 integer arithmetic; the semantic names above are inferred from the manager offsets.

The next gate compares `recoverable` against approximately `user_segments / 10` using a reciprocal multiply by `0xAAAAAAAAAAAAAAAB`-style constant.

It also requires:

```text
w21 >= 0x15f   // 351
```

## 3. Remaining arming checks

The stock then requires a free-segment relationship based on `sm->main_segments` and `free_i->free_segments`.

It next compares the non-SIT portion of `user_block_count` against a ~2.5%-scaled quantity derived from `13 * user_block_count` using `0x51EB851F >> 37`.

Finally it compares `sbi + 0x3f0` against a ~2.5%-scaled `27 * sit_blocks` quantity.

Only if all these checks pass:

```text
+0xa00 = 1
+0xa04 = 1
```

Otherwise:

```text
+0xa00 = 2
```

## 4. Detector-state transition

Common path after the `+0xa00` decision:

```text
+0x9d4 = 2
+0xa06 = 1
```

The code then tests superblock/filesystem state and vendor/global guards before continuing into the next stage.

At `0x377494` a later transition is explicit:

```text
+0x9d4 = 3
vendor-state +0x158 = 1
```

Two vendor helper calls follow, then an external boolean gate determines whether the detector proceeds.

## 5. Runtime guard paths after state 2

The detector checks:

- `sbi->sb->s_flags` against active state;
- several filesystem counters at `sbi + 0x444..0x45c`;
- a pair of nested objects under `sm_info + 0x80`, including offsets `+0xa0/+0x2090` and `+0x98/+0x24`;
- vendor state at `+0x974`;
- a runtime quantity at vendor `+0xa10` against `*(object + 0x10)`.

When the `+0xa10` value differs, the stock code stores the new value and invokes the helper at `0xe0693c`.

When equal, it calls the helper at `0x1eca60` with `(1, 0)` and branches based on the return value.

These helper names remain unresolved until their call targets are separately reconstructed.

## 6. Important correction

The earlier model treated `+0xa00` as an ambiguous statistic. It is now proven to participate in control flow as a detector state/mode gate.

Similarly, `+0x9d4 = 2` is a direct transition after detector arming, while `+0x9d4 = 3` is a later explicit transition at `0x377494`.

## Confidence

| Finding | Confidence |
|---|---|
| `+0xa00` is a detector mode/state gate | High |
| `+0xa00 = 1` is reached only when all static arming predicates pass | High |
| `+0xa00 = 2` is the fallback state | High |
| state 2 sets `+0x9d4 = 2` | High |
| state 2 sets `+0xa06 = 1` | High |
| state 3 sets `+0x9d4 = 3` | High |
| `+0xa04 = 1` is enabled on successful static arming | High |
| exact semantic names of runtime helper targets | Unresolved |
