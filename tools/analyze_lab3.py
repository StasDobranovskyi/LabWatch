#!/usr/bin/env python3
"""Read MCU statistics; do not derive latency from host arrival time."""
import argparse,json,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('log',type=Path);a=p.parse_args()
t=a.log.read_text(errors='replace');stats={};health=[]
for line in t.splitlines():
 pairs=dict(re.findall(r'(\w+)=([^\s]+)',line))
 if line.startswith('STAT '):stats[pairs['name']]={k:int(v) for k,v in pairs.items() if k!='name'}
 if line.startswith('HEALTH '):health.append(pairs)
identity=sorted(set(re.findall(r'(?:BOOT|VERSION) .*source=([a-f0-9]+)',t)))
invalid=[name for name,s in stats.items() if s.get('max_us',0)>=0x80000000]
print(json.dumps({'identity':identity,'timing_validity':'invalid_unsigned_duration' if invalid else 'not_independently_validated','invalid_duration_tasks':invalid,'stats':stats,'last_health':health[-1] if health else None,
 'integrity_note':'JREC output is firmware CRC-filtered. A raw flash dump is still required to independently verify stored bytes.',
 'warning':'Use BOOT or live VERSION identity; validate against manifest. Near-2^32 intervals are invalid counter differences, not physical delays. Host-model logs are not MCU measurements.'},indent=2))
