# X683/H694 GC reconstruction — full sanity check

Date: 2026-08-18

## Verdict

The repository now contains a substantially corrected binary-derived X683/H694 GC reconstruction. It is still **not build-proven or byte-accurate source replacement**.

## Fresh binary verification

The supplied stock `boot(8).img` was re-read directly and independently hashed:

- Boot SHA-256: `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`
- Boot size: `33,554,432` bytes
- Kernel gzip offset: `0x800`
- Decompressed Image size: `26,615,820` bytes
- Decompressed Image SHA-256: `96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba`

The fresh bytes match the repository's canonical binary hashes.

## Corrections from this sanity pass

### 1. `0x366cd4` boundary

Previous `0x366edc` termination was wrong.

The function continues through:

```text
0x366ee0 .. 0x366f28
```

with the canary failure call at `0x366f2c` and the next function beginning at `0x366f30`.

### 2. Seven-field logic

Previous wording said the seven SBI fields had to be all zero to permit the normal path. That was wrong.

Fresh machine code proves:

```text
any nonzero among:
  +0x44c +0x450 +0x454 +0x448 +0x444 +0x45c +0x458
    -> active path at 0x366da4

all seven zero
    -> alternate clean path at 0x366ee0
```

`gc_mode == 3` bypasses this discriminator and goes to the shared stage at `0x366de4`.

### 3. Clean-path tail recovered

`0x366ee0..0x366f28` is live policy code:

```text
sbi+0x80 object
  -> child+0x2090 escalation test
  -> list+0x24 escalation test
  -> 250*sbi+0x1d0 + sbi+0x1a0
  -> compare against Image+0x16c6980
  -> either active path or shared stage
```

This path had previously been omitted from the function boundary.

### 4. Terminal-call argument corrected

Fresh bytes show:

```text
0x3e1014(stack_object)
0x34e224(sbi,1)
0x3e1558(stack_object)
```

`0x34e224` receives the SBI pointer, not the temporary stack object.

### 5. Terminal-path conditionality corrected

At `0x366e7c`, `sbi+0x4b9` bit 7 controls only the three TLS/list helpers.

Once the terminal path is reached:

```text
0x341250(sbi->sb,1)
stat_info+0x16c++
```

execute unconditionally.

### 6. Shared policy global corrected

The ADRP/LDR sequence resolves to:

```text
Image + 0x16c6980
```

not the previously documented `Image + 0x16c6000 + 0xc14` approximation.

### 7. Controller mapping revalidated

Fresh wrapper disassembly confirms:

```text
controller 0 -> mode unchanged
controller 1 -> temporary gc_mode = 2
controller 2 -> temporary gc_mode = 3
```

Therefore Stop-4/5's controller value `2` produces the temporary `GREEDY` (`gc_mode=3`) path.

## Other sanity checks

`0x35cc18` selector dispatch is directly confirmed for values `0..5`:

```text
0 -> 0x35cc7c
1 -> 0x35ccc8
2 -> 0x35cd14
3 -> 0x35cd2c
4 -> 0x35cd78
5 -> 0x35cdb4
```

`0x373108` remains a real vendor threshold/accounting helper after an initial `sbi+0x4b9` bit5 gate.

`0x3e1014` and `0x3e1558` form a paired TLS temporary-object lifecycle.

`0x341250` is retained as an anonymous terminal filesystem synchronization/balance-style helper; its original source symbol is not promoted from call shape alone.

## Current authoritative artifacts

- `docs/reverse-engineering/x683-366cd4-byte-sanity-pass.md`
- `docs/reverse-engineering/bootimg-366cd4-byte-sanity.hex`
- `fs/f2fs/tran_gc_policy_reconstructed.c`
- `docs/reverse-engineering/x683-366cd4-vendor-policy-final.md`
- `docs/reverse-engineering/vendor-control-final-status.md`

## Current honest status

- Binary hashes: verified against fresh uploaded image.
- `tran_f2fs_gc` controller mapping: high confidence.
- X683 `0x3503a8` four-argument GC boundary: high confidence.
- `0x366cd4` branch topology: now byte-checked through its full live tail.
- Vendor helper symbolic names: incomplete.
- Exact X683-vs-stock `gc_node_segment()/gc_data_segment()` delta: incomplete.
- Buildability: **not established**.
- Replacement-kernel readiness: **not established**.
