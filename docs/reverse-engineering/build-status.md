# Build status

## Milestone 0 — repository and evidence

- [x] Git repository established
- [x] `reconstruction` branch established
- [x] stock X683/H694 kernel fingerprint recorded
- [x] F2FS binary layout work recorded
- [x] Transsion GC behavior reconstruction recorded

## Milestone 1 — source reconstruction

- [x] X683 F2FS offset compatibility header
- [x] Transsion GC reconstruction skeleton
- [x] recovered F2FS configuration fragment
- [ ] exact vendor-era F2FS source revision
- [ ] complete MT6768 kernel base
- [ ] exact X683/H694 defconfig
- [ ] X683/H694 DTS/DTSI
- [ ] Transsion board-specific drivers

## Milestone 2 — first build

- [ ] kernel source compiles
- [ ] `Image.gz` produced
- [ ] DTB produced
- [ ] modules produced
- [ ] boot image packaged

## Milestone 3 — device validation

- [ ] first boot
- [ ] F2FS mounts userdata with `tran_gc`
- [ ] storage/USB/display/PMIC functional
- [ ] stock userspace reaches Android framework

The repository must not label the kernel bootable until Milestone 2 and a device test pass are complete.
