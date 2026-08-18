# X683/H694 — seven SBI fields and Image+0x16c6980 producer/consumer trace

Date: 2026-08-18

## Authority

Binary-derived from the stock X683/H694 Image. The repository currently exposes the kernel payload as `artifacts/kernel/x683_kernel_compressed.gz`; the connected GitHub binary fetch path cannot decode that gzip blob as text. This report therefore promotes only references independently present in committed raw/disassembly artifacts and prior byte-level analysis.

## Seven SBI fields

Exact policy-discriminator fields:

```text
sbi +0x444
sbi +0x448
sbi +0x44c
sbi +0x450
sbi +0x454
sbi +0x458
sbi +0x45c
```

Fresh byte-level analysis proves:

```text
any nonzero -> active guarded path 0x366da4
all seven zero -> clean/alternate path 0x366ee0
gc_mode == 3 -> bypass discriminator, enter shared stage 0x366de4
```

The same seven-field region is also read by the detector runtime guard after state-2 arming, so these are persistent SBI-resident values rather than locals of `0x366cd4`.

## Writer status

No committed raw disassembly artifact currently contains enough instruction coverage to assign exact writer addresses for all seven fields. The consumer side is proven; producer identities remain unresolved.

| Field | Consumers | Writer |
|---|---|---|
| `+0x444` | `0x366cd4`, detector runtime guard | unresolved |
| `+0x448` | `0x366cd4`, detector runtime guard | unresolved |
| `+0x44c` | `0x366cd4`, detector runtime guard | unresolved |
| `+0x450` | `0x366cd4`, detector runtime guard | unresolved |
| `+0x454` | `0x366cd4`, detector runtime guard | unresolved |
| `+0x458` | `0x366cd4`, detector runtime guard | unresolved |
| `+0x45c` | `0x366cd4`, detector runtime guard | unresolved |

Current X683 SBI anchors around the region are:

```text
+0x408 user_block_count
+0x428 reserved_blocks
+0x430 current_reserved_blocks
+0x438 unusable_block_count
+0x440 nquota_files
+0x4b8 mount_opt.opt
+0x534 gc_mode
+0x568 f2fs_stat_info *
```

Because the seven fields sit in the vendor/X683-divergent region after `nquota_files`, they must not be renamed from generic 4.14 structure layouts without an exact offset match.

## Image+0x16c6980 consumers

Two independent policy branches consume the same 64-bit global:

### Active/shared path

```text
value  = 250 * (sbi +0x1c8) + (sbi +0x198)
global = *(u64 *)(Image +0x16c6980)
value >= global -> direct return
value <  global -> terminal path
```

### Clean/alternate path

```text
value  = 250 * (sbi +0x1d0) + (sbi +0x1a0)
global = *(u64 *)(Image +0x16c6980)
value >= global -> active path 0x366da4
value <  global -> shared stage 0x366de4
```

Therefore this is one shared 64-bit vendor policy threshold/reference.

## Image+0x16c6980 producer status

No committed producer/write instruction has been located in the currently accessible artifacts.

Keep the neutral label:

```text
x683_gc_policy_global_16c6980
```

Do not call it a timeout, jiffies value, segment threshold, charger value, or other semantic symbol from arithmetic shape alone.

## Required producer sweep

Once the decompressed Image is available to a byte-capable tool:

```text
1. resolve all ADRP/ADD references to Image+0x16c6980;
2. classify every load/store/atomic access;
3. identify initialization and runtime mutation writers;
4. repeat for sbi+0x444..0x45c;
5. map each writer to caller/context;
6. only then assign source-level names.
```

## Related stat_info result

The terminal path accesses `sbi+0x568` and increments `stat_info+0x16c`. Historical 4.14 structure correlation strongly identifies this member as `dirty_count`; this is separate from the unresolved seven-field writer mapping.

## Confidence

High: exact seven-field consumer set and branch role; exact `Image+0x16c6980` consumers.

Unresolved: exact writers/producers and original source-level names.
