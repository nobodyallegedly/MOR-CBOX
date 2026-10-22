#!/usr/bin/env python3
"""Seed the fuzz corpora from the published vectors (tooling only: it reads
vectors/ and writes fuzz/corpus/<target>/). Run from the repository root."""
import hashlib
import json
import os
import sys

V = 'vectors'
OUT = 'fuzz/corpus'


def put(target, data):
    d = os.path.join(OUT, target)
    os.makedirs(d, exist_ok=True)
    name = hashlib.sha256(data).hexdigest()[:20]
    with open(os.path.join(d, name), 'wb') as f:
        f.write(data)


def load(name):
    with open(os.path.join(V, name)) as f:
        return json.load(f)


def main():
    cb = load('cbor.json')
    for c in cb['valid'] + cb['invalid']:
        put('cbor', bytes.fromhex(c['hex']))
    for c in load('canonical-text.json')['vectors']:
        put('text', bytes.fromhex(c['utf8']))
    vd = load('verdicts.json')
    pool = vd['pool']
    for h in pool.values():
        put('act', bytes.fromhex(h))
        put('cbor', bytes.fromhex(h))
    for layer, cases in vd['cases'].items():
        for c in cases:
            if 'acts' in c:
                blob = b''
                for a in c['acts']:
                    b = bytes.fromhex(pool[a])
                    blob += len(b).to_bytes(2, 'big') + b
                put('world', blob)
            if 'content' in c:
                put('spec', bytes.fromhex(c['content']))
                for v in c.get('held_specifications', {}).values():
                    put('spec', bytes.fromhex(v))
            if 'file' in c:
                put('spec', c['file'].encode())
    tf = load('textual-forms.json')
    for f in tf['forms'] + tf['refused']:
        put('people', f['form'].encode())
    for k in ('upper', 'bare', 'lives'):
        put('people', tf[k]['form'].encode())
    s = load('sealed.json')
    put('sealed', bytes.fromhex(s['sealed']))
    put('act', bytes.fromhex(s['inner_act']))
    for a in load('sequence-three-acts.json')['acts']:
        put('cbor', bytes.fromhex(a['inside']))
        put('cbor', bytes.fromhex(a['outside']))
    for name in ('tagged-hash.json', 'lock.json', 'running-summary.json', 'textual-forms.json', 'cbor.json'):
        with open(os.path.join(V, name), 'rb') as f:
            put('json', f.read())
    return 0


if __name__ == '__main__':
    sys.exit(main())
