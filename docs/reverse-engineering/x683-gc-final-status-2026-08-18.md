# X683/H694 GC — final status after full synthesis

## Binary

```text
boot SHA-256  = a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180
Image SHA-256 = 96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba
Image size    = 26,615,820
```

## End-to-end path

```text
Transsion detector/thread
    |
    +-- state/arming predicates
    +-- freezer/per-CPU synchronization gate
    |
    v
tran_f2fs_gc() @ 0x37ada8
    |
    +-- controller 0: normal
    +-- controller 1: temporary gc_mode=2
    +-- controller 2: temporary gc_mode=3
    |
    v
X683 f2fs_gc() @ 0x3503a8
    |
    +-- historical F2FS GC core
    +-- victim selection
    +-- migration
    +-- statistics
    +-- checkpoint/retry/cleanup
    |
    v
vendor post-GC policy @ 0x366cd4
    |
    +-- policy selector 4/1/0
    +-- I/O activity discriminator
    +-- fixed-point/reservation gates
    +-- selector 1/3
    +-- jiffies gate
    +-- optional f2fs_balance_fs(sbi,true)
    +-- X683 superblock sync/checkpoint helper
    +-- bg_cp_count++
```

The detector-side call site at `0x377410..0x37742c` proves the ordering:

```text
tran_f2fs_gc(sbi)
    -> on success
0x366cd4(sbi)
    -> release per-CPU/superblock synchronization
```

## Resolved major ambiguities

```text
seven SBI fields
    = writeback/read/direct-I/O counters

stat +0x164
    = call_count

stat +0x168
    = cp_count

stat +0x16c
    = bg_cp_count

Image +0x16c6980
    = jiffies_64/jiffies backing storage
```

The last identity is strongly supported by the exact initial value `-75000` at HZ=250 and Linux's `INITIAL_JIFFIES = -300*HZ`. citeturn472669search0turn605116search2

## Remaining source-attribution gaps

```text
non-initialization writers for +0x444/+0x44c/+0x454/+0x45c
original symbolic names of 0x35cc18 selectors 0..5
exact public symbol for 0x341250
exact vendor source names for a few SBI threshold fields
final names of the three tail memory-accounting fields at +0x220/+0x228/+0x230
```

These are source-name gaps. The GC architecture, policy branch topology, statistics family, I/O gate, controller mapping and shared time reference are resolved to high confidence.

## Build status

The semantic reconstructions are not yet a buildable replacement kernel. Exact X683 4.14.141 source integration, vendor driver restoration, DT/DTBO reconstruction and symbol/control-flow equivalence remain before boot testing.
