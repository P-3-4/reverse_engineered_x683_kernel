# X683/H694 `f2fs_stat_info` model

Target stock kernel: Linux 4.14.141+ / X683-H694.

## Evidence boundary

The stock X683 binary proves that `sbi + 0x568` contains a pointer used by F2FS GC statistics/accounting. This proves the `stat_info` pointer location, but the uploaded X683 disassembly artifacts currently available in the project do **not** provide enough direct `stat_info + member_offset` accesses to assign individual X683 member offsets safely.

Therefore this document separates:

1. binary-proven X683 facts;
2. historical F2FS structure reference;
3. unresolved X683 member offsets.

## Binary-proven fact

```text
struct f2fs_sb_info *sbi
        + 0x568
            -> stat_info pointer
```

Confidence: **high**.

The stock symbol/layout documentation also records `sbi + 0x568` as the statistics pointer, and `f2fs_gc()` accesses this field. The stock kernel has `CONFIG_F2FS_STAT_FS=y`.

## Historical reference model

Historical 4.14-era F2FS defines a separate `struct f2fs_stat_info`, beginning with fields such as:

```text
+0x00  struct list_head stat_list
+0x10  struct f2fs_sb_info *sbi
+0x18  int all_area_segs
+0x1c  int sit_area_segs
+0x20  int nat_area_segs
+0x24  int ssa_area_segs
+0x28  int main_area_segs
+0x2c  int main_area_sections
+0x30  int main_area_zones
+0x38  unsigned long long hit_largest
+0x40  unsigned long long hit_cached
+0x48  unsigned long long hit_rbtree
+0x50  unsigned long long hit_total
+0x58  unsigned long long total_ext
+...
```

The exact tail and all intermediate fields vary across F2FS revisions. Public historical source confirms the object is separately allocated and assigned to `sbi->stat_info`; it is not the bytes immediately following the pointer in `f2fs_sb_info`. citeturn205243search0turn205243search4

## Important distinction

These are **not** valid statements without additional binary evidence:

```text
sbi + 0x570 = stat_info->meta_count
sbi + 0x5d4 = stat_info->bg_gc
```

The correct forms would have to be:

```text
sbi + 0x568 -> STAT
STAT + N -> some f2fs_stat_info member
```

while fields such as `meta_count`, `bg_gc`, `io_skip_bggc`, and `other_skip_bggc` may themselves belong to `f2fs_sb_info` in the X683-era source generation. Historical 4.14 source demonstrates exactly this split: `stat_info` is a pointer, while the GC/stat counters are also present as `f2fs_sb_info` fields. citeturn205243search6

## Current X683 status

| Item | Status |
|---|---|
| `sbi + 0x568` is `stat_info *` | **High confidence** |
| `stat_info` is a separate object | **Confirmed by historical allocation model + pointer semantics** |
| Individual X683 `stat_info + offset` members | **Unresolved** |
| `sbi + 0x5d4` / `0x5d8` / `0x5dc` | **SBI fields; names remain strong structural candidates** |
| Exact X683 `struct f2fs_stat_info` revision | **Unresolved** |

## Next required binary evidence

The next search must find code of the form:

```asm
ldr xN, [xSBI, #0x568]
...
ldr/str/cas/atomic* [xN, #OFFSET]
```

or an equivalent path through a `F2FS_STAT(sbi)` helper.

Only those accesses should be used to assign X683-specific member offsets.

## Sanity rule

Do not use the historical C structure to manufacture X683 offsets. Use it only after an X683 pointer-relative access has been recovered. When the binary does not distinguish a member, retain the numeric offset and mark it unresolved.
