# F2FS GC API history and X683 constraint

Public F2FS history shows the GC prototype changed over time:

- Older 4.14-era code includes a simpler `f2fs_gc(sbi, bool sync)` form.
- Later code added background/force and victim-segment parameters.
- A later 4.14/Android-era tree exposes `f2fs_gc(sbi, bool sync, bool background, bool force, unsigned int segno)`.
- Newer kernels consolidated the arguments into `struct f2fs_gc_control`.

The X683 stock reverse-engineering record reports a direct call equivalent to:

    f2fs_gc(sbi, sync, true)

Therefore the X683 binary's callable ABI is not safely interchangeable with the currently inspected MT6768 reference tree. The reconstruction must select the exact vendor-era F2FS revision or add a local adapter after the stock call-site/register evidence is fully matched.

The three-argument observation is treated as stock evidence; it is not being overwritten by the newer public prototype.
