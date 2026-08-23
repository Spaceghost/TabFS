#!/usr/bin/env python3
from pathlib import Path
import sys

EXPECTED = {
    "f3_abi_version": 0,
    "f3_add_u32": 0,
    "f3_postscript_size": 0,
    "f3_parse_postscript": 0,
    "memory": 2,
}

def uleb(data, off):
    value = shift = 0
    while True:
        if off >= len(data): raise ValueError("truncated uleb")
        b = data[off]; off += 1
        value |= (b & 0x7f) << shift
        if not b & 0x80: return value, off
        shift += 7
        if shift > 35: raise ValueError("oversized uleb")

def name(data, off):
    n, off = uleb(data, off); end = off + n
    if end > len(data): raise ValueError("truncated name")
    return data[off:end].decode(), end

def main(path):
    data = Path(path).read_bytes()
    if data[:8] != b"\0asm\x01\0\0\0": raise ValueError("not wasm1")
    imports = 0; exports = {}; off = 8
    while off < len(data):
        sid = data[off]; off += 1
        size, off = uleb(data, off); end = off + size
        payload = data[off:end]
        if end > len(data): raise ValueError("truncated section")
        if sid == 2: imports, _ = uleb(payload, 0)
        if sid == 7:
            count, cur = uleb(payload, 0)
            for _ in range(count):
                n, cur = name(payload, cur); kind = payload[cur]; cur += 1
                _, cur = uleb(payload, cur); exports[n] = kind
        off = end
    if imports: raise ValueError(f"imports={imports}")
    if exports != EXPECTED: raise ValueError(f"exports={exports!r}")

if __name__ == "__main__": main(sys.argv[1])
