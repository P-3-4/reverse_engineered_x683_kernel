#!/usr/bin/env python3
"""Build a reproducible X683 function inventory from authoritative local artifacts.

Usage:
  rebuild_x683_inventory.py <Image> <x683_kallsyms.txt> <config> <dtb> <out.jsonl.gz>

The inventory distinguishes binary/kallsyms proof from lexical triage. It does not
invent BLR targets or source equivalence.
"""
import gzip, json, re, sys
from collections import defaultdict
from pathlib import Path

if len(sys.argv) != 6:
    raise SystemExit(__doc__)
image_path, kall_path, config_path, dtb_path, out_path = map(Path, sys.argv[1:])
image = image_path.read_bytes()
symbols = defaultdict(list)
for line in kall_path.read_text(errors="replace").splitlines():
    p=line.split()
    if len(p)<3 or p[1] not in "TtWw": continue
    try: a=int(p[0],16)
    except ValueError: continue
    symbols[a].append({"type":p[1],"symbol":p[2],"module":p[3][1:-1] if len(p)>=4 and p[3].startswith("[") else ""})
starts=sorted(symbols)

def subsystem(name):
    pats=[
      ("F2FS",r"f2fs|curseg|sit_|nat_|segment|checkpoint|orphan|gc_"),
      ("storage",r"msdc|mmc|cqhci|emmc|bio|blk_|request_queue|discard|tuning"),
      ("MM",r"kswapd|reclaim|shrink|oom|vmscan|page_alloc|slab|slub|vmalloc|compaction|migration|psi_|memcg|lru"),
      ("ION/M4U/DMA",r"ion|m4u|mva|iommu|smmu|dma_buf|dmabuf"),
      ("binder/android",r"binder|ashmem|wakelock|autosleep|android"),
      ("scheduler",r"schedtune|scheduler|sched_|enqueue_task|dequeue_task|select_task|cpuset|cgroup"),
      ("PPM/DVFS/thermal",r"ppm|cpufreq|dvfs|eem|pbm|thermal|cooling|mt_idle|idle_"),
      ("power",r"suspend|resume|pm_|spm|wakeup|wake_|regulator|power_"),
      ("display/GPU",r"gpu|ged|mali|disp|dsi|fb_|framebuffer|lcm|backlight"),
      ("battery/USB",r"battery|gauge|charger|bmt|usb|power_supply|mt6358"),
      ("input/sensors",r"touch|tpd|ilitek|sensor|accel|gyro|als|proximity|hall|input"),
      ("audio",r"afe|aud|audio|asoc|snd_|codec|i2s|pcm|mt_soc"),
      ("networking",r"wlan|wifi|cfg80211|wmt|btif|bluetooth|bt_|netdev|skb|tcp_|udp_|inet_"),
      ("security/crypto",r"selinux|security_|avc_|fscrypt|verity|crypto|aes_|sha|ghash"),
      ("arch/IRQ",r"irq|gic_|exception|el[0-3]_"),
    ]
    for n,p in pats:
        if re.search(p,name,re.I): return n
    return "other"

def vendor_class(name,module):
    if module: return "module"
    return "vendor-likely" if re.search(r"(^|_)(mtk|mt_|tran|msdc|ppm|eem|pbm|m4u|ion|ged|mali|wmt|btif|fpsgo|mt6358)(_|$)",name,re.I) or name.startswith("tran_") else "stock-or-unknown"

with gzip.open(out_path,"wt",encoding="utf-8") as out:
    for i,a in enumerate(starts):
        nxt=starts[i+1] if i+1<len(starts) else None
        size=(nxt-a) if nxt and nxt>a and nxt-a<=0x200000 else 0
        for j,e in enumerate(symbols[a]):
            out.write(json.dumps({
                "address":f"0x{a:016x}",
                "size":size,
                "type":e["type"],
                "symbol":e["symbol"],
                "module":e["module"],
                "aliases":[x["symbol"] for x in symbols[a] if x is not e],
                "subsystem":subsystem(e["symbol"]),
                "vendor_class":vendor_class(e["symbol"],e["module"]),
                "evidence":"kallsyms proven",
                "reconstruction_status":"inventory-only"
            },separators=(",",":"))+"\n")
print(json.dumps({"kallsyms_executable_entries":sum(len(v) for v in symbols.values()),"unique_executable_addresses":len(starts),"image_bytes":len(image)},indent=2))
