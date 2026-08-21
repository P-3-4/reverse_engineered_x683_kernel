#!/usr/bin/env python3
"""Reconstruct a readable DTS from the appended X683 stock boot DTB.

Evidence-first tool: it preserves raw property bytes as cells/byte arrays and
only decodes properties with known DT string semantics. Numeric phandles are
left numeric so the binary values remain directly traceable.
"""
import argparse
import struct
from collections import defaultdict
from pathlib import Path

STR_PROPS = {
    "compatible", "model", "bootargs", "status", "device_type", "clock-names",
    "pinctrl-names", "label", "regulator-name", "name", "clock-output-names",
    "power-domain-names", "interrupt-names", "reset-names", "dma-names",
    "gpio-names", "phy-names", "io-channel-names",
}


def parse_fdt(data):
    magic, total, struct_off, strings_off, _, _, _, _, strings_size, struct_size = struct.unpack(">10I", data[:40])
    if magic != 0xD00DFEED or total > len(data):
        raise ValueError("invalid FDT header")
    end = struct_off + struct_size
    stack, nodes, props = [], [], []
    def sname(off):
        p = strings_off + off
        e = data.find(b"\0", p, strings_off + strings_size)
        if e < 0:
            raise ValueError("unterminated property name")
        return data[p:e].decode("ascii", "replace")
    p = struct_off
    while p < end:
        tag = struct.unpack_from(">I", data, p)[0]
        p += 4
        if tag == 1:
            e = data.find(b"\0", p, end)
            if e < 0: raise ValueError("unterminated node name")
            name = data[p:e].decode("ascii", "replace")
            p = (e + 4) & ~3
            stack.append(name)
            nodes.append("/" + "/".join(x for x in stack if x))
        elif tag == 2:
            stack.pop()
        elif tag == 3:
            length, nameoff = struct.unpack_from(">II", data, p)
            p += 8
            value = data[p:p + length]
            p = (p + length + 3) & ~3
            props.append(("/" + "/".join(x for x in stack if x), sname(nameoff), value))
        elif tag == 4:
            continue
        elif tag == 9:
            break
        else:
            raise ValueError(f"unknown FDT token {tag} at 0x{p-4:x}")
    return nodes, props


def fmt(name, value):
    if not value:
        return None
    parts = value.split(b"\0")
    if parts and parts[-1] == b"":
        parts.pop()
    if name in STR_PROPS and parts and all(s and all(32 <= c < 127 for c in s) for s in parts):
        return ", ".join('"' + s.decode("ascii").replace('"', '\\"') + '"' for s in parts)
    if len(value) % 4 == 0:
        words = struct.unpack(">" + "I" * (len(value) // 4), value)
        return "<" + " ".join(f"0x{x:08x}" for x in words) + ">"
    return "[" + value.hex(" ") + "]"


def render(nodes, props):
    by = defaultdict(list)
    children = defaultdict(list)
    for path, name, value in props:
        by[path].append((name, value))
    for path in nodes:
        if path == "/":
            continue
        parts = path.strip("/").split("/")
        parent = "/" + "/".join(parts[:-1]) if len(parts) > 1 else "/"
        children[parent].append(path)
    out = ["/dts-v1/;", "", "/* Generated from stock X683/H694 DTB; not original vendor source. */", ""]
    def emit(path, depth):
        name = "/" if path == "/" else path.rsplit("/", 1)[-1]
        out.append("\t" * depth + ("/ {" if path == "/" else name + " {"))
        for prop, value in by.get(path, []):
            rendered = fmt(prop, value)
            out.append("\t" * (depth + 1) + (prop + ";" if rendered is None else prop + " = " + rendered + ";"))
        for child in children.get(path, []):
            emit(child, depth + 1)
        out.append("\t" * depth + "};")
    emit("/", 0)
    return "\n".join(out) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("boot_img", type=Path)
    ap.add_argument("-o", "--output", type=Path, required=True)
    ap.add_argument("--dtb-offset", type=lambda x: int(x, 0), default=9642700)
    args = ap.parse_args()
    blob = args.boot_img.read_bytes()[args.dtb_offset:]
    nodes, props = parse_fdt(blob)
    args.output.write_text(render(nodes, props), encoding="utf-8")
    print(f"nodes={len(nodes)} properties={len(props)} output={args.output}")


if __name__ == "__main__":
    main()
