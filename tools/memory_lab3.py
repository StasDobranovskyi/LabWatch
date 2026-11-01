#!/usr/bin/env python3
"""Export linked memory, object upper estimates, GCC frames and content identity."""
from pathlib import Path
import hashlib,json,re,subprocess
r=Path(__file__).resolve().parents[1];out=r/'docs/metrics/lab3';out.mkdir(parents=True,exist_ok=True)
elf=(r/'firmware/build/firmware.elf').read_bytes();identity=(r/'firmware/Platform/build_id.h').read_text()
source=re.search(r'BUILD_ID "([a-f0-9]+)"',identity)[1]
if source.encode() not in elf:raise SystemExit('ELF/source stamp mismatch: rebuild first')
command=['docker','run','--rm','--name','labwatch-lr3-memory-export','-v',str(r)+':/workspace','-w','/workspace','labwatch-build:lab2','sh','-c','arm-none-eabi-size firmware/build/firmware.elf; arm-none-eabi-size -A firmware/build/firmware.elf; arm-none-eabi-nm --size-sort -S --radix=d firmware/build/firmware.elf; arm-none-eabi-size firmware/build/*.o']
raw=subprocess.check_output(command,text=True);(out/'memory-raw.txt').write_text(raw)
n=re.search(r'\n\s*(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+[a-f0-9]+\s+firmware/build/firmware.elf',raw)
if not n:raise SystemExit('size output unavailable')
text,data,bss=map(int,n.groups());frames=[];symbols=[]
for p in (r/'firmware/build').glob('*.su'):
 for line in p.read_text().splitlines():
  x=line.split('\t')
  if len(x)==3:frames.append({'function':x[0],'bytes':int(x[1]),'kind':x[2]})
for line in raw.splitlines():
 m=re.fullmatch(r'(\d+)\s+(\d+)\s+([bBdD])\s+(\S+)',line)
 if m:symbols.append({'name':m[4],'bytes':int(m[2]),'kind':m[3]})
log=(out/'build.log').read_text();ram=re.search(r'RAM:\s+(\d+) B',log)
chain=['main','runtime_poll','note','instrument_note','event','print_record','lw_printf','board_write_line','transmit_next','HAL_UART_Transmit_IT']
frame_by_name={x['function'].rsplit(':',1)[-1]:x['bytes'] for x in frames}
chain_frames={name:frame_by_name[name] for name in chain}
res={'source':source,'git_base':re.search(r'GIT_BASE "([^\"]+)"',identity)[1],
 'size_text':text,'size_data':data,'size_bss_including_reserves':bss,'flash_code_data':text+data,
 'flash_code_region':28672,'journal_reserved':4096,'flash_total':32768,'ram_linker_used_with_reserves':int(ram[1]),
 'ram_total':12288,'heap_reserved':512,'stack_reserved':2048,'stack_runtime':None,'heap_runtime':None,
 'ram_symbols':sorted(symbols,key=lambda x:-x['bytes'])[:20],'static_frames':sorted(frames,key=lambda x:-x['bytes']),
 'static_chain_example':{'functions':['main','runtime_poll','note','instrument_note','event','print_record','lw_printf','board_write_line','transmit_next','HAL_UART_Transmit_IT'],
 'sum_frame_bytes_without_irq':sum(chain_frames.values()),'individual_frame_bytes':chain_frames,'status':'source-level sum of GCC frames; not a global bound. Optimized paths, library frames and IRQ nesting require runtime comparison.'},
 'elf_sha256':hashlib.sha256(elf).hexdigest(),'export_command':command}
(out/'memory.json').write_text(json.dumps(res,ensure_ascii=False,indent=2))
print(json.dumps({k:res[k] for k in ['source','flash_code_data','ram_linker_used_with_reserves','elf_sha256']},indent=2))
