#!/usr/bin/env python3
"""An annotated, explicitly single-event trace from this product's real pilot."""
from pathlib import Path
import json,hashlib
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from analyze_vcd import read_vcd,uart,pulses,at,clip
source=Path(__file__).resolve().parents[1]/'docs/captures/lab3/pilot-d9c97958/raw.vcd'
traces,end=read_vcd(source);rx=uart(traces['UART_RX'],end_ns=end);tx=uart(traces['UART_TX'],end_ns=end)
mark=next(x for x in rx if x['text']=='mark');end_rx=mark['time_ns']+5*10e9/115200
flash=next(p for p in pulses(traces['FLASH_MARK']) if p['start_ns']>=end_rx)
start=(mark['time_ns']//1000000)*1000000;stop=start+24000000
result={'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'source':'d9c97958','n':1,
 'metric':'last UART RX stop-bit end to next FLASH_MARK rising edge','rx_end_ns':end_rx,
 'flash_start_ns':flash['start_ns'],'delay_us':(flash['start_ns']-end_rx)/1000,'RX':rx,'TX':tx,
 'limits':'Single diagnostic event; not a30-event S3 sample. FLASH_MARK is the software journal call, not physical erase busy.'}
(source.parent/'uart-decoded.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
(source.parent/'uart-wire.txt').write_text('\n'.join(f"{r['time_ns']/1e6:.6f} ms {r['text']}" for r in tx)+'\n')
clip(traces,source,source.parent/'mark-window.vcd',int(start),int(stop))
fig,axes=plt.subplots(4,1,figsize=(9,4.6),sharex=True,layout='constrained')
for ax,name in zip(axes,['UART_RX','FLASH_MARK','IMU_MARK','ADC_MARK']):
 trace=traces[name];xs=[start];ys=[at(trace,start)]
 for t,v in zip(*trace):
  if start<t<=stop:xs.append(t);ys.append(v)
 xs.append(stop);ys.append(ys[-1]);ax.step([(t-start)/1e6 for t in xs],ys,where='post',color='#183e65',linewidth=1.2)
 ax.set_ylim(-.25,1.4);ax.set_yticks([0,1]);ax.set_ylabel(name,rotation=0,labelpad=52,fontsize=12);ax.grid(axis='x',alpha=.25)
 for t in [end_rx,flash['start_ns']]:ax.axvline((t-start)/1e6,color='#ae4030',linestyle='--',linewidth=1)
axes[1].annotate(f"RX → FLASH: {result['delay_us']:.1f} µs, n=1",xy=((flash['start_ns']-start)/1e6,1.1),xytext=(6.4,1.18),fontsize=12,
 arrowprops={'arrowstyle':'->','color':'#ae4030'})
axes[-1].set_xlabel(f'Час VCD від {start/1e9:.3f} s, ms',fontsize=12)
fig.suptitle('LabWatch d9c97958: фактичний діагностичний захват Wokwi',fontsize=13)
fig.savefig(source.parent/'mark-annotated.png',dpi=200);plt.close(fig)
print(json.dumps({k:result[k] for k in ['n','metric','delay_us','limits']},ensure_ascii=False,indent=2))
