"""Package the native GL3 graphics implementation, without replacing game files."""
import argparse
from pathlib import Path
from zipfile import ZipFile, ZIP_STORED
p=argparse.ArgumentParser()
p.add_argument('--output',type=Path,required=True)
p.add_argument('--donor',type=Path,required=True)
a=p.parse_args()
repo=Path(__file__).resolve().parent.parent
module=repo/'build-mingw/leaf-modules/leaf_graphics.dll'
if not module.is_file(): raise SystemExit('Build leaf_graphics first')
a.output.parent.mkdir(parents=True,exist_ok=True)
with ZipFile(a.output,'w',ZIP_STORED) as z:
    z.writestr('leaf.ini','format=1\nid=graphics\nname=Native GL3 Graphics Preview\nabi=1\narchitecture=x86_64\nentry=leaf_graphics.dll\nhost_build=e19f406-leaf-2\n')
    z.write(module,'leaf_graphics.dll')
    z.write(a.donor/'models/particle.txd','assets/particle_original.txd')
    for f in sorted((repo/'leaf-modules/graphics').iterdir()):
        if f.is_file(): z.write(f,('shaders/' if f.suffix in ('.vert','.frag') else 'source/')+f.name)
    z.writestr('README.txt','Native graphics preview: world-aligned water swells and lighting, sky/cloud integration, and weather selection.\n'
        'Ctrl+K toggles graphics; Ctrl+W cycles stock weather types and automatic weather.\n'
        'Scene refraction, shore foam, shadows, matcap and advanced particles are not yet ported.\n'
        'Anti-aliasing stays with reVC. Remove this archive while the game is closed to disable it.\n'
        'Swell formula adapted from the supplied VCWater shader (Einheit-101 OSWS adaptation).\n')
with ZipFile(a.output) as z:
    if z.testzip(): raise ValueError('Archive CRC failed')
print(a.output)
