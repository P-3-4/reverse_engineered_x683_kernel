# X683/H694 Transsion GC helper-cluster deep pass

Source: stock `boot(8).img`, SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.

## 0x35cc90 — multi-mode GC policy predicate

Signature observed at the call sites:

```c
bool helper(struct f2fs_sb_info *sbi, unsigned int mode);
```

The function dispatches on `mode` values `0..5` and returns a boolean.

Recovered call sites from the vendor GC policy gate:

```text
0x366d74: helper(sbi, 4)
0x366d90: helper(sbi, 1)
0x366dac: helper(sbi, 0)
0x366e60: helper(sbi, 1)
```

Modes are not yet assigned public vendor names. The bodies are distinct policy tests involving F2FS SM state and vendor thresholds.

Mode table at Image+0xE74030:

```text
mode 0 -> +0x00
mode 1 -> +0x4c
mode 2 -> +0x98
mode 3 -> +0xb0
mode 4 -> +0xfc
mode 5 -> +0x138
```

Important: this is a **boolean policy helper**, not the actual GC executor.

## 0x362CB8 — heavy GC execution path

The function accepts three logical inputs:

```text
sbi
arg1
arg2
```

It immediately acquires the GC-related lock at `sm_info + 0xb8`, walks dirty/free/victim state, performs segment selection/migration work, and returns an integer result.

The control flow and data structures match the heavy F2FS GC/migration machinery rather than a simple policy predicate.

At the Transsion policy wrapper the call is:

```text
0x366dbc -> 0x362cb8(sbi, 0, 0)
```

This is therefore the direct execution path reached after the vendor policy predicate permits GC.

The exact relationship between this three-argument vendor entry and the historical four-argument stock `f2fs_gc(sbi, sync, background, segno)` still requires a caller/ABI reconciliation pass; do not yet replace the repository's four-argument reconstruction with this signature solely from this call site.

## 0x363300 — dirty/list cleanup helper

This helper:

- validates `sm_info + 0x70` segment-space size against `0xe39`;
- takes a lock around `sm_info + 0xb8`;
- iterates a linked list rooted at `sm_info + 0x98`;
- removes entries from the list;
- decrements the associated count at `sm_info + 0xA8`;
- releases the lock;
- returns `requested_count - remaining_count`.

At the vendor wrapper:

```text
0x366dd0 -> helper(sbi, 0xe38)
```

So the wrapper is explicitly draining/cleaning up up to `0xe38` list entries on this branch. The exact public source-level helper name is unresolved.

## 0x3412C8 — write/freezer-aware superblock operation

Call site:

```text
0x366f18:
    x0 = sbi->sb
    w1 = 1
    bl 0x3412c8
```

The helper accesses the superblock's backing state, consults freezer/operation state, conditionally takes a superblock-related lock, invokes the registered operation chain, and releases it.

The machine code is consistent with a superblock write/freezer synchronization wrapper. Its public symbol is not yet proven and should remain unnamed in source reconstruction.

## Wrapper branch map around 0x366D4C

High-confidence control flow:

```text
filesystem dirty/error guard
    |
    +-- helper(sbi, 4)
    |     false -> auxiliary path 0x373180
    |
    +-- helper(sbi, 1)
    |     false -> helper(sbi, 0x1c7)
    |
    +-- helper(sbi, 0)
    |
    +-- gc_mode == 3 ?
    |       yes -> urgent/alternate branch
    |
    +-- normal capacity/current-state guards
    |
    +-- helper(sbi, 1)
    |       false -> continue toward cleanup/return
    |
    +-- helper(sbi, 3)
    |       false -> continue toward cleanup/return
    |
    +-- capacity/current-segment thresholds
    |
    +-- policy checks / mount options
    |
    +-- 0x3412c8(sb, 1)
    |
    +-- stat_info +0x16c++
```

The wrapper is therefore not a thin `gc_mode -> f2fs_gc` shim; it contains multiple independent policy gates and cleanup branches surrounding the actual GC execution path.

## Remaining exact targets

1. Resolve the public names/semantics of modes `0..5` in `0x35cc90`.
2. Reconcile the direct `0x362cb8(sbi,0,0)` ABI with the historical four-argument `f2fs_gc()` path.
3. Resolve the list structure behind `0x363300`.
4. Identify the exact kernel symbol corresponding to `0x3412c8`.
5. Integrate the complete `0x366d4c..0x366f34` branch structure into the reconstructed vendor wrapper.
