"""Content identity for a dirty working tree; excludes outputs and this generated header."""
from pathlib import Path
import hashlib, subprocess
r=Path(__file__).resolve().parents[1]
paths=sorted([p for p in (r/'firmware').rglob('*') if p.is_file() and p.suffix in {'.c','.h','.s','.ld'} and 'build' not in p.parts and p.name!='build_id.h']+[r/'firmware/Makefile',r/'diagram.json'])
h=hashlib.sha256()
for p in paths:
 h.update(str(p.relative_to(r)).encode()+b'\0'+p.read_bytes()+b'\0')
base=subprocess.check_output(['git','-C',str(r),'rev-parse','--short=12','HEAD'],text=True).strip()
text=f'#define GIT_BASE "{base}"\n#define BUILD_ID "{h.hexdigest()}"\n'
p=r/'firmware/Platform/build_id.h'
if not p.exists() or p.read_text()!=text:p.write_text(text)
print(text.strip())
