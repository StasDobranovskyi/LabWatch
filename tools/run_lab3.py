#!/usr/bin/env python3
"""Collect only live serial output. Host scheduling time is never an MCU metric.
S1/S2 duration is checked against HEALTH time_ms, not USB arrival timestamps.
"""
import argparse, hashlib, json, re, time
from pathlib import Path
import serial
p=argparse.ArgumentParser()
p.add_argument('--scenario',choices=['S1','S2','S3','fault','flood','boundary'],required=True)
p.add_argument('--port',default='rfc2217://localhost:4537')
p.add_argument('--seconds',type=int)
p.add_argument('--simulator-version',default='not-recorded')
p.add_argument('--rate',type=float,default=20)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--elf',type=Path,default=Path('firmware/build/firmware.elf'))
p.add_argument('--wall-timeout',type=int,default=14400)
a=p.parse_args()
if a.rate<=0 or a.rate>2000:p.error('rate must be in (0,2000]')
if a.output.exists():p.error('refusing to overwrite an existing run directory')
a.output.mkdir(parents=True)
r=Path(__file__).resolve().parents[1]
elf_path=a.elf if a.elf.is_absolute() else r/a.elf
identity=(r/'firmware/Platform/build_id.h').read_text()
duration=a.seconds or (3600 if a.scenario=='S2' else 120)
if a.scenario=='S2' and duration<3600:p.error('S2 requires at least 3600 MCU seconds')
log=a.output/'uart.log';commands=[];started=time.monotonic();last_ms=None;elapsed=0;pending=b'';begun=None;begin_ms=None;observed_source=None
expected_source=re.search(r'BUILD_ID "([a-f0-9]+)"',identity)[1]
manifest={'scenario':a.scenario,'port':a.port,'required_mcu_seconds':duration,'rate':a.rate,
 'identity':identity,'firmware_sha256':hashlib.sha256(elf_path.read_bytes()).hexdigest(),
 'time_basis':'firmware HEALTH time_ms; host timestamps only schedule commands',
 'external_stimuli':'PIR, MPU6050 and LDR changes must be entered in observations.jsonl with simulator times; script does not fabricate them',
 'collector_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
 'simulator_version':a.simulator_version,
 'status':'running','started_utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())}
ser=None
def send(text):
 ser.write((text+'\n').encode());commands.append({'host_elapsed_s':time.monotonic()-started,'text':text})
def collect(raw):
 global pending,last_ms,elapsed,begun,begin_ms,observed_source
 pending+=raw
 while b'\n' in pending:
  line,pending=pending.split(b'\n',1)
  version=re.search(rb'(?:VERSION|BOOT) .*source=([a-f0-9]{64})',line)
  if version:
   observed_source=version[1].decode()
   if observed_source!=expected_source:raise RuntimeError('live firmware source identity does not match this build')
  m=re.search(rb'HEALTH time_ms=(\d+)',line)
  if m:
   now=int(m[1])
   if last_ms is not None:
    delta=(now-last_ms)&0xffffffff
    if delta>60000:raise RuntimeError('MCU time discontinuity/reset or >60s health gap; do not merge reset runs')
    if begun is not None:
     if begin_ms is None:begin_ms=now
     elapsed=(now-begin_ms)&0xffffffff
   last_ms=now
  if a.scenario=='S2' and b'event=calibration_ready' in line and begun is None:
   calibration=re.search(rb'time_ms=(\d+)',line)
   if calibration:begin_ms=int(calibration[1]);begun=time.monotonic()
try:
 ser=serial.serial_for_url(a.port,baudrate=115200,timeout=0.1)
 with log.open('wb') as f:
  send('version')
  send('mode work' if a.scenario=='S2' else 'mode idle')
  # Establish sensor readiness before requesting the 10-second reference window.
  prepare=time.monotonic()+2;next_command=float('inf');fault_phase=0;next_stats=30
  while True:
   raw=ser.read(4096)
   if raw:f.write(raw);f.flush();collect(raw)
   wall=time.monotonic()
   if wall-started>a.wall_timeout:raise TimeoutError('wall timeout before required MCU duration')
   if wall>=prepare and next_command==float('inf'):
    if observed_source is None:raise RuntimeError('live firmware did not return VERSION; cannot bind raw evidence to ELF')
    send('calibrate');next_command=wall+(1/a.rate)
    if a.scenario!='S2':begun=wall
   if begun is None:continue
   seconds=elapsed/1000
   if seconds>=next_stats:send('stats');next_stats+=30
   if a.scenario in ['S3','flood','boundary'] and wall>=next_command:
    send('mark' if a.scenario=='S3' else 'status');next_command=wall+1/a.rate
   if a.scenario=='fault':
    if fault_phase==0 and seconds>=15:send('imuaddr 0x69');fault_phase=1
    if fault_phase==1 and seconds>=25:send('imuaddr 0x68');fault_phase=2
   if a.scenario=='S2' and wall>=next_command:
    send('status');send('mark');next_command=wall+60
   if elapsed>=duration*1000:break
  send('stats');send('journal dump');until=time.monotonic()+5
  while time.monotonic()<until:
   raw=ser.read(4096)
   if raw:f.write(raw);collect(raw)
 manifest.update(status='collected',mcu_elapsed_ms=elapsed,wall_elapsed_s=time.monotonic()-started)
except BaseException as e:
 manifest.update(status='incomplete',error=str(e),mcu_elapsed_ms=elapsed,wall_elapsed_s=time.monotonic()-started)
 raise
finally:
 if ser is not None:ser.close()
 (a.output/'commands.json').write_text(json.dumps(commands,ensure_ascii=False,indent=2))
 manifest['observed_source']=observed_source
 manifest['uart_sha256']=hashlib.sha256(log.read_bytes()).hexdigest() if log.exists() else None
 (a.output/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2))
