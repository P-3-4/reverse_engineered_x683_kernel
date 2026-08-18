# Boot-image artifact index

The raw boot.img is not stored because the available repository text API cannot create binary blobs. The image used for extraction is permanently identified by SHA-256:

`a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`

## Permanent artifacts

- `bootimg-gc-artifacts.md` — image identity, extraction parameters, controller map, stop-condition summary, ABI correction.
- `bootimg-gc-key-hex.txt` — raw hexadecimal bytes for the wrapper and Stop-condition core.
- `transsion-gc-stop-conditions-disassembly.md` — instruction-level Stop Conditions 1–5 and their controller effects.

## Extraction recipe

1. Verify boot.img SHA-256 against the value above.
2. Android boot page size is `0x800`.
3. Compressed kernel starts at `0x800` and has size `0x94dad4`.
4. Decompress the gzip member only; do not feed the trailing kernel-slot bytes to gzip as another member.
5. Use the resulting decompressed kernel offsets directly for the addresses documented here.

## Critical offsets

- `0x37ada8` — Transsion `tran_f2fs_gc` wrapper.
- `0x3503a8` — stock F2FS GC entry called by the wrapper.
- `sbi + 0x534` — stock `gc_mode`.
- controller `+0x998` — Transsion GC controller.
- controller `+0x9c0` — controller-write guard.
- controller `+0x9f8` — stop-result flag.
- controller `+0x9fc` — stop-condition number.
- controller `+0xa04` — periodic cadence selector.
- controller `+0xa08` — segment baseline.
- controller `+0xa0c` — written-segment baseline.

## ABI

Stock X683 uses:

`f2fs_gc(sbi, sync, background, segno)`

The Transsion wrapper passes `segno = -1` and temporarily changes `sbi+0x534` before restoring it.
