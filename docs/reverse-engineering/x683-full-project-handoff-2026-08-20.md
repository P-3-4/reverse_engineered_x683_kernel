# X683 Kernel Reverse Engineering — Full Project Handoff

Date: 2026-08-20

## Project Identity

Repository: P-3-4/reverse_engineered_x683_kernel

Device: Infinix X683 / H694
SoC: MediaTek MT6768
Architecture: ARM64
Stock target: Android 10
Stock kernel: Linux 4.14.141+ vendor modified

Objective: reconstruct a buildable, functionally equivalent X683/H694 kernel from stock binary evidence.

Authority rules:
- Stock boot.img kernel execution is authoritative.
- No unsupported structure fields, callback ownership, BLR targets, or source matches.

## Current Roadmap

Phase 1: vendor kernel behavior reconstruction (active)
- Recover vendor modifications.
- Recover Transsion F2FS GC.
- Recover vendor structures.
- Lock kernel API compatibility.
- Produce compilable patches.

Phase 2: kernel tree reconstruction.

Phase 3: functional kernel build and boot validation.

## Confirmed F2FS Findings

CONFIG_F2FS=y
CONFIG_F2FS_FS=y
CONFIG_F2FS_TRAN_GC=y

Recovered functions:
- tran_gc_init()
- tran_gc_stop()
- tran_gc_thread_func()
- tran_do_f2fs_gc()
- tran_has_enough_free_segment()
- is_f2fs_fragmentation()

tran_do_f2fs_gc confirmed behavior:
- save gc_mode
- call f2fs_gc(sbi, sync, true, NULL_SEGNO)
- restore gc_mode

Stock binary uses four argument f2fs_gc ABI.
GC entry identified around Image offset 0x3503a8.

## Recovered f2fs_sb_info offsets

sbi + 0x3d8 log_blocks_per_seg
sbi + 0x3dc blocks_per_seg
sbi + 0x3e0 segs_per_sec
sbi + 0x408 user_block_count
sbi + 0x410 total_valid_block_count
sbi + 0x418 discard_blks
sbi + 0x420 last_valid_block_count
sbi + 0x428 reserved_blocks
sbi + 0x430 current_reserved_blocks
sbi + 0x438 unusable_block_count
sbi + 0x440 nquota_files
sbi + 0x4b8 mount_opt.opt

GC fields:
0x528 gc_thread
0x530 cur_victim_sec
0x534 gc_mode
0x538 next_victim_seg[0]
0x53c next_victim_seg[1]
0x540 skipped_atomic_files[0]
0x544 skipped_atomic_files[1]
0x550 skipped_gc_rwsem
0x558 gc_pin_file_threshold
0x560 max_victim_search
0x564 migration_granularity
0x568 stat_info pointer

Unresolved:
- vendor region 0x5d4-0x5dc
- GC mutex location

## sm_info Reconstruction

sbi + 0x80 = sm_info

sm_info:
0x00 sit_info
0x08 free_info
0x10 dirty_info
0x40 seg0_blkaddr
0x48 main_blkaddr
0x50 ssa_blkaddr
0x58 segment_count
0x5c main_segments
0x60 reserved_segments
0x64 additional_reserved_segments
0x68 ovp_segments

Dirty info accesses around +0x68 to +0x7c remain unresolved.

## Current Priority

1. Runtime BLR provenance and execution ownership.
2. MMC/MSDC storage path reconstruction.
3. DT driver graph validation.
4. Kernel source baseline lock.
5. Build skeleton integration.

## Remaining Tasks

F2FS:
- complete sit_info layout
- complete free_segmap_info layout
- complete dirty_seglist_info layout
- complete curseg_info layout
- identify exact vendor F2FS revision
- recover gc.c and segment.c modifications
- recover threshold formulas

Kernel:
- map vendor drivers
- match DT bindings
- recover callback ownership
- validate boot-critical paths

## Continuation

Next agent should read docs/reverse-engineering/x683-current-state.md and continue on branch kernel-reconstruction-current using evidence-only updates.
