# X683 Unresolved Evidence Register — 2026-08-19

| item | state | confidence |
|---|---|---|
| Exact Transsion source git revision | not proven | LOW |
| Formal vendor global-state C struct | offset-backed only | MEDIUM/HIGH per field |
| Adjacent `f2fs_sb_info` fields | partial | MEDIUM/HIGH where consumers prove them |
| `sit_info + 0x10` exact historical member name | semantic use proven, name unresolved | MEDIUM |
| Indirect proc-op/event callback container layouts | handlers identified, container layout incomplete | MEDIUM |
| Static device-tree contents | supplied DT tar contains only a symlink | HIGH that artifact is insufficient |
| Exact source mapping for every executable | symbols complete; source mapping partial | HIGH |
| Inline helper standalone addresses | not all helpers survive as kallsyms | HIGH |

Unknowns remain offset-backed rather than being silently assigned historical member names.
