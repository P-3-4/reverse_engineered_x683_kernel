# X683 stock DTB storage evidence — 2026-08-21

## Source

The supplied `x683_boot.img` was parsed directly. The appended FDT begins at boot-image offset `0x9386EC` (decimal `9,642,700`) and has total size `114,696` bytes. FDT header fields are internally consistent: structure block starts at `0x38`, strings block at `0x1874C`, structure block size `100,212`, strings size `14,428`.

The resulting tree contains 542 nodes and 2,645 properties.

## Stock storage nodes

### `/msdc@11230000`

- `compatible = "mediatek,msdc"`
- `reg = <0x0 0x11230000 0x0 0x10000>`
- `interrupts = <0x0 0x64 0x8>`
- `index = 0`
- `clk_src = 1`
- `bus-width = 8`
- `max-frequency = 0x0bebc200`
- `cap-mmc-highspeed`
- `mmc-ddr-1_8v`
- `mmc-hs200-1_8v`
- `mmc-hs400-1_8v`
- `no-sd`, `no-sdio`, `non-removable`
- `pinctl = <0x30>`
- `pinctl_hs400 = <0x31>`
- `pinctl_hs200 = <0x32>`
- `register_setting = <0x33>`
- `bootable`
- `status = "okay"`
- `vmmc-supply = <0x34>`
- clocks = `<0x21 0x4c 0x21 0x1c 0x21 0x44>`
- clock names = `msdc0-clock`, `msdc0-hclock`, `msdc0-aes-clock`
- phandle `0xc1`

### `/msdc@11240000`

- `compatible = "mediatek,msdc"`
- `reg = <0x0 0x11240000 0x0 0x10000>`
- `interrupts = <0x0 0x65 0x8>`
- `index = 1`
- `clk_src = 2`
- `bus-width = 4`
- `max-frequency = 0x0bebc200`
- `cap-sd-highspeed`
- `sd-uhs-sdr12`, `sd-uhs-sdr25`, `sd-uhs-sdr50`, `sd-uhs-sdr104`, `sd-uhs-ddr50`
- `no-mmc`, `no-sdio`
- `pinctl = <0x35>`
- `pinctl_sdr104 = <0x36>`
- `pinctl_sdr50 = <0x37>`
- `pinctl_ddr50 = <0x38>`
- `register_setting = <0x39>`
- `host_function = 1`
- `cd_level = 1`
- `cd-gpios = <0x1e 0x4 0x0>`
- `status = "okay"`
- `vmmc-supply = <0x3a>`
- `vqmmc-supply = <0x3b>`
- clocks = `<0x21 0x4d 0x21 0x1d>`
- clock names = `msdc1-clock`, `msdc1-hclock`
- phandle `0xc2`

## Top blocks

- `/msdc0_top@11cd0000`: `compatible = "mediatek,msdc0_top"`, `reg = <0x0 0x11cd0000 0x0 0x1000>`
- `/msdc1_top@11c90000`: `compatible = "mediatek,msdc1_top"`, `reg = <0x0 0x11c90000 0x0 0x1000>`

## Pinctrl groups

The stock DTB contains these phandle groups: `0x30` `/pinctrl/msdc0@default`, `0x31` `/pinctrl/msdc0@hs400`, `0x32` `/pinctrl/msdc0@hs200`, `0x33` `/pinctrl/msdc0@register_default`, `0x35` `/pinctrl/msdc1@default`, `0x36` `/pinctrl/msdc1@sdr104`, `0x37` `/pinctrl/msdc1@sdr50`, `0x38` `/pinctrl/msdc1@ddr50`, and `0x39` `/pinctrl/msdc1@register_default`.

Register-default groups explicitly contain `cmd_edge=0`, `rdata_edge=0`, `wdata_edge=0`.

The pin child property values recovered are drive-strength `0x03` for the default groups. MSDC0 HS400/HS200 use `0x04` for data/clock/ds and `0x03` for command/reset. MSDC1 SDR104/SDR50/DDR50 expose drive-strength `0x03` on command/data/clock.

## Supply phandle ownership

- `0x34` → `/pwrap@1000d000/mt6358-pmic/mt6358regulator/ldo_vemc`
- `0x3a` → `/pwrap@1000d000/mt6358-pmic/mt6358regulator/ldo_vmch`
- `0x3b` → `/pwrap@1000d000/mt6358-pmic/mt6358regulator/ldo_vmc`

Clock phandle `0x21` resolves to `/infracfg_ao@10001000`. Clock indices are copied directly from the DTB and are not interpreted as source-level API behavior.

## Reconstruction state

**HIGH-CONFIDENCE RECOVERED:** node addresses, interrupts, MMC capabilities, supply phandles, clock phandle/index tuples, DT clock names, pinctrl phandles, and register-setting values.

**UNKNOWN:** exact vendor driver source revision, private host structure layout, exact clock/regulator API call sequence, DMA setup, CQHCI implementation details, and pinctrl driver source ownership.

The companion DTS fragment is intentionally a source-level reconstruction, not a claim of original Transsion DTS source.
