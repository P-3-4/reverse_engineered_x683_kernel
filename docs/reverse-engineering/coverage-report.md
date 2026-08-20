# X683 Coverage Report

## Executable coverage

- kallsyms entries: 56,976
- function entries: 56,975
- unique kernel function starts: 52,784
- direct BL sites: 295,805
- direct BL mapped edges: 270,139
- exact symbol-start BL edges: 1,772
- direct-call callers: 35,034
- direct-call caller coverage: 66.3723856%
- BLR sites: 11,692
- existing conservative static BLR candidates: 922
- independently rechecked simple ADRP+ADD+LDR chains: 49
- exact targets from that simple recheck: 0

## Structural coverage

Six F2FS layout anchors are high-confidence. Vendor-added F2FS state outside proven offsets remains unresolved.

## Source/build coverage

No complete exact Transsion/X683 4.14.141 source tree has been proven. No ARM64 build is claimed.
