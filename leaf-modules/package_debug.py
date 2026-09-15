import argparse
from pathlib import Path
from zipfile import ZipFile,ZIP_STORED
from make_debug_scm import build
p=argparse.ArgumentParser()
p.add_argument('--output',type=Path,required=True)
p.add_argument('--full-debug',action='store_true',help='Include the development-only F9 fire preview and body-opacity controls')
a=p.parse_args()
repo=Path(__file__).resolve().parent.parent;dll=repo/'build-mingw/leaf-modules/leaf_debug.dll'
if not a.full_debug:
    dll=repo/'build-mingw/leaf-modules/leaf_debug_release.dll'
if not dll.is_file():raise SystemExit('Build leaf_debug_release (or leaf_debug with --full-debug) first')
a.output.parent.mkdir(parents=True,exist_ok=True)
with ZipFile(a.output,'w',ZIP_STORED) as z:
 z.writestr('leaf.ini','format=1\nid=debug\nname=BTTF-HV Reversed Release Debug Camera\nabi=1\narchitecture=x86_64\nentry=leaf_debug.dll\nhost_build=e19f406-leaf-2\n')
 z.write(dll,'leaf_debug.dll')
 if a.full_debug:
  z.writestr('data/main.scm',build(repo))
  readme='Development debug archive. F10 toggles first person; F9 toggles the persistent fire-trail preview; numpad +/- changes first-person body opacity. The archive also supplies a minimal freeroam SCM for quick testing.\n'
 else:
  readme='Release debug archive. F10 toggles first person on foot and in vehicles; mouse look remains active and the normal freecam setting is preserved. The local player head is hidden while looking through the first-person camera. This archive does not replace or disable the campaign SCM.\n'
 z.writestr('README.txt',readme)
with ZipFile(a.output) as z:
 if z.testzip():raise ValueError('Archive CRC failed')
print(a.output)
