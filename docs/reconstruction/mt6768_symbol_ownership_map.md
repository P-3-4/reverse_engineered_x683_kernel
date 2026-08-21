# MT6768 Symbol Ownership Map

## Scope

Initial evidence-backed symbol ownership map for X683/H694 MT6768 Linux 4.14.141 kernel reconstruction.

Sources:

- stock x683_kallsyms.txt
- stock kernel configuration
- extracted boot image DTB notes
- existing reconstruction state

This document only records ownership where evidence exists. Unknown source ownership is marked UNKNOWN.

---

## MSDC / MediaTek Storage Host

Subsystem: MediaTek MSDC

Evidence:

- kallsyms contains a large `msdc_*` symbol family.
- DTB contains `msdc@11240000` with `compatible = mediatek,msdc`.
- Storage dependency requires MediaTek host controller before MMC block layer.

Representative symbols:

| Symbol family | Ownership | Confidence |
|---|---|---|
| msdc_* | MediaTek MSDC vendor driver | High |
| mtk_msdc_* | UNKNOWN vendor naming variant | Medium |

Known unresolved:

- exact vendor source file layout
- exact probe path
- exact callback implementation

---

## MMC Core

Subsystem: Linux MMC

Evidence:

- kallsyms contains mmc_* and mmc_blk* symbols.

| Symbol family | Ownership | Confidence |
|---|---|---|
| mmc_* | Linux MMC core | High |
| mmc_blk* | Linux MMC block layer | High |

---

## MT6768 Clock Provider

Subsystem: Clock framework

Evidence:

- kallsyms contains `clk_mt6768_init`.
- DTB storage node references `msdc1-clock` and `msdc1-hclock`.

| Symbol | Ownership | Confidence |
|---|---|---|
| clk_mt6768_init | MT6768 clock provider | High |

Unknown:

- clock gate table ownership
- mux and PLL source mapping

---

## MT6768 Pinctrl

Subsystem: pinctrl

Evidence:

- kallsyms contains MT6768 pinctrl symbols.

| Symbol | Ownership | Confidence |
|---|---|---|
| mt6768_pinctrl_probe | MT6768 pinctrl driver | High |
| mt6768_pinctrl_init | MT6768 pinctrl initialization | High |

---

## MT6358 Regulator

Subsystem: PMIC regulator

Evidence:

- kallsyms contains MT6358 regulator symbols.
- DTB storage node references vmmc/vqmmc supplies mapped to MT6358 rails.

| Symbol | Ownership | Confidence |
|---|---|---|
| mt6358_regulator_probe | MT6358 regulator driver | High |

---

## Reconstruction Status

Completed:

- initial subsystem ownership classification
- evidence boundaries documented

Not yet reconstructed:

- MSDC probe sequence
- host allocation sequence
- DT property parsing
- regulator enable ordering
- clock acquisition sequence

No proprietary source ownership has been assumed without evidence.
