#!/usr/bin/env python3
"""One bounded LabWatch d7 Wokwi capture. UART/VCD remain the evidence, not host schedule."""
import argparse, hashlib, json, re, sys, time
from pathlib import Path
import serial
SOURCE='d7c3f569f1d1dfb638999a4578bc940a164f577837d2776d9796671d594cf9fa'
ELF='fbb10f8e971e68c7f920f759c0a21c769ec9440fff2cd9f60147864f338789f2'
ROOT=Path(__file__).resolve().parents[1]

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--go-file',type=Path,required=True);p.add_argument('--port',default='rfc2217://localhost:4537');p.add_argument('--wall-timeout',type=int,default=300);a=p.parse_args()
 if not 90<=a.wall_timeout<=600:p.error('wall timeout must be 90–600 s')
 if a.output.exists() or a.go_file.exists():p.error('output and GO must not already exist')
 if hashlib.sha256((ROOT/'firmware/build-rx-final/firmware.elf').read_bytes()).hexdigest()!=ELF:p.error('ELF mismatch')
 a.output.mkdir(parents=True); start=time.monotonic(); last_health_wall=start;first_ms=last_ms=None; pending=b'';phase='gate';version=False;diag=None;rxhealth=None;cal=False;port=None;attempts=0;sent=0;result={'status':'incomplete','source':SOURCE,'elf_sha256':ELF,'collector_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'interpretation':'imuaddr modifies software I2C address, not SDA grounding; command schedule uses host clock; analyze VCD for actual wire time and simulator interval'}
 try:
  port=serial.serial_for_url(a.port,baudrate=115200,timeout=.02)
  with (a.output/'uart.log').open('wb') as log,(a.output/'events.jsonl').open('w') as events:
   def event(kind,**kw):
    events.write(json.dumps({'kind':kind,'host_elapsed_s':round(time.monotonic()-start,6),'host_unix_s':time.time(),'last_health_ms':last_ms,**kw})+'\n');events.flush()
   def send(payload,label):
    nonlocal sent
    if sent+len(payload)>5000:raise RuntimeError('transmit cap exceeded')
    before=time.monotonic();n=port.write(payload);sent+=n
    event('write',label=label,hex=payload.hex(),expected=len(payload),written=n,duration_s=time.monotonic()-before)
    if n!=len(payload):raise RuntimeError('short write')
   def command(s):send((s+'\n').encode('ascii'),s)
   command('version');command('rxdiag')
   while True:
    now=time.monotonic()
    if now-start>a.wall_timeout:raise TimeoutError('wall timeout')
    if now-last_health_wall>30:raise TimeoutError('HEALTH absent for 30 host seconds')
    raw=port.read(4096)
    if raw:
     log.write(raw);log.flush();pending+=raw
     if len(pending)>65536:raise RuntimeError('unterminated UART data')
     while b'\n' in pending:
      line,pending=pending.split(b'\n',1);line=line.rstrip(b'\r');text=line.decode('ascii','replace')
      if line.startswith(b'BOOT ') and first_ms is not None:raise RuntimeError('MCU reset')
      m=re.match(rb'VERSION source=([0-9a-f]{64}) ',line)
      if m:
       if m[1].decode()!=SOURCE:raise RuntimeError('live source mismatch')
       version=True
      if line.startswith(b'RXDIAG '):diag=text
      if line.startswith(b'RXHEALTH '):rxhealth=text
      h=re.match(rb'HEALTH time_ms=(\d+) ',line)
      if h:
       ms=int(h[1]);last_health_wall=time.monotonic()
       if last_ms is not None and ((ms-last_ms)&0xffffffff)>2000:raise RuntimeError('MCU HEALTH gap/reset')
       last_ms=ms
       if first_ms is None:first_ms=ms;first_wall=time.monotonic()
       event('health',mcu_ms=ms,text=text)
      if line.startswith(b'EVT ') and b'event=calibration_ready ' in line and phase=='calibrating':cal=True;event('calibration_ready',text=text)
      if line.startswith(b'EVT ') and (b'event=imu_error ' in line or b'event=imu_recovered ' in line or b'event=calibration_required ' in line):event('imu_event',text=text)
    now=time.monotonic()
    if phase=='gate' and first_ms is not None and ((last_ms-first_ms)&0xffffffff)>=15000:
     if not version or not diag or not rxhealth:raise RuntimeError('gate incomplete')
     gate={'status':'awaiting_go','source':SOURCE,'mcu_first_ms':first_ms,'mcu_last_ms':last_ms,'host_elapsed_s':now-first_wall,'rxdiag':diag,'rxhealth':rxhealth,'note':'review rates, counters, fixture and personal GUI admission before GO; this gate is not a VCD validation'}
     (a.output/'gate.json').write_text(json.dumps(gate,indent=2)+'\n');phase='await_go';event('gate')
    if phase=='await_go' and a.go_file.exists():
     if a.go_file.read_text().strip()!=SOURCE:raise RuntimeError('invalid GO')
     command('mode work');command('calibrate');phase='calibrating';phase_time=now;event('phase',name=phase)
    elif phase=='calibrating' and cal:
     command('stats');command('imuaddr 0x69');phase='wrong_address';phase_time=now;event('phase',name=phase)
    elif phase=='calibrating' and now-phase_time>35:raise TimeoutError('calibration not observed')
    elif phase=='wrong_address' and now-phase_time>=8:
     command('imuaddr 0x68');phase='recovery';phase_time=now;event('phase',name=phase)
    elif phase=='recovery' and now-phase_time>=8:
     command('rxdiag');command('stats');command('journal dump');phase='load_2hz';phase_time=now;next_send=now;next_dump=now+3;event('phase',name=phase,nominal_rate_hz=2)
    elif phase.startswith('load_'):
     rate={'load_2hz':2,'load_10hz':10,'load_40hz':40}[phase]
     if now>=next_send:command('status');attempts+=1;next_send=now+1/rate
     if now>=next_dump:command('journal dump');next_dump=now+3
     if now-phase_time>=8:
      if rate==2:phase='load_10hz'
      elif rate==10:phase='load_40hz'
      else:
       command('journal dump');send(b'status\n'*100,'bounded_no_pause_burst_100');attempts+=100;phase='drain'
      phase_time=now;next_send=now;event('phase',name=phase)
    elif phase=='drain' and now-phase_time>=12:
     command('rxdiag');command('stats');command('storage');phase='final';phase_time=now;event('phase',name=phase)
    elif phase=='final' and now-phase_time>=10:
     result.update(status='collected',last_health_ms=last_ms,attempted_status=attempts,bytes_written=sent);break
 except BaseException as e:result.update(error=repr(e),phase=phase,last_health_ms=last_ms,attempted_status=attempts,bytes_written=sent)
 finally:
  if port is not None:port.close()
  log=a.output/'uart.log';result['uart_sha256']=hashlib.sha256(log.read_bytes()).hexdigest() if log.exists() else None;result['host_elapsed_s']=time.monotonic()-start
  (a.output/'manifest.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print(json.dumps(result),flush=True)
 return 0 if result['status']=='collected' else 1
if __name__=='__main__':sys.exit(main())
