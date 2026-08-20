#!/usr/bin/env python3
"""Recheck conservative static ADRP+ADD+LDR BLR chains in the recovered X683 Image.

This tool intentionally reports only simple static chains. Runtime-initialized
ops tables and private-state callbacks remain unresolved.
"""
from __future__ import annotations
import bisect, hashlib, json, re, struct
from pathlib import Path

def parse_symbols(path: Path):
    out=[]
    for line in path.read_text(errors="replace").splitlines():
        m=re.match(r"([0-9a-fA-F]+)\s+([A-Za-z])\s+(\S+)", line)
        if m: out.append((int(m.group(1),16),m.group(2),m.group(3)))
    return out

def sx(x,b): return x-(1<<b) if x&(1<<(b-1)) else x

def adrp(ins, pc):
    rd=ins&31; immlo=(ins>>29)&3; immhi=(ins>>5)&0x7ffff
    imm=sx((immhi<<2)|immlo,21)
    return rd,(pc&~0xfff)+(imm<<12)

def addi(ins):
    if (ins&0x7f000000)!=0x11000000: return None
    rd=ins&31; rn=(ins>>5)&31
    imm=((ins>>10)&0xfff)<<(12 if (ins>>22)&1 else 0)
    return rd,rn,-imm if (ins>>30)&1 else imm

def ldr(ins):
    mask=ins&0xffc00000
    if mask==0xf9400000: return ins&31,(ins>>5)&31,((ins>>10)&0xfff)*8
    if mask==0xb9400000: return ins&31,(ins>>5)&31,((ins>>10)&0xfff)*4
    return None

def main():
    import argparse
    ap=argparse.ArgumentParser(); ap.add_argument("image"); ap.add_argument("kallsyms"); ap.add_argument("-o",default="x683-blr-static-resolver.json"); a=ap.parse_args()
    image=Path(a.image).read_bytes(); sy=parse_symbols(Path(a.kallsyms))
    st=next(v for v,t,n in sy if n=="_stext"); base=st-0x80000; limit=base+len(image)
    funcs=sorted({v for v,t,n in sy if t in "TtWwVv" and base<=v<limit}); starts=set(funcs)
    static=exact=0; sites=[]
    for i,f in enumerate(funcs):
        off=f-base; end=min((funcs[i+1] if i+1<len(funcs) else limit)-base,len(image))
        for pos in range(off,end-3,4):
            ins=struct.unpack_from("<I",image,pos)[0]
            if ins&0xfffffc1f != 0xd63f0000: continue
            rn=(ins>>5)&31
            start=max(off,pos-20); words=[struct.unpack_from("<I",image,q)[0] for q in range(start,pos,4)]
            for j,x in enumerate(words):
                if x&0x9f000000 != 0x90000000: continue
                rd,addr=adrp(x,base+start+j*4)
                if j+2>=len(words): continue
                aa=addi(words[j+1]); ll=ldr(words[j+2])
                if not aa or not ll or aa[0]!=rd or aa[1]!=rd or ll[0]!=rn or ll[1]!=rd: continue
                static+=1; slot=addr+aa[2]+ll[2]
                target=None
                if base<=slot<=limit-8:
                    target=struct.unpack_from("<Q",image,slot-base)[0]
                    if target in starts: exact+=1
                sites.append({"blr":hex(base+pos),"slot":hex(slot),"target":hex(target) if target is not None else None,"exact":target in starts if target is not None else False})
                break
    result={"image_sha256":hashlib.sha256(image).hexdigest(),"blr_static_simple_chains":static,"exact_function_targets":exact,"sites":sites}
    Path(a.o).write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps({k:v for k,v in result.items() if k!="sites"},indent=2))

if __name__=="__main__": main()
