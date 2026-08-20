#!/usr/bin/env python3
"""Fresh executable-level metrics and conservative BL/BLR inventory for X683."""
from __future__ import annotations
import argparse, bisect, hashlib, json, re, struct
from pathlib import Path

def sha256(path: Path) -> str:
    h = hashlib.sha256(); h.update(path.read_bytes()); return h.hexdigest()

def parse_kallsyms(path: Path):
    out = []
    for line in path.read_text(errors='replace').splitlines():
        m = re.match(r'([0-9a-fA-F]+)\s+([A-Za-z])\s+(\S+)(?:\s+(\S+))?$', line)
        if m:
            out.append((int(m.group(1), 16), m.group(2), m.group(3), m.group(4)))
    return out

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument('image')
    ap.add_argument('kallsyms')
    ap.add_argument('-o', '--output', default='x683-executable-analysis.json')
    args = ap.parse_args()
    image_path = Path(args.image)
    image = image_path.read_bytes()
    symbols = parse_kallsyms(Path(args.kallsyms))
    stext = next(v for v, _, n, _ in symbols if n == '_stext')
    text_offset = 0x80000
    base = stext - text_offset
    limit = base + len(image)
    all_funcs = sorted({v for v, t, _, _ in symbols if t in 'TtWwVv'})
    kernel_funcs = [v for v in all_funcs if base <= v < limit]
    ranges = []
    for i, value in enumerate(kernel_funcs):
        nxt = kernel_funcs[i + 1] if i + 1 < len(kernel_funcs) else limit
        end = min(nxt, limit)
        if end > value:
            ranges.append((value, value - base, end - base))
    starts = set(kernel_funcs)
    bl_sites = bl_mapped = bl_exact = blr = 0
    callers = set(); exact_callers = set()
    for value, off, end in ranges:
        for pos in range(off, end - 3, 4):
            ins = struct.unpack_from('<I', image, pos)[0]
            if (ins & 0xfc000000) == 0x94000000:
                bl_sites += 1
                imm = ins & 0x03ffffff
                if imm & (1 << 25):
                    imm -= 1 << 26
                target = base + pos + (imm << 2)
                if target in starts:
                    bl_exact += 1
                    exact_callers.add(value)
                idx = bisect.bisect_right(kernel_funcs, target) - 1
                if idx >= 0 and kernel_funcs[idx] <= target < (kernel_funcs[idx + 1] if idx + 1 < len(kernel_funcs) else limit):
                    bl_mapped += 1
                    callers.add(value)
            if (ins & 0xfffffc1f) == 0xd63f0000:
                blr += 1
    result = {
        'image_sha256': sha256(image_path),
        'image_size': len(image),
        'kallsyms_sha256': sha256(Path(args.kallsyms)),
        'kallsyms_total_entries': len(symbols),
        'kallsyms_function_entries': sum(t in 'TtWwVv' for _, t, _, _ in symbols),
        'unique_function_starts_all': len(all_funcs),
        'unique_kernel_function_starts': len(kernel_funcs),
        'kernel_file_base': hex(base),
        '_stext': hex(stext),
        'text_offset': hex(text_offset),
        'direct_bl_instruction_sites': bl_sites,
        'direct_bl_mapped_edges': bl_mapped,
        'direct_bl_exact_symbol_start_edges': bl_exact,
        'direct_bl_unique_callers': len(callers),
        'direct_bl_caller_coverage': len(callers) / len(kernel_funcs),
        'direct_bl_exact_unique_callers': len(exact_callers),
        'indirect_blr_sites': blr,
        'known_symbols': {n: hex(v) for v, _, n, _ in symbols if n in ('f2fs_gc', 'schedule', '_stext', '_einittext')},
    }
    Path(args.output).write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))

if __name__ == '__main__':
    main()
