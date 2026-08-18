# X683/H694 F2FS `gc_mode` state-machine deep pass

## Result

The recovered `f2fs_sb_info + 0x534` field is treated as `gc_mode`, not the older 64-bit `fggc_threshold`. The strongest compatible historical lineage is the four-state model:

```c
GC_NORMAL      = 0
GC_IDLE_CB     = 1
GC_IDLE_GREEDY = 2
GC_URGENT      = 3
```

This exact four-state enum is independently present in nearby F2FS source. Later trees add `GC_IDLE_AT` and split urgent mode into HIGH/LOW/MID; those later states are deliberately excluded from the X683 reconstruction unless stock evidence proves otherwise. citeturn2search8turn2search4

## State -> victim algorithm

The reconstructed selection function is now:

```text
                     gc_type
                    /        \
                BG_GC        FG_GC
                  |             |
               GC_CB        GC_GREEDY
                    \         /
                     +-------+
                         |
                    gc_mode override
                         |
        +----------------+----------------+
        |                |                |
    NORMAL           IDLE_CB        IDLE_GREEDY/URGENT
        |                |                |
      keep             CB              GREEDY
```

This matches the nearby upstream implementation: default background GC uses cost-benefit, foreground GC uses greedy, `GC_IDLE_CB` forces cost-benefit, and `GC_IDLE_GREEDY` / `GC_URGENT` force greedy. citeturn1search2turn1search8

## Why this matters for X683

`gc_mode` therefore has two distinct roles:

1. It is a persistent policy/state variable in `f2fs_sb_info`.
2. Its selected algorithm becomes the index for `SIT_I(sbi)->last_victim[]`.

That explains why the recovered binary needs a word-sized state field at `0x534`: a boolean trigger would not explain the policy selection and per-policy victim cursor behavior.

The project layout already independently resolves `sbi+0x4b8` as `mount_opt.opt`, so the force-FG-GC mount option and internal GC policy state remain separate. fileciteturn13file0L2-L2

## State transitions

### NORMAL

Expected steady state. No policy override is active.

- BG_GC -> GC_CB
- FG_GC -> GC_GREEDY
- `last_victim[GC_CB]` / `last_victim[GC_GREEDY]` is selected according to the resulting policy.

### IDLE_CB

Forces cost-benefit victim selection even when the request would otherwise use greedy GC.

```text
NORMAL -> IDLE_CB
IDLE_CB -> CB
```

The exact X683 writer/trigger has not yet been proven from stock instructions; therefore this is a policy-state reconstruction, not a claim that the device exposes a particular sysfs interface.

### IDLE_GREEDY

Forces greedy victim selection.

```text
NORMAL -> IDLE_GREEDY
IDLE_GREEDY -> GREEDY
```

### URGENT

Also forces greedy victim selection in the four-state lineage.

```text
NORMAL -> URGENT
URGENT -> GREEDY
```

Nearby F2FS source explicitly makes `GC_URGENT` select the greedy policy and uses the state to make SSR/GC more aggressive. citeturn1search5turn1search13

## Important distinction: state vs request

The X683 `f2fs_gc()` ABI still has the three arguments:

```c
f2fs_gc(sbi, sync, background)
```

The request type is therefore represented separately:

```text
sync == true  -> FG_GC
sync == false -> BG_GC
```

The recovered `mount_opt.opt` bit-14 path feeds this request classification. `gc_mode` does not replace it.

This produces the complete conceptual path:

```text
mount_opt FORCE_FG_GC / caller
              |
              v
       sync/background
              |
              v
          gc_type
              |
              +--------+
              |        |
           BG_GC      FG_GC
              |        |
           default   default
              |        |
             CB     GREEDY
              \        /
               \      /
                gc_mode
                  |
       +----------+----------+
       |          |          |
      CB       GREEDY     NORMAL default
                  |
                  v
       last_victim[policy]
                  |
                  v
          victim selection
```

## What was corrected

The previous reconstruction incorrectly used `gc_thread->gc_idle` to select the policy. That belongs to a later/alternate GC-thread implementation. The X683 field evidence points to `sbi->gc_mode`, and the reconstruction now reads `sbi->gc_mode` directly.

This is an important correction because the recovered field is at a fixed `f2fs_sb_info` offset (`0x534`), whereas a GC-thread field would have a completely different object-relative address.

## What is NOT yet proven

The following remain explicitly unresolved:

- exact stock X683 writes to `sbi+0x534`;
- whether X683 exposes `gc_urgent` / `gc_idle` sysfs controls;
- whether Transsion has additional private states;
- whether any charging/USB/framebuffer/wakelock event directly writes `gc_mode`;
- whether `GC_URGENT` is entered directly or only through a vendor wrapper;
- whether X683 retains `no_fggc_candidate()` after the `fggc_threshold -> gc_mode` structural change.

We should not invent those transitions.

## Confidence

| Item | Confidence |
|---|---|
| `sbi+0x534` = `gc_mode` | High |
| four-state policy family is the correct historical neighborhood | High |
| NORMAL default BG=CB / FG=GREEDY | High |
| IDLE_CB forces CB | High |
| IDLE_GREEDY forces GREEDY | High |
| URGENT forces GREEDY | High |
| `last_victim[]` is indexed by resulting policy | High |
| exact X683 state writers | Unresolved |
| exact Transsion state-transition triggers | Unresolved |
| later AT/URGENT_HIGH/LOW/MID states in X683 | Not established |
