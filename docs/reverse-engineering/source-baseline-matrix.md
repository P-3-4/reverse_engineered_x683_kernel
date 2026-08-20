# X683 Source Baseline Matrix

| Candidate | Evidence | Mismatch / gap | Confidence |
|---|---|---|---|
| X683-H694 `211105V361` vendor release | Kernel build timestamp exactly matches 2021-11-05 release target; firmware correlation is high-confidence | Boot-image/package hash not matched; source tree revision not proven | High as release target, not source proof |
| Public MT6768 4.14 vendor trees | Architecture, scheduler, MMC and platform APIs provide useful correlation | Different vendor/device history; exact Transsion delta unresolved | Reference only |
| Mainline Linux 4.14.141 | Version/API baseline | Does not contain MT6768/Transsion vendor stack | Reference only |

Recovered Image build identity: Linux `4.14.141+`, Android clang `9.0.3`, LLVM `9.0.3svn`, `Fri Nov 5 15:56:25 CST 2021`.

No candidate is imported into the reconstructed kernel tree until source paths, signatures, layouts, strings, DT bindings and vendor deltas correlate with the Image.
