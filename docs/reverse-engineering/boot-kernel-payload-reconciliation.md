# X683/H694 boot kernel payload reconciliation

## Evidence

The supplied `x683_boot.img` is 32 MiB and has SHA-256:

```text
a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180
```

The Android boot header reports:

```text
page_size       = 0x800 (2048)
kernel_size     = 9755348 bytes
kernel_addr     = 0x40080000
```

The kernel payload begins at file offset `0x800`.

The separately supplied `x683_kernel_compressed.gz` is 9,640,652 bytes and has SHA-256:

```text
6ddfd017d9ee7152a856f46657f9ddd5287adf69d49cb853f7e747c2b7c18dfd
```

## Reconciliation result

The kernel payload inside `boot.img` starts with the same gzip stream as the supplied compressed kernel. Parsing the first gzip member shows:

```text
boot.img kernel payload:  9,755,348 bytes
first gzip member:        9,640,652 bytes
trailing payload bytes:     114,696 bytes
```

The gzip member in both locations decompresses to exactly the same 26,615,820-byte stream with SHA-256:

```text
96513877085ad4784a17d7b51f4109650bfe90449f0e6a2b77681fa55c3ca7ba
```

Therefore the separately supplied compressed kernel is not a different kernel revision. It is the exact gzip member carried at the beginning of the boot image kernel payload; the boot image contains an additional 114,696 bytes after the gzip member.

## Consequence for reconstruction

Use the decompressed gzip member as the kernel-image evidence source. Do not compare the complete `boot.img` kernel payload byte-for-byte against the standalone `.gz` file: the latter intentionally excludes the post-gzip bytes present in the boot-image kernel region.

The boot-image header and gzip reconciliation also independently confirms the kernel size recorded in `bootimg-analysis-manifest.md`.

## Confidence

High. This conclusion is based on direct SHA-256 comparison of the decompressed gzip streams and direct parsing of the Android boot header/payload boundaries.
