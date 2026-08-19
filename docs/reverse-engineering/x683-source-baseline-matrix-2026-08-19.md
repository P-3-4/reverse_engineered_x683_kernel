# X683 Historical Source Baseline Matrix — 2026-08-19

| area | evidence | confidence |
|---|---|---|
| Kernel core | Linux `4.14.141+` strings/symbols/config | HIGH |
| F2FS | 4.14 Android/common + upstream F2FS history matches API/source-path family; X683 symbols/layouts are authoritative | HIGH for family, MEDIUM for exact revision |
| MediaTek | MT6768-specific source paths, config and symbols | HIGH for platform family |
| Transsion | `tran_*` symbols/config and direct binary control flow | HIGH for presence, LOW for exact git revision |

Historical public Android/Linux 4.14 source is used for naming and semantic correlation only. When historical source and X683 binary differ, the binary wins.
