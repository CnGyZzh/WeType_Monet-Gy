#!/usr/bin/env python3
from pathlib import Path
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED
root=Path(__file__).resolve().parent
apk=root/'ksu/gyhook.apk'
if not apk.is_file():
    raise SystemExit('ERROR: build app APK first and copy it to ksu/gyhook.apk')
out=root/'out/GyGlass_Standalone_v29.zip'
out.parent.mkdir(exist_ok=True)
with ZipFile(out,'w',ZIP_DEFLATED,compresslevel=8) as z:
    for p in sorted((root/'ksu').rglob('*')):
        if not p.is_file(): continue
        name=p.relative_to(root/'ksu').as_posix()
        zi=ZipInfo(name)
        zi.compress_type=ZIP_DEFLATED
        zi.external_attr=((0o100755 if name.endswith('.sh') else 0o100644)<<16)
        z.writestr(zi,p.read_bytes())
with ZipFile(out) as z: assert z.testzip() is None
print(out)
