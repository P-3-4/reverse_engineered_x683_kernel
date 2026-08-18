# X683/H694 policy-field producer/consumer — final v2

## Seven SBI fields

The exact policy fields are:

```text
+0x444  nr_wb_cp_data
+0x448  nr_wb_data
+0x44c  nr_rd_data
+0x450  nr_rd_node
+0x454  nr_rd_meta
+0x458  nr_dio_write
+0x45c  nr_dio_read
```

The semantic identities are established by the stock statistics-copy path at `0x375ed8..0x375f0c`, where each SBI field is copied into the corresponding `f2fs_stat_info` I/O-stat field.

## Policy consumption

At `0x366cd4`, for `gc_mode != 3`:

```text
any nonzero -> active guarded path 0x366da4
all zero    -> clean/alternate path 0x366ee0
```

So the gate is an I/O-activity discriminator.

## Writers

Initialization:

```text
0x344efc -> +0x444 = 0
0x344f00 -> +0x448 = 0
0x344f04 -> +0x44c = 0
0x344f08 -> +0x450 = 0
0x344f0c -> +0x454 = 0
0x344f10 -> +0x458 = 0
0x344f14 -> +0x45c = 0
```

Runtime writers positively bound to the `f2fs_sb_info` base:

```text
0x32b0e0 -> +0x450
0x338f58 -> +0x450
0x338f5c -> +0x448
0x338f70 -> +0x458
```

No other non-initialization writers have yet been proven for:

```text
+0x444
+0x44c
+0x454
+0x45c
```

Offset-only matches in unrelated battery/thermal or other structures are excluded.

## `Image+0x16c6980`

Consumers in the vendor policy:

```text
0x366e64
0x366f10
```

The whole-image xref count is very large, proving this is shared kernel state rather than a Transsion-only field.

The bytes are:

```text
08 db fe ff 00 00 00 00
```

which gives low 32-bit value `-75000`.

The X683 build uses HZ=250 and Linux defines:

```c
INITIAL_JIFFIES = -300 * HZ
```

so `INITIAL_JIFFIES == -75000`. citeturn472669search0

Kernel timer code initializes `jiffies_64` from `INITIAL_JIFFIES`. citeturn605116search2

The raw Image also contains the `jiffies` symbol name in its kernel string data.

Therefore the global is now classified with **high confidence** as:

```text
jiffies_64 / jiffies backing storage
```

The GC code uses it as an absolute jiffies-domain timing reference.

## Superseded interpretation

The former neutral label `x683_shared_time_global_16c6980` is obsolete. It was a temporary producer-independent label used before the `INITIAL_JIFFIES` value match was established.
