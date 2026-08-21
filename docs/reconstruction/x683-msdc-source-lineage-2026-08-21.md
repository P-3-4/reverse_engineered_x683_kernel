# X683 MSDC source-lineage comparison — 2026-08-21

## Public reference candidate

A public MT6768 4.14 vendor tree was located at `Hadenix/kernel_umidigi_f2_mt6771_4.14`, commit `e494bc66f92dfacf9494a7ac8a40c225cca87181`.

Its `arch/arm/boot/dts/mediatek/cust_mt6768_msdc.dtsi` matches the stock X683 DTB at the structural level and across most values. The public file contains the same MSDC0/1 capabilities, 200 MHz maximum frequency, clock-source selections, pinctrl groups, register-setting groups, host-function values, supply relationships and clock-name strings. The public `mt6768-msdc.h` defines `MSDC_EMMC=0`, `MSDC_SD=1`, `MSDC1_CLKSRC_200MHZ=2`, and `MSDC_SMPL_RISING=0`, all matching the recovered stock values.

## Binary-confirmed X683 deltas

1. **SD card-detect GPIO differs.** Public DTS uses `cd-gpios = <&pio 18 0>`. Stock X683 DTB contains `cd-gpios = <0x1e 0x4 0x0>`, where phandle `0x1e` resolves to `/pinctrl`. Therefore the X683 DT uses GPIO index `4`, not the public candidate's `18`.

2. **Register-setting values match the public macros.** Stock `cmd_edge`, `rdata_edge`, and `wdata_edge` are all byte value `0`, which the public MT6768 binding identifies as `MSDC_SMPL_RISING`.

3. **MSDC0/1 clock and supply tuples match the public DT structure.** Stock uses the same clock-name strings: `msdc0-clock`, `msdc0-hclock`, `msdc0-aes-clock`, `msdc1-clock`, `msdc1-hclock`.

## Driver-source mismatch preventing direct import

The same public tree's `drivers/mmc/host/mtk-sd.c` contains `msdc_drv_probe`, but its probe acquires clocks using names `source`, `hclk`, and optional `source_cg`. This does **not** directly match the stock DT clock-name strings above. Therefore this driver file is a reference implementation, not yet a provable X683 source match.

Its probe nevertheless establishes useful historical API structure: `mmc_alloc_host`, `mmc_of_parse`, resource mapping, regulator acquisition, clock acquisition, IRQ acquisition, pinctrl lookup, `msdc_of_property_parse`, host-op assignment, DMA allocation, `msdc_init_gpd_bd`, delayed request timeout setup, `msdc_init_hw`, IRQ registration, runtime-PM setup and `mmc_add_host`.

## Current confidence

**HIGH-CONFIDENCE SOURCE-LINEAGE CORRELATION:** the public MT6768 DT binding and `cust_mt6768_msdc.dtsi` are close historical references for the X683 storage DT structure.

**NOT PROVEN:** that this public tree is the exact X683 vendor source baseline.

**ACTIONABLE DELTA:** use the public MT6768 source as a structural starting point, but retain stock X683 DT values and recover the vendor `msdc` implementation differences from the stock executable before integration.
