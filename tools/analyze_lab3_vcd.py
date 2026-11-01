#!/usr/bin/env python3
"""VCD GPIO widths and full input-to-marker latency, timestamps converted to us."""
import argparse,json,re,statistics,hashlib
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('vcd',type=Path);a=p.parse_args()
s=a.vcd.read_text(errors='strict');m=re.search(r'\$timescale\s+(\d+)\s*(s|ms|us|ns|ps)\s*\$end',s)
if not m:raise SystemExit('missing timescale')
scale=int(m[1])*{'s':1e6,'ms':1e3,'us':1,'ns':1e-3,'ps':1e-6}[m[2]]
ids={symbol:name for symbol,name in re.findall(r'\$var\s+\w+\s+1\s+(\S+)\s+(\S+)\s+\$end',s)}
# The configured channel names or D0...D7 are both accepted. Inspect header before trusting mapping.
names=['PIR','IMU_MARK','FLASH_MARK','REACT_MARK','ADC_MARK','UART_TX','UART_RX','LED']
def channel(name):
 if name in names:return name
 if re.fullmatch('D[0-7]',name):return names[int(name[1:])]
 return name
edges={channel(n):[] for n in ids.values()};now=0;state={}
for line in s.split('$enddefinitions',1)[-1].splitlines():
 if line.startswith('#'):now=int(line[1:])*scale
 elif len(line)>1 and line[0] in '01' and line[1:] in ids:
  symbol=line[1:];v=int(line[0]);old=state.get(symbol);state[symbol]=v
  if old is not None and old!=v:edges[channel(ids[symbol])].append((now,v))
def summary(values):
 return dict(n=len(values),min_us=min(values),avg_us=statistics.mean(values),max_us=max(values)) if values else {'n':0}
result={}
for name in ['IMU_MARK','FLASH_MARK','ADC_MARK']:
 rise=None;previous=None;width=[];period=[]
 for time,v in edges.get(name,[]):
  if v:
   rise=time
   if previous is not None:period.append(time-previous)
   previous=time
  elif rise is not None:width.append(time-rise);rise=None
 result[name]={'width':summary(width),'period':summary(period)}
latencies=[];reactions=[t for t,v in edges.get('REACT_MARK',[]) if v];i=0
for t,v in edges.get('PIR',[]):
 if not v:continue
 while i<len(reactions) and reactions[i]<t:i+=1
 if i<len(reactions):latencies.append(reactions[i]-t);i+=1
result['PIR_to_REACT']=summary(latencies)
print(json.dumps({'sha256':hashlib.sha256(a.vcd.read_bytes()).hexdigest(),'channels':ids,'metrics':result,
 'pairing':'Greedy rising input to next reaction rising edge; validate one input per reaction and annotate queue-loss episodes separately.'},indent=2))
