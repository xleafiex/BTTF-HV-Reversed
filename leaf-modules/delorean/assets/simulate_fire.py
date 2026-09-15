"""Bake a buoyant, advected flame burst for the native DeLorean particle renderer.

This produces simulation data, not a retouched photograph. Fixed seed, pressure
projection and vorticity confinement make the result reproducible. The atlas
stores premultiplied emission RGB and coverage; no stock VC assets are changed.
"""
from pathlib import Path
import struct
import numpy as np
from scipy.ndimage import map_coordinates, gaussian_filter
from PIL import Image, ImageDraw

OUT = Path(__file__).resolve().parent
W, H, FRAMES, FPS = 96, 192, 64, 30
DT = 1 / (FPS * 3)
rng = np.random.default_rng(881985)
y, x = np.mgrid[:H, :W].astype(np.float32)
u = np.zeros((H, W), np.float32)
v = u.copy()
temperature = u.copy()
fuel = u.copy()
soot = u.copy()
noise = gaussian_filter(rng.normal(size=(H, W)).astype(np.float32), 1.2)
noise /= noise.std()

def advect(field):
    return map_coordinates(field, [np.clip(y-v*DT, 0, H-1), np.clip(x-u*DT, 0, W-1)], order=1, mode='nearest')

def step(t, envelope=1.0):
    global u, v, temperature, fuel, soot
    u, v = advect(u)*.995, advect(v)*.995
    temperature, fuel, soot = advect(temperature), advect(fuel), advect(soot)
    # Five independently breathing fuel jets break the base into flame tongues.
    for j, center in enumerate([23, 35, 48, 61, 73]):
        cx = center + 3.1*np.sin(t*(14+j*2)+j)
        jet = np.exp(-((x-cx)/3.6)**2 - ((y-3)/2.2)**2)
        strength = envelope*(.74+.26*np.sin(t*(23+j*3)+j*2))
        fuel += jet*strength*.19
        temperature += jet*strength*.27
        v += jet*strength*(140+25*np.sin(t*17+j)-v)*.30
        u += jet*np.sin(t*19+j)*strength*6
    # Buoyancy and confinement generate rolling eddies in the rising hot gas.
    v += DT*(temperature*245 - soot*8)
    curl = (np.roll(v,-1,1)-np.roll(v,1,1) - np.roll(u,-1,0)+np.roll(u,1,0))*.5
    ay, ax = np.gradient(np.abs(curl))
    norm = np.sqrt(ax*ax+ay*ay)+1e-5
    u += DT*7*ay/norm*curl
    v -= DT*7*ax/norm*curl
    divergence = (np.roll(u,-1,1)-np.roll(u,1,1)+np.roll(v,-1,0)-np.roll(v,1,0))*.5
    pressure = np.zeros_like(u)
    for _ in range(18):
        pressure = (np.roll(pressure,1,0)+np.roll(pressure,-1,0)+np.roll(pressure,1,1)+np.roll(pressure,-1,1)-divergence)*.25
        pressure[:,[0,-1]]=0
        pressure[[0,-1],:]=0
    u -= (np.roll(pressure,-1,1)-np.roll(pressure,1,1))*.5
    v -= (np.roll(pressure,-1,0)-np.roll(pressure,1,0))*.5
    u = np.clip(u,-130,130); v = np.clip(v,-80,240)
    reaction = np.minimum(fuel, DT*3.0*np.clip(temperature,0,1))
    fuel -= reaction
    temperature = np.clip((temperature+reaction*.8)*np.exp(-DT*2.2),0,2.0)
    soot = (soot+reaction*.22)*np.exp(-DT*2.1)
    u[:,[0,-1]]=0;v[0,:]=0
    temperature[:,[0,-1]]=0;temperature[-1,:]=0

def render():
    # Cooler luminous wisps are orange; dense hot cores are pale yellow. A
    # narrow fuel-rich reaction zone gets blue emission, not a blue outline.
    detail = map_coordinates(noise,[np.mod(y-v*.022,H),np.mod(x-u*.022,W)],order=1)
    heat = np.maximum(0,temperature*(1+.12*detail)-.075)
    coverage = 1-np.exp(-heat*2.8)
    brightness = np.clip(heat*1.3,0,1)
    green = .16+.76*np.clip(heat/1.2,0,1)**.75
    blue = .015+.55*np.clip((heat-.50)/1.25,0,1)**1.3
    rgb = np.stack([np.ones_like(heat),green,blue],axis=-1)
    # Broader blue/violet reaction tongues follow the advected heat instead
    # of separate hard blue dots at the injector positions. Vary the hue with
    # the flow, then feather the transition into the upper amber flame.
    hot_base = np.exp(-(y/22.0)**2)*np.clip(temperature*3.0+fuel*1.2,0,.98)
    violet = np.clip(y/19.0+detail*.12,0,1)[:,:,None]
    cool = np.array([.035,.26,1.0])*(1-violet)+np.array([.48,.105,.95])*violet
    rgb = rgb*(1-hot_base[:,:,None])+cool*hot_base[:,:,None]
    edge = np.clip(np.minimum(x,W-1-x)/8,0,1)*np.clip((H-1-y)/15,0,1)
    coverage *= edge
    emission = rgb*(coverage*brightness)[:,:,None]
    # Small subpixel scattering softens particle joins without blurring away
    # the rolling tongues. Keep the atlas border transparent after filtering.
    emission = (emission*.78+gaussian_filter(emission,(.9,.9,0))*.22)*edge[:,:,None]
    return np.uint8(np.clip(np.dstack([emission,coverage*.68]),0,1)*255)[::-1]

frames=[]
for variant in range(4):
    u.fill(0);v.fill(0);temperature.fill(0);fuel.fill(0);soot.fill(0)
    warmup=300+variant*47
    for pre in range(warmup): step(pre*DT)
    for frame in range(FRAMES):
        for sub in range(3):
            t=(frame*3+sub+1)*DT
            step(t+warmup*DT, float(np.clip((.85-t)/.28,0,1)))
        frames.append(Image.fromarray(render()).resize((128,256),Image.Resampling.LANCZOS))
atlas=Image.new('RGBA',(2048,4096))
for i,im in enumerate(frames):atlas.paste(im,((i%16)*128,(i//16)*256))
(OUT/'fire-burst.rgba').write_bytes(struct.pack('<II',*atlas.size)+atlas.tobytes())
atlas.save(OUT/'fire-burst-atlas.png')
preview=Image.new('RGB',(1024,580),(22,25,29))
draw=ImageDraw.Draw(preview)
for j,i in enumerate([5,10,16,22,28,36,45,56]):
    arr=np.asarray(frames[i]).astype(np.float32)/255
    bg=np.array([22,25,29])/255
    result=np.clip(arr[:,:,:3]+bg*(1-arr[:,:,3:4]),0,1)
    preview.paste(Image.fromarray(np.uint8(result*255)),(j*128,42))
    draw.text((j*128+8,20),f'{i/FPS:.2f}s',fill=(220,220,220))
draw.text((12,330),'Single simulated flame burst: ignition, curl, breakup, extinction',fill=(220,220,220))
preview.save(OUT/'single-flame-preview.png')
# Animation is a preview of emission compositing, not a game capture.
anim=[]
for im in frames[:64]:
    a=np.asarray(im).astype(np.float32)/255
    b=np.clip(a[:,:,:3]+np.array([.045,.05,.06])*(1-a[:,:,3:4]),0,1)
    anim.append(Image.fromarray(np.uint8(b*255)).resize((256,512)))
anim[0].save(OUT/'single-flame.gif',save_all=True,append_images=anim[1:],duration=33,loop=0)
print('Wrote four 64-frame flame bursts in a 2048x4096 atlas and single-flame previews.')
