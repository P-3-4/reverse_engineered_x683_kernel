# X683 Unresolved Evidence Register

| ID | Item | Current state | Required evidence |
|---|---|---|---|
| U-001 | Exact Transsion/X683 kernel Git revision | unresolved | matching source tree plus binary/compiler/DT correlation |
| U-002 | Runtime BLR callback targets | 11,692 sites; 922 conservative candidates; 0 exact from simple static recheck | structure-base provenance and initializer tracing |
| U-003 | F2FS vendor fields around unresolved GC/vendor region | partial | more xrefs and field-use proof |
| U-004 | MSDC private structures | partial | allocation/use/destruction tracing |
| U-005 | Vendor module source outside Image | unresolved | WLAN/WMT/FPSGO and other module binaries/source |
| U-006 | Complete build tree | unavailable | exact or sufficiently correlated 4.14.141 vendor baseline |

Inference is not promoted to fact merely because it agrees with a public MT6768 source tree.
