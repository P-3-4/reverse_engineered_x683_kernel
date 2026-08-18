# F2FS GC API history and X683 constraint

Public F2FS history shows the GC prototype changed over time:

- Older 4.14-era code includes simpler two/three-argument forms.
- Later trees added background/force and victim-segment parameters.
- Newer kernels consolidated GC arguments into `struct f2fs_gc_control`.

The X683 stock binary is not to be identified from historical API chronology alone. Direct AArch64 disassembly of the stock entry at `0x3503a8` establishes the callable interface as:

```c
int f2fs_gc(struct f2fs_sb_info *sbi,
            bool sync,
            bool background,
            unsigned int segno);
```

Stock call sites pass either a real segment or `-1` (`NULL_SEGNO`), and the Transsion wrapper passes `-1`.

Therefore the four-argument form is the current X683 ABI. Older three-argument statements in historical project documents are superseded and must not be used for source reconstruction.

The exact vendor-era implementation behind this ABI still requires matching against the historical F2FS source revision and the remaining stock helper/call-site disassembly.
