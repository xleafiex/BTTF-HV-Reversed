import argparse
from pathlib import Path
from zipfile import ZipFile,ZIP_STORED
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
repo=Path(__file__).resolve().parent.parent;dll=repo/'build-mingw/leaf-modules/leaf_ragdoll.dll'
if not dll.is_file():raise SystemExit('Build leaf_ragdoll first')
a.output.parent.mkdir(parents=True,exist_ok=True)
with ZipFile(a.output,'w',ZIP_STORED) as z:
    z.writestr('leaf.ini','format=1\nid=ragdoll\nname=Bullet Ragdoll Prototype\nabi=1\narchitecture=x86_64\nentry=leaf_ragdoll.dll\nhost_build=e19f406-leaf-2\n')
    z.write(dll,'leaf_ragdoll.dll')
    # UCRT64's Bullet build links LinearMath as a separate DLL.
    for name in ('libBulletDynamics.dll','libBulletCollision.dll','libLinearMath.dll'):
        dep=Path(r'C:/msys64/ucrt64/bin')/name
        if not dep.is_file(): raise SystemExit('Missing '+name)
        z.write(dep,name)
    z.writestr('README.txt','Optional Bullet whole-body ragdoll prototype. F6 toggles the player and nearby peds; a temporary Bullet floor keeps bodies near the current map surface. Remove ragdoll.leaf to disable.\n')
with ZipFile(a.output) as z:
    if z.testzip():raise ValueError('Archive CRC failed')
print(a.output)
