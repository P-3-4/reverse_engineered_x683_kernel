# X683 Kernel Reverse Engineering — Current State Snapshot

Date: 2026-08-19
Repository: P-3-4/reverse_engineered_x683_kernel

## Completed phase

The F2FS segment-manager reconstruction phase is complete enough to move on.

Reconstructed against the stock X683 binary:

- `f2fs_sm_info`
- `sit_info`
- `free_segmap_info`
- `dirty_seglist_info`
- `curseg_info[6]`
- `flush_cmd_control`
- `discard_cmd_control`

Key established facts:

```text
sizeof(f2fs_sm_info)          = 0xA8
sizeof(sit_info)              = 0xA8
sizeof(free_segmap_info)      = 0x20
sizeof(dirty_seglist_info)    = 0x90
sizeof(curseg_info)           = 0x70
curseg_info count             = 6
curseg array size             = 0x2A0
sm_info + 0x98                = fcc_info
sm_info + 0xA0                = dcc_info
sizeof(discard_cmd_control)   = 0x20B0
```

The dirty counters at `dirty_info + 0x68..0x7c` are the first six elements of `nr_dirty[8]`, not vendor-specific counters.

The earlier hypothesis that `sm_info + 0x40..0x47` and `+0x90..0x97` were vendor insertions was withdrawn during consumer-based proof. They are accounted for by the actual structure generation/alignment and normal F2FS fields. Do not revive that hypothesis without new binary evidence.

## Transsion GC phase completed

A deep pass was performed on the vendor GC framework. The main conclusion is:

Transsion did not replace the core F2FS collector. The vendor code provides a policy/control layer around stock GC.

Relevant vendor functions identified:

- `tran_gc_thread_func`
- `tran_urgent_gc_read`
- `tran_urgent_gc_write`
- `tran_do_f2fs_gc`
- `tran_gc_init`
- `tran_gc_stop`

`tran_do_f2fs_gc()` is a policy wrapper around `f2fs_gc()`: it selects a requested GC mode, invokes the normal collector, and restores the previous mode.

The vendor thread consumes normal F2FS segment-manager state, including the flush/discard control objects. The vendor framework contains urgent-GC state, GC type/control state, counters, free-segment thresholds, fragmentation/SSR decisions, timing, and telemetry.

`CONFIG_F2FS_TRAN_GC=y` is enabled in the X683 configuration.

## Next phase

The next target is NOT more reconstruction of the standard segment-manager structures.

Target the actual algorithmic Transsion delta inside F2FS GC:

1. victim selection
2. victim scoring/cost calculation
3. SSR selection
4. age/mtime use
5. victim filtering
6. migration policy
7. whether Transsion changes the stock victim-selection algorithm or only controls when/mode it is invoked

Compare the X683 implementations directly against the closest matching Linux/Android 4.14 F2FS baseline. Keep three evidence classes separate:

- binary-proven X683 behavior
- source-correlated behavior
- inference/speculation

Do not fill unknowns from a newer F2FS definition merely because the names look familiar.

## Important repository branches

The repository accumulated many exploratory branches during reconstruction. The primary active workline is:

`reconstruction-f2fs-balance-delta`

Recent completed commits on that workline include:

- `904452656f23c3102c0a7e7b6ada6ca80926fb9f` — final X683 F2FS child-layout reconstruction
- `117058dfc9d4d623af065948ad16ca8c93e6b812` — deep Transsion GC reconstruction pass

An archival branch was created:

`archive/reconstruction-f2fs-balance-delta-2026-08-19`

It preserves the completed balance/segment-manager reconstruction state before further cleanup.

The repository also contains many older exploratory GC and reconstruction branches. Treat them as historical work unless their commits contain evidence not present on the active workline. Do not assume branch names imply correctness.

## Evidence rule

The stock X683 kernel binary is authoritative. Public F2FS source is used as a comparison/naming baseline only. Every claimed vendor delta must have an X683 binary/code-path basis.
