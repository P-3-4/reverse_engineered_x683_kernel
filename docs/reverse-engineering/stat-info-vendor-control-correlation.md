# X683 stat_info / vendor-control correlation

Reconstructed from the verified X683/H694 stock Image. Labels are reconstruction labels, not recovered proprietary names.

## Direct stat_info map

`sbi + 0x568` points to the vendor-divergent `f2fs_stat_info` object.

Confirmed members:

```text
+0x164  GC/accounting counter candidate
+0x170  paired statistics member; exact role unresolved
+0x174  completion count A
+0x178  completion count B
+0x184  aggregate A
+0x188  aggregate B
+0x18c  completion counter A
+0x190  completion counter B
+0x194  unresolved
+0x198  completion aggregate
```

The binary directly increments the count members and accumulates a local GC quantity into the aggregate members.

## Vendor controls

The names `need_switch_ssr`, `tran_urgent_gc`, and `detect_charger_type` are registered as control descriptors through the common registry. Their descriptors are known, but the current evidence does not prove that any of the stat_info members above are their backing storage.

Likewise, the strings `gc_times`, `gc_segment_info`, and `written_data` have not been tied to these offsets by a direct read/store chain in the repository evidence. Do not assign those names by proximity.

## 0x37b5d4..0x37b8c0 status

The existing repository evidence is insufficient to claim an exact source-level reconstruction of this range. The current handoff's threshold formulas should remain the authoritative reconstruction until a raw disassembly slice covering this range is added.

## Next exact target

Acquire/derive the raw Image disassembly for `0x37b5d4..0x37b8c0`, then trace every load/store and call target. Correlate those accesses against the registered control descriptors and stat_info reads before assigning semantic field names.
