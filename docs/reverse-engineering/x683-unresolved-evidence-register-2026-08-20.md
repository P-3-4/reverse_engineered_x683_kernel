# X683 Unresolved Evidence Register — 2026-08-20

| item | status | reason |
|---|---|---|
| exact Transsion vendor git revision | unresolved | no revision metadata supplied |
| formal Transsion GC global struct | unresolved | offsets proven, formal type/name absent |
| adjacent `f2fs_sb_info` fields | partial | only directly accessed offsets promoted |
| indirect callback/ops containers | unresolved | BLR target registers need data-reference/container recovery |
| private MSDC host/CQ/error state | partial | direct accesses exist; complete container mapping not proven |
| ION/M4U ownership containers | partial | APIs/shrinkers mapped; private structures indirect |
| PPM/schedtune client structures | partial | callbacks proven; private tables unresolved |
| exact DT initcall ordering | unresolved | probe symbols known; full initcall pointer order not proven |
| WLAN/WMT/FPSGO module bodies | unresolved | module binaries not supplied |
| exact DSI probe container | unresolved | DT and DSI symbols prove path, not unique container |
| exact codec private state | unresolved | AFE/codec entry surface known; private layout pending |
