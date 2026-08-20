#!/usr/bin/env python3
"""Rebuild the X683 executable inventory and direct-BL graph from local artifacts."""
import bisect, gzip, json, struct, sys
from pathlib import Path

if len(sys.argv) != 3:
    raise SystemExit("usage: rebuild_x683_analysis.py <Image> <x683_kallsyms.txt>")
image_path, kallsyms_path = map(Path, sys.argv[1:])
image = image_path.read_bytes()
base = 0xffffff92d0a00000
entries = []
for line in kallsyms_path.read_text(errors="replace").splitlines():
    p = line.split()
    if len(p) >= 3 and p[1] in "TtWw":
        try: entries.append((int(p[0], 16), p[1], p[2]))
        except ValueError: pass
by = {}
for a, t, n in entries: by.setdefault(a, []).append((t, n))
starts = sorted(by)
funcs = []
for i, a in enumerate(starts):
    end = starts[i + 1] if i + 1 < len(starts) else base + len(image)
    if 0 <= a - base < len(image) and end > a: funcs.append((a, end - a, by[a]))
start_only = [a for a, _, _ in funcs]
def containing(x):
    i = bisect.bisect_right(start_only, x) - 1
    if i < 0: return None
    end = start_only[i + 1] if i + 1 < len(start_only) else base + len(image)
    return start_only[i] if x < end else None
cal, callers = {}, {}
for a, size, _ in funcs:
    off, end = a - base, min(a - base + size, len(image))
    for p in range(off, end - 3, 4):
        ins = struct.unpack_from("<I", image, p)[0]
        if ins & 0xfc000000 != 0x94000000: continue
        imm = ins & 0x03ffffff
        if imm & 0x02000000: imm -= 0x04000000
        target = base + p + (imm << 2)
        owner = containing(target)
        if owner is None: continue
        cal.setdefault(a, set()).add(owner); callers.setdefault(owner, set()).add(a)
with gzip.open("x683-function-inventory.jsonl.gz", "wt", encoding="utf-8") as out:
    for a, size, names in funcs:
        out.write(json.dumps({"address":f"0x{a:016x}","size":size,"symbol":names[0][1],"aliases":[n for _,n in names[1:]],"callers":[f"0x{x:016x}" for x in sorted(callers.get(a,()))],"callees":[f"0x{x:016x}" for x in sorted(cal.get(a,()))]},separators=(",", ":"))+"\n")
with gzip.open("x683-callgraph.jsonl.gz", "wt", encoding="utf-8") as out:
    for a in sorted(cal):
        for t in sorted(cal[a]):
            out.write(json.dumps({"caller":f"0x{a:016x}","callee":f"0x{t:016x}","caller_symbol":by[a][0][1],"callee_symbol":by[t][0][1]},separators=(",", ":"))+"\n")
print(f"functions={len(funcs)} callers={len(callers)}")
