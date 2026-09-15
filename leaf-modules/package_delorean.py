"""Build the removable DeLorean ZIP_STORED package from a local HV installation."""
import argparse
import struct
import re
from pathlib import Path
from zipfile import ZipFile, ZIP_STORED

def txd_chunks(data):
    kind, length, version = struct.unpack_from('<III', data)
    if kind != 22 or length + 12 > len(data):
        raise ValueError('Invalid TXD')
    chunks = []
    offset = 12
    while offset < length + 12:
        tag, size, ver = struct.unpack_from('<III', data, offset)
        end = offset + 12 + size
        if end > length + 12: raise ValueError('Invalid TXD chunk')
        chunks.append((tag, data[offset:end]))
        offset = end
    return version, chunks

def rebuild_txd(version, chunks):
    count = sum(tag == 21 for tag, _ in chunks)
    payload = b''
    for tag, raw in chunks:
        if tag == 1:
            raw = raw[:12] + struct.pack('<H', count) + raw[14:]
        payload += raw
    return struct.pack('<III', 22, len(payload), version) + payload

p = argparse.ArgumentParser()
p.add_argument('--donor', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--frost-image', type=Path)
p.add_argument('--implosion-dir', type=Path,
               help='Directory containing 21 RGBA implosion frames (defaults to the tracked release assets)')
a = p.parse_args()
repo = Path(__file__).resolve().parent.parent
a.implosion_dir = a.implosion_dir or (repo / 'leaf-modules/delorean/assets/implosion')
vehicles = a.donor / 'modloader/bttf-hv-lite/models/vehicles'
particle_cfg = a.donor / 'data/particle.cfg'
particle_txd = a.donor / 'modloader/bttf-hv-lite/models/particles/wormhole_particles.txd'
dll = repo / 'build-mingw/leaf-modules/bttf_delorean.dll'
if not dll.is_file():
    raise SystemExit('Build bttf_delorean first')
a.output.parent.mkdir(parents=True, exist_ok=True)
with ZipFile(a.output, 'w', ZIP_STORED) as z:
    z.writestr('leaf.ini', 'format=1\nid=delorean\nname=BTTF-HV Reversed DeLorean\nabi=1\narchitecture=x86_64\nentry=bttf_delorean.dll\nhost_build=e19f406-leaf-2\n')
    z.write(dll, 'bttf_delorean.dll')
    # The donor's main particle.cfg matches the stock game.  Preserve only the
    # presets referenced by the DeLorean scripts, plus the donor-only wormhole
    # images, under names owned by this removable leaf.
    used_names=set(re.findall(r'PARTICLE_LEAF_FIRST\+(PARTICLE_\w+)', (repo/'leaf-modules/delorean/Delorean.cpp').read_text()))
    used_names={n.removeprefix('PARTICLE_') for n in used_names}
    definitions = [line for line in particle_cfg.read_text(encoding='ascii').splitlines()
                   if line.strip() and not line.lstrip().startswith(';')]
    selected = [line for line in definitions if line.split()[0] in used_names]
    z.writestr('particles_additional.cfg',
               '; Donor particle presets used by the native DeLorean port\n' +
               '; Isolated Leaf presets; sourced from donor data/particle.cfg\n' +
               '\n'.join(selected) + '\n;the end\n')
    extra_version, extra_chunks = txd_chunks(particle_txd.read_bytes())
    texture_names={'cloudmasked','rainsmall','spark','smokeII_3','flame1'} | {f'smoke{i}' for i in range(1,6)} | {f'explo{i:02d}' for i in range(1,7)}
    _, donor_chunks=txd_chunks((a.donor/'models/particle.txd').read_bytes())
    extra_chunks[-1:-1]=[c for c in donor_chunks if c[0]==21 and c[1][32:64].split(b'\0')[0].decode('ascii') in texture_names]
    directory = (vehicles / 'vehicles.dir').read_bytes()
    found = set()
    with (vehicles / 'vehicles.img').open('rb') as image:
        for offset in range(0, len(directory), 32):
            sector, count, rawname = struct.unpack_from('<II24s', directory, offset)
            name = rawname.split(b'\0')[0].decode('ascii')
            if name in ('delorean.dff', 'delorean.txd'):
                image.seek(sector * 2048)
                data = image.read(count * 2048)
                if len(data) != count * 2048:
                    raise ValueError('Truncated vehicle archive')
                if name == 'delorean.txd':
                    version, chunks = txd_chunks(data)
                    moved = [c for c in chunks if c[0] == 21 and c[1][32:64].split(b'\0')[0] == b'frost']
                    if len(moved) != 1: raise ValueError('Expected exactly one frost texture')
                    if a.frost_image:
                        from PIL import Image
                        image = Image.open(a.frost_image).convert('RGBA').resize((1024,1024), Image.Resampling.LANCZOS)
                        version_native = struct.unpack_from('<I', moved[0][1],8)[0]
                        def chunk(tag, data):
                            return struct.pack('<III',tag,len(data),version_native)+data
                        levels=[]
                        while True:
                            pixels=image.tobytes('raw','BGRA')
                            levels.append(struct.pack('<I',len(pixels))+pixels)
                            if image.width==1: break
                            image=image.resize((image.width//2,image.height//2),Image.Resampling.LANCZOS)
                        header=struct.pack('<II',8,0x1106)+b'frost'.ljust(32,b'\0')+bytes(32)
                        header+=struct.pack('<IIHHBBBB',0x8500,1,1024,1024,32,len(levels),4,0)
                        moved=[(21,chunk(21,chunk(1,header+b''.join(levels))+chunk(3,b'')))]
                    extra_chunks[-1:-1] = moved
                    data = rebuild_txd(version, [c for c in chunks if not (c[0]==21 and c[1][32:64].split(b'\0')[0]==b'frost')])
                z.writestr('assets/' + name, data)
                found.add(name)
    if len(found) != 2:
        raise ValueError('Missing DeLorean model or texture')
    z.writestr('particles_additional.txd', rebuild_txd(extra_version, extra_chunks))
    col = (vehicles / 'hv_vehicles.col').read_bytes()
    length = struct.unpack_from('<I', col, 4)[0] + 8
    if col[:4] != b'COLL' or col[8:30].split(b'\0')[0] != b'delorean' or length > len(col):
        raise ValueError('Unexpected collision archive')
    z.writestr('assets/delorean.col', col[:length])
    sounds = a.donor / 'bttfhv/sound/delorean'
    for f in sorted(sounds.rglob('*.wav')):
        z.write(f, 'sounds/delorean/' + f.relative_to(sounds).as_posix())
    instant_travel = a.donor / 'bttfhv/sound/instant_timetravel.wav'
    if not instant_travel.is_file():
        raise ValueError('Missing donor instant time-travel sound')
    z.write(instant_travel, 'sounds/instant_timetravel.wav')
    for number in range(10):
        keypad = a.donor / f'bttfhv/sound/{number}.wav'
        if not keypad.is_file():
            raise ValueError(f'Missing donor keypad sound {number}')
        z.write(keypad, f'sounds/{number}.wav')
    from PIL import Image
    for frame in range(21):
        source = a.implosion_dir / f'{frame}.png'
        raw = a.implosion_dir / f'{frame}.rgba'
        if not source.is_file() and raw.is_file():
            data = raw.read_bytes()
            if len(data) != 8+512*512*4 or struct.unpack('<II',data[:8]) != (512,512):
                raise ValueError(f'Invalid preserved implosion frame {frame}')
            z.writestr(f'effects/implosion/{frame}.rgba',data)
            continue
        if not source.is_file():
            raise ValueError(f'Missing implosion frame {frame}')
        image = Image.open(source).convert('RGBA').resize((512,512), Image.Resampling.LANCZOS)
        z.writestr(f'effects/implosion/{frame}.rgba', struct.pack('<II',512,512) + image.tobytes())
    fire_path = repo / 'leaf-modules/delorean/assets/fire-burst.rgba'
    fire = fire_path.read_bytes()
    if len(fire) != 8+2048*4096*4 or struct.unpack('<II', fire[:8]) != (2048,4096):
        raise ValueError('Invalid simulated fire particle atlas')
    z.writestr('effects/fire-burst.rgba',fire)
    z.write(repo / 'leaf-modules/delorean/assets/simulate_fire.py','source/simulate_fire.py')
    for f in sorted((repo / 'leaf-modules/delorean').glob('*')):
        if f.is_file():
            z.write(f, 'source/' + f.name)
    z.writestr('README.txt', 'Native preview, not complete HV parity. Replaces Deluxo in memory only.\n'
        'Cabin/refuel batch: donor oil, temperature, fuel and voltage gauges; warning lamps; glow; sequential reactor animation; concurrent OpenAL sounds.\n'
        'Cabin controls: donor wipers, turn signals/hazards, moving windows, pedals/handbrake, reactor gauges and engine/hover audio transitions.\n'
        'Gear lever selects neutral/reverse/1-5; RPM needle follows donor wheel-speed gear ratios and hover-speed calculation.\n'
        'Hover underbody lights fade by 15 alpha steps at the donor 30fps baseline; five chaser groups advance every 250ms.\n'
        'Cabin compass counters vehicle heading; mapped horn moves the stalk. P toggles overhead emergency light with donor sound and handle animation.\n'
        'Headlights now have feathered depth-tested beam volumes; P also controls a warm cabin beam and point illumination. Requires the updated host pre-render callback. Visual tuning awaits in-game testing.\n'
        'Console clock: starts from last departure and continues counting elapsed minutes across time travel; blank leading zero, blinking colon and matching analog hands.\n'
        'Compass uses a 30fps damped ball response with counter-pitch/roll while the vehicle banks.\n'
        'Native seated leg adjustment places the driver feet at pedals; right foot follows gas/brake selection. Requires the matching updated host executable. Visual alignment awaits user testing.\n'
        'U: wipers normal/fast/off; also parks intermittent. I: reverse stalk/single sweep. J/K: left/right window. Left Shift+L: left signal. Right Shift+L: right signal. L alone: hazards. Repeat combination to turn off.\n'
        'Animations use the donor 30 fps baseline at all render rates. Refueling locks movement until 500 ms after closing, then grants fuel 500 ms later.\n'
        'The reactor still has unlimited plutonium in this preview; donor inventory/pickups and engine-turnover branches remain to be ported.\n'
        'Remove this archive while the game is closed to disable it next launch.\n'
        'Period: donor debug variation/spawn. C: hover conversion.\n'
        'Enter MMDDYYYYHHMM then -: destination. Tab at the rear: refuel.\n'
        'Travel parity batch: donor coil-threshold plasma, sparks loop, persistent twin tire fire trails, OUTATIME plate tumble/fall, occupied-car time-travel mix, and hover acceleration/deceleration/thruster cues.\n'
        'Cockpit/effects batch: car-locked plasma, interior window frost, world-depth portal using the donor dome mesh and original UVs, and bonnet/rear-cover synchronization. Travel uses the displayed 88 MPH threshold without waiting for the final portal frame.\n'
        'Time-travel mode defaults to the donor cinematic sequence: fixed exterior camera, visible departure burst, disappearance, delayed fade, re-entry and camera restoration. M toggles cinematic/instant mode.\n'
        'Implosion: supplied 21 frames with GPU interpolation at 90 ms per source frame, original opacity and subdued point light. Re-entry fades in before blue bursts and reveals the car already moving at 88 100 ms inside the third burst.\n'
        'Display parity: units-first digits, half-second colon blink and donor beep once per second when powered, and three animated donor shutters. Updated host improves first-person depth precision and resolves multisampled scene colour for water droplets.\n'
        'Keypad digits use the donor plugin mapping: keys 0-9 play matching 0.wav-9.wav tones. Interior frost recognizes custom and normal in-car cameras, including door glass.\n'
        'Dome/window/hook batch: moving door glass receives frost directly; cold smoke, hover particles and reactor effects wait until cinematic reveal. Side hooks settle vertically; rear hooks fall backward. Mesh-ground clearance prevents generic panel collision impulses; both leave holder-only mode.\n'
        'Arrival/details: travel audio and hidden meshes preload before arrival; completed audio voices recycle. Implosion sound is world-attached at 30 m range. Gauges light first, BTTF1 startup audio plays at illumination, then needles move. Instrument layers receive a small depth bias. Matching host places DeLorean damage smoke/fire in the rear engine bay.\n'
        'Sources: https://github.com/bttfhillvalley/vc-cleo-plugins and supplied HV scripts.\n')
with ZipFile(a.output) as z:
    if z.testzip():
        raise ValueError('Archive CRC failed')
print(a.output)
