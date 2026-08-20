# X683 Subsystem Status Matrix

| Subsystem | Binary evidence | Reconstruction status | Build readiness |
|---|---|---|---|
| F2FS | Strong | Proven layouts, GC ABI and vendor GC symbols | Partial |
| Block/bio | Strong generic evidence | Call-path correlation pending | Not ready |
| MMC/MSDC | Strong symbols/DT context | Private structures and callback edges pending | Not ready |
| DMA/IRQ | Present | Runtime callback resolution pending | Not ready |
| ION/DMA-BUF/M4U | Present in project evidence | Vendor-specific callback graph incomplete | Not ready |
| Binder | Present in project evidence | Generic/vendor split incomplete | Not ready |
| Scheduler/schedtune | Present | Vendor hook graph incomplete | Not ready |
| PPM/cpufreq/thermal | Present | Policy/private structures incomplete | Not ready |
| GPU/display | Present | Driver-private graph incomplete | Not ready |
| PM/battery/charger/USB | Present | Runtime callback graph incomplete | Not ready |

`Not ready` means evidence is insufficient for safe source integration; it does not mean the subsystem is absent from the stock kernel.
