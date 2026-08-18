# X683/H694 F2FS GC victim/deep-pass reconstruction

Target: Infinix X683/H694, MT6768, Linux 4.14.141-era Android 10.

## 1. Revision anchor

The recovered stock call boundary is the three-argument form:

    f2fs_gc(sbi, sync, background)

Public Linux history confirms this exact prototype existed before the later victim-segment extension. The 4.15-era implementation is therefore a much stronger source anchor for the X683 target than the later five-argument implementation.

## 2. `__get_victim()` reconstruction

The helper is structurally:

    sit_i = SIT_I(sbi)
    lock sit_i->sentry_lock
    ret = DIRTY_I(sbi)->v_ops->get_victim(
        sbi, victim, gc_type, NO_CHECK_TYPE, LFS)
    unlock sit_i->sentry_lock
    return ret

This is high confidence because the same helper shape is present in the 4.15-era tree and the recovered X683 layout independently identifies `sm_info -> dirty_info` as the dirty-victim path.

Important: do not collapse `DIRTY_I(sbi)->v_ops->get_victim()` into a direct victim algorithm yet. The actual candidate selection lives in the dirty-segment manager implementation and is the next subtarget.

## 3. Candidate-selection ABI

The call arguments establish four semantic inputs beyond `sbi`:

- output victim segment pointer
- GC type (`FG_GC` or `BG_GC`)
- `NO_CHECK_TYPE`
- `LFS` allocation mode

The `NO_CHECK_TYPE`/`LFS` pair is significant: the stock core is asking the generic dirty-victim selector for a normal LFS GC victim rather than an SSR/type-restricted candidate.

## 4. `do_garbage_collect()` reconstruction

The historical 4.15 implementation gives the following core sequence:

1. Derive the end of the victim section from `start_segno + segs_per_sec`.
2. Determine whether the segment is a data or node segment from its segment-entry type.
3. If the section contains multiple segments, perform SSA summary-page readahead.
4. Acquire/reference the summary pages for the victim segments and release their page locks before migration.
5. Start a block plug for the migration batch.
6. For each victim segment:
   - obtain its summary page;
   - skip invalid/unusable summary state as required by the target revision;
   - inspect the summary footer;
   - dispatch to `gc_node_segment()` or `gc_data_segment()`;
   - update GC statistics;
   - release the summary page.
7. For foreground GC, flush merged node/data writes.
8. Determine whether the entire target section has become free.
9. Finish the block plug and return the freed-segment/section result according to the exact target revision.

The X683 reconstruction must preserve this division: `do_garbage_collect()` is a summary/migration dispatcher, not the victim-selection algorithm itself.

## 5. Critical revision boundary

There are two nearby historical forms:

### Older 4.14/4.15-era form

`do_garbage_collect()` returns a section-freed indicator and `f2fs_gc()` takes:

    (sbi, sync, background)

### Later 4.15+ development form

The API was extended with a starting victim segment:

    (sbi, sync, background, segno)

The later form also introduced `init_segno` bookkeeping and eventually more GC-state machinery. Those additions must not be backported into X683 unless stock evidence requires them.

## 6. Matching against recovered `f2fs_sb_info`

Known X683 offsets:

- `0x3d8` = `log_blocks_per_seg`
- `0x3dc` = `blocks_per_seg`
- `0x3e0` = `segs_per_sec`
- `0x408` = `user_block_count`
- `0x428` = `reserved_blocks`
- `0x430` = `current_reserved_blocks`
- `0x438` = `unusable_block_count`
- `0x440` = `nquota_files`
- `0x4b8` = `mount_opt.opt`
- `0x534` = `gc_mode` (high confidence)

The GC helpers directly explain the importance of `0x3e0`: it controls the section traversal count and the complete-section success test.

The dirty manager is reached indirectly through `sm_info`; therefore no new `f2fs_sb_info` offset is required for `__get_victim()` itself beyond the already recovered `sm_info` pointer.

## 7. Dirty-manager boundary

Recovered segment-manager relationships:

- `sm_info + 0x00` = `sit_info`
- `sm_info + 0x08` = `free_info`
- `sm_info + 0x10` = `dirty_info`
- `sm_info + 0x60` = `reserved_segments`

This gives a coherent path:

    sbi
      -> sm_info
         -> dirty_info
            -> v_ops
               -> get_victim()

and separately:

    sbi
      -> sm_info
         -> free_info
            -> free_segments

No offset conflict is presently visible.

## 8. `gc_mode` significance

The resolved `0x534` member is consistent with the historical `gc_mode` field used by victim selection to index the last-victim/search state. This is important because a false identification here would shift the interpretation of subsequent GC-state accesses.

The current evidence supports `gc_mode` as the semantic identity, but the exact byte/word width and surrounding padding must still be validated against the stock instruction widths before finalizing the C struct packing.

## 9. What is NOT yet claimed

Not yet reconstructed with sufficient stock-specific evidence:

- exact X683 `get_victim()` candidate-cost function;
- exact `p`/`victim_sel_policy` structure layout;
- exact `last_victim[]` indexing around `gc_mode`;
- Transsion modifications to candidate scoring;
- exact `gc_node_segment()` implementation revision;
- exact `gc_data_segment()` implementation revision;
- any vendor-specific charging/USB/display/wakelock predicate inside the GC thread/wrapper.

These remain separate targets rather than being guessed from a newer kernel.

## 10. Confidence

| Component | Confidence |
|---|---|
| 3-argument `f2fs_gc` ABI | High |
| `__get_victim()` locking/dispatch structure | High |
| `DIRTY_I -> v_ops -> get_victim` path | High |
| `NO_CHECK_TYPE, LFS` arguments | High |
| `do_garbage_collect()` summary dispatch | High |
| `segs_per_sec` section traversal | High |
| `gc_mode` at `sbi+0x534` | High |
| exact X683 dirty-victim cost function | Unresolved |
| exact Transsion victim-selection delta | Unresolved |
| exact node/data migration revision | Unresolved |
