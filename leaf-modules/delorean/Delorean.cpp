// Native reVC adaptation of bttfhillvalley/vc-cleo-plugins and HV's CLEO scripts.
// No CLEO binary, retail executable addresses, or on-disk model replacements.
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "common.h"
#include "leaf_api.h"
#include "LeafMods.h"
#include "ModelInfo.h"
#include "ModelIndices.h"
#include "Streaming.h"
#include "TxdStore.h"
#include "FileLoader.h"
#include "NodeName.h"
#include "World.h"
#include "Pools.h"
#include "PlayerPed.h"
#include "Automobile.h"
#include "HandlingMgr.h"
#include "Pad.h"
#include "Timer.h"
#include "Clock.h"
#include "Hud.h"
#include "Font.h"
#include "Text.h"
#include "Frontend.h"
#include "Camera.h"
#include "Wanted.h"
#include "Particle.h"
#include "Weather.h"
#include "Script.h"
#include "CarCtrl.h"
#include "ColModel.h"
#include "Object.h"
#include "VisibilityPlugins.h"
#include "Quaternion.h"
#include "Sprite2d.h"
#include "Sprite.h"
#include "OriginalComponents.h"
#include "ArrivalSequence.h"
#include "DonorSystems.h"
#include "IgnitionCycle.h"
#include "DestinationInput.h"
#include "TravelCalendar.h"
#include "DonorAudio.h"
#include "DonorCabin.h"
#include "DonorLegPose.h"
#include "PointLights.h"
#include "Bones.h"
#include "RpAnimBlend.h"
#include "LightBeams.h"
#include "DonorMirrors.h"

namespace {
const int Model = MI_DELUXO;
LeafHost host;
std::string root;
bool enabled = false, changedModel = false, circuits = true, fuel = true, hover = false;
bool boostActive = false;
bool instantTravelMode = false;
bool travelHudVisible = true;
DonorSystems::TravelCalendar travelCalendar;
DonorSystems::IgnitionCycle ignitionCycle;
struct CinematicTravel {
    bool active=false, vanished=false, fading=false, reentered=false, revealed=false;
    bool savedFirstPerson=false, savedDriverVisible=true, savedCollision=true, savedSpecialFov=false;
    uint32 started=0, revealAt=0;
    CMatrix matrix;
    CVector velocity=CVector(0,0,0), turn=CVector(0,0,0);
    float savedFovForTrain=70.0f;
} cinematicTravel;
bool emergencyKeyHeld = false;
bool leftWindowKeyHeld = false, rightWindowKeyHeld = false;
uint32 keypadConfirmUntil = 0;
bool destinationConfirmActive=false;
uint32 destinationConfirmStarted=0;
bool displayStartupActive=false;
uint32 displayStartupStarted=0;
float timeCircuitShutter=0.0f;
double timeCircuitShutterPending=0.0;
bool autoLanding = false;
int hoverTransition = 0;
float hoverExtension = 0.0f, hoverPivot = 0.0f;
float hoverTargetZ = 0.0f;
CVector previousCompassVelocity(0,0,0);
bool compassVelocityReady=false;
bool refueling = false;
int plutonium = 5;
bool lowPower = false;
uint32 stallDeadline = 0;
int startAttempts = 0;
uint32 crankUntil = 0;
bool cranking = false;
uint32 refuelStarted = 0;
int plutoniumStage=-1;
bool refuelControlLocked=false;
uint32 nextFusionSteam=0;
const uint16 RefuelControlMask=0x8000;
int vehicleRef = -1, variant = 2;
int hookMode = 0; // BTTF1: 0=none, 1=holder only, 2=side hook. BTTF3: 0=stock, 1=horse hook, 2=railroad/firebox.
bool hookDeployed = false;
float hookMotion = 0.0f;
bool hookAnimating = false;
bool hookDoorReserved = false;
float fluxCabinAlpha=0;
int hoodCoilSegments[3]={4,4,4};
uint32 nextHoodCoilFlicker=0;
int sidLevels[10]={};
double sidPending=0;
float hookFlexPitch=0, hookFlexRoll=0, hookFlexPitchSpeed=0, hookFlexRollSpeed=0;
CVector hookPreviousVelocity(0,0,0);
bool hookWindReady=false;
uint32 cooldown = 0, flashUntil = 0, lastAnim = 0;
uint32 emptyFlashUntil = 0;
int emptyAudioPhase = -1;
bool emptyAudioActive = false;
uint32 coldStarted=0, nextColdParticle=0;
DonorSystems::ArrivalSequence arrival;
uint32 arrivalStarted=0;
CVector arrivalPosition(0,0,0);
int coldDelay=50, coldEmissions=0;
uint32 nextReactorSteam=0;
bool reactorVentPlayed=false;
std::vector<RwTexture*> presetTextures;
std::map<int,std::vector<RwRaster*>> presetRasters;
bool LoadPresetTextures(int slot){
    CTxdStore::PushCurrentTxd();CTxdStore::SetCurrentTxd(slot);
    bool ok=true;
    auto assign=[&](int type,std::initializer_list<const char*> names){
        auto &rasters=presetRasters[type];
        for(auto name:names){auto t=RwTextureRead(name,nil);if(!t){ok=false;continue;}presetTextures.push_back(t);rasters.push_back(RwTextureGetRaster(t));}
        if(rasters.size()==names.size()){
            auto &preset=mod_ParticleSystemManager.m_aParticles[PARTICLE_LEAF_FIRST+type];
            preset.m_ppRaster=rasters.data();preset.m_nFinalAnimationFrame=Min(int(preset.m_nFinalAnimationFrame),int(rasters.size()-1));
        }
    };
    assign(PARTICLE_SPARK,{"rainsmall"});assign(PARTICLE_SPARK_SMALL,{"rainsmall"});
    assign(PARTICLE_ROCKET_SMOKE,{"cloudmasked"});
    assign(PARTICLE_WATER_SPARK,{"spark"});assign(PARTICLE_ENGINE_STEAM,{"smokeII_3"});
    assign(PARTICLE_DRY_ICE,{"smoke1","smoke2","smoke3","smoke4","smoke5"});
    assign(PARTICLE_BULLETHIT_SMOKE,{"smoke1","smoke2","smoke3","smoke4","smoke5"});
    assign(PARTICLE_FLAME,{"flame1"});assign(PARTICLE_CARFLAME,{"flame1"});
    assign(PARTICLE_EXPLOSION_LARGE,{"explo01","explo02","explo03","explo04","explo05","explo06"});
    CTxdStore::PopCurrentTxd();return ok;
}
int wormholeFrame=0;
int previousWormholeFrame=0;
float wormholeTicks=0.0f, coilAlpha=0.0f;
uint32 nextPlasmaParticle=0;
bool sparkLoopActive=false;
bool hoverAccelerating=false, hoverThrusterActive=false;
struct DepartureTrail {
    bool active=false, grounded=false, ignited=false;
    uint32 started=0, nextEmission=0;
    CVector left=CVector(0,0,0), right=CVector(0,0,0), delta=CVector(0,0,0);
} departureTrail;
struct DetachedPlate {
    int objectRef=-1;
    bool detached=false, active=false, landed=false;
    uint32 started=0;
    CVector position=CVector(0,0,0), velocity=CVector(0,0,0);
    float spin=0, pitch=-65, spinSpeed=0, pitchSpeed=0;
} detachedPlate;
DonorSystems::Dashboard dashboard;
DonorSystems::Shifter cabinShifter;
DonorSystems::UnderbodyLights underbodyLights;
DonorSystems::EmergencyLight emergencyLight;
DonorSystems::ConsoleClock consoleClock;
DonorSystems::CompassState compassState;
DonorSystems::DoorSounds doorSounds;
DonorSystems::Wipers cabinWipers;
DonorSystems::Pedals cabinPedals;
DonorSystems::Signals cabinSignals;
DonorSystems::Windows cabinWindows;
DonorSystems::ReactorGauges reactorGauges;
DonorSystems::EngineSounds engineSounds;
bool keys[256] = {};
bool debugPlayerPlaced=false;
int destinationDate = 20151021, destinationTime = 1629;
int presentDate = 19851026, departedDate = 19851026, departedTime = 121;
std::string digits;
std::map<std::string, RwFrame*> frames;
std::map<std::string, RwMatrix> originals;
std::set<std::string> hidden;
tHandlingData handling, oldHandling;
CColModel *oldCol = nil;
bool oldColOwn = false;
float oldWheelScale = 0;
int oldTxd = -1, txd = -1, particleTxd = -1, beamTxd = -1;
char oldName[24] = {}, oldGameName[10] = {};
CVehicleModelInfo *model = nil;
RwTexture *wormholeTextures[70] = {};
RwTexture *wormholeRedTextures[70] = {};
RwTexture *implosionTextures[21] = {};
rw::gl3::Shader *implosionShader=nil;
rw::gl3::Shader *wormholeShader=nil;
void InitialiseWormholeShader(){
    const char *vertex=R"GLSL(
VSIN(ATTRIB_POS) vec3 in_pos;
VSOUT vec2 uv;
void main(){gl_Position=u_proj*u_view*u_world*vec4(in_pos,1.0);uv=in_tex0;}
)GLSL";
    const char *fragment=R"GLSL(
uniform sampler2D tex0;uniform sampler2D tex1;
uniform float frameBlend;uniform float charge;uniform float effectTime;uniform float redMode;
FSIN vec2 uv;
vec4 sampleFrame(vec2 p){
 vec4 a=texture(tex1,p),b=texture(tex0,p);
 return mix(vec4(a.rgb*a.a,a.a),vec4(b.rgb*b.a,b.a),frameBlend);
}
void main(){
 // UVs are always the donor mesh UVs. No camera projection or scrolling.
 vec4 s=sampleFrame(uv);
 vec2 d=1.5/vec2(textureSize(tex0,0));
 vec4 halo=(sampleFrame(uv+vec2(d.x,0))+sampleFrame(uv-vec2(d.x,0))+
            sampleFrame(uv+vec2(0,d.y))+sampleFrame(uv-vec2(0,d.y)))*.25;
 float energy=max(s.r,max(s.g,s.b));
 float core=smoothstep(.42,.95,energy);
 float pulse=.94+.04*sin(effectTime*17.0)+.02*sin(effectTime*31.0);
 vec3 hue=mix(vec3(.18,.48,1.0),vec3(1.0,.28,.08),redMode);
 vec3 body=mix(s.rgb,hue*energy,.22);
 body=mix(body,vec3(1.0,.97,.91)*energy,core*.7);
 float glow=max(halo.r,max(halo.g,halo.b));
 vec3 light=(body*(1.0+.22*charge)+hue*glow*.3)*pulse;
 float alpha=clamp(s.a+halo.a*.18,0.0,1.0);
 FRAGCOLOR(vec4(light/max(alpha,.0001),alpha));
}
)GLSL";
    const char *vs[]={rw::gl3::shaderDecl,rw::gl3::header_vert_src,vertex,nil};
    const char *fs[]={rw::gl3::shaderDecl,rw::gl3::header_frag_src,fragment,nil};
    wormholeShader=rw::gl3::Shader::create(vs,fs);
}
void InitialiseImplosionShader(){
    const char *vertex=R"GLSL(
VSIN(ATTRIB_POS) vec3 in_pos;
VSOUT vec2 uv;
void main(){gl_Position=u_proj*u_view*u_world*vec4(in_pos,1.0);uv=in_tex0;}
)GLSL";
    const char *fragment=R"GLSL(
uniform sampler2D tex0;uniform sampler2D tex1;uniform float frameBlend;
FSIN vec2 uv;
void main(){
 vec2 p=vec2(uv.x,1.0-uv.y);
 vec4 a=texture(tex0,p),b=texture(tex1,p);
 float alpha=mix(a.a,b.a,frameBlend);
 vec3 premult=mix(a.rgb*a.a,b.rgb*b.a,frameBlend);
 FRAGCOLOR(vec4(premult/max(alpha,.00001),alpha));
}
)GLSL";
    const char *vs[]={rw::gl3::shaderDecl,rw::gl3::header_vert_src,vertex,nil};
    const char *fs[]={rw::gl3::shaderDecl,rw::gl3::header_frag_src,fragment,nil};
    implosionShader=rw::gl3::Shader::create(vs,fs);
}
RwTexture *beamTexture = nil;
RwFrame *cabinEmitterFrame=nil;
RwV3d cabinEmitterCenter={0,0,0};

void Log(const char *s) { if (host.log) host.log(s); }
float Bound(float f, float a, float b) { return f < a ? a : f > b ? b : f; }
void Help(const char *s) { wchar text[256]; AsciiToUnicode(s, text); CHud::SetHelpMessage(text, true); }
void Sound(const char *name) {
    if(!DonorAudio::Play(name)) Log("DeLorean sound unavailable (OpenAL/WAV)");
}
void SoundLoop(const char *name) {
    if(!DonorAudio::Play(name,true)) Log("DeLorean loop unavailable (OpenAL/WAV)");
}
void SoundAt(const char *name,float x,float y,float z,float range,bool loop=false,float gain=1) {
    if(!DonorAudio::Play(name,loop)) Log("DeLorean cabin sound unavailable");
    DonorAudio::Configure(name,x,y,z,range,gain);
}
void SoundWorld(const char *clip,const char *voice,const CVector &position,float range) {
    if(DonorAudio::Play(clip,false,voice))
        DonorAudio::ConfigureWorld(voice,position.x,position.y,position.z,range);
}
bool Press(int key) {
    const bool down = (GetAsyncKeyState(key) & 0x8000) != 0;
    bool edge = down && !keys[key]; keys[key] = down; return edge;
}
CAutomobile *Car() {
    CVehicle *p = vehicleRef >= 0 ? CPools::GetVehicle(vehicleRef) : nil;
    return p && p->GetModelIndex() == Model && p->IsCar() ? (CAutomobile*)p : nil;
}
void Hide(const std::vector<std::string> &names) { hidden.insert(names.begin(), names.end()); }
void Show(const std::vector<std::string> &names) { for (const auto &n : names) hidden.erase(n); }
void HideAllPlates() {
    for(const char *name : {"plate", "platebttf2", "platestock", "plate_back"})
        hidden.insert(name);
}
void ShowOnlyPlate(const char *active) {
    HideAllPlates();
    if(active && frames.find(active) != frames.end())
        hidden.erase(active);
}
bool IsBttf3() { return variant == 3; }
bool UsesMrFusion() { return variant == 2 || variant == 3; }
RwFrame *Remember(RwFrame *f, void *) {
    const char *n = GetFrameNodeName(f);
    if (n && *n) { frames[n] = f; originals[n] = *RwFrameGetMatrix(f); }
    RwFrameForAllChildren(f, Remember, nil); return f;
}
RpAtomic *Visibility(RpAtomic *a, void *) {
    bool plateAtomic=false, hiddenByName=false;
    for (RwFrame *f = RpAtomicGetFrame(a); f; f = RwFrameGetParent(f)) {
        const char *n = GetFrameNodeName(f);
        if(n && strcmp(n,"plate")==0) plateAtomic=true;
        // `_fr` nodes are the intact/frosted variants used by the donor
        // model (including the gullwing doors).  Only the explicit variant
        // table and damage nodes should control visibility here.
        if (n && (hidden.count(n) || strstr(n,"_dam"))) { hiddenByName = true; break; }
    }
    bool show = !(cinematicTravel.active && cinematicTravel.vanished && !cinematicTravel.revealed);
    if(detachedPlate.detached && plateAtomic) show=false;
    if(hiddenByName) show=false;
    RpAtomicSetFlags(a, show ? rpATOMICRENDER : 0); return a;
}
void ApplyVisibility() { if (Car()) RpClumpForAllAtomics((RpClump*)Car()->m_rwObject, Visibility, nil); }
RpMaterial *DonorGlowMaterial(RpMaterial *material, void *) {
    RwSurfaceProperties surface=*RpMaterialGetSurfaceProperties(material);
    // vc-cleo-plugins SetAmbientCB uses 5.0 for glow, retaining lighting.
    surface.ambient=5.0f;
    material->surfaceProps=surface;
    return material;
}
RpAtomic *DonorGlowAtomic(RpAtomic *atomic, void *) {
    static const std::set<std::string> glowing=[](){
        std::set<std::string> result(GLOWING_COMPONENTS.begin(),GLOWING_COMPONENTS.end());
        result.insert(GLOWING_HIDDEN_COMPONENTS.begin(),GLOWING_HIDDEN_COMPONENTS.end());
        result.insert({"lightRl","lightRr","lightSl","lightSr","lightRVl","lightRVr"});
        result.insert({"fxwheelbttf2rbon","fxwheelbttf2rfon","fxwheelbttf2lbon","fxwheelbttf2lfon"});
        return result;
    }();
    bool glow=false;
    for(RwFrame *frame=RpAtomicGetFrame(atomic);frame;frame=RwFrameGetParent(frame)) {
        const char *name=GetFrameNodeName(frame);if(!name)continue;
        if(glowing.count(name)) {glow=true;break;}
        for(const char *prefix:{"dtmonth","ptmonth","ltdmonth","dtday","ptday","ltdday",
            "dtyear","ptyear","ltdyear","dthour","pthour","ltdhour","dtmin","ptmin","ltdmin",
            "digitalspeedodigit","sidledsline","consoleclockdigit"})
            if(strncmp(name,prefix,strlen(prefix))==0)glow=true;
    }
    if(glow){
        RpGeometry *geometry=RpAtomicGetGeometry(atomic);
        RpGeometryForAllMaterials(geometry,DonorGlowMaterial,nil);
        RpGeometrySetFlags(geometry,RpGeometryGetFlags(geometry)|rpGEOMETRYLIGHT);
    }
    return atomic;
}
RpMaterial *CoilMaterial(RpMaterial *material, void *) {
    RwRGBA color=*RpMaterialGetColor(material);
    color.alpha=(uint8)coilAlpha;
    RpMaterialSetColor(material,&color);
    DonorGlowMaterial(material,nil);
    return material;
}
RpAtomic *CoilAtomic(RpAtomic *atomic, void *) {
    const char *name=GetFrameNodeName(RpAtomicGetFrame(atomic));
    if(name && (strncmp(name,"fluxcoilson",10)==0 || strcmp(name,"fluxemitteron")==0)) {
        RpGeometry *geometry=RpAtomicGetGeometry(atomic);
        RpGeometryForAllMaterials(geometry,CoilMaterial,nil);
        RpGeometrySetFlags(geometry,RpGeometryGetFlags(geometry)|rpGEOMETRYMODULATEMATERIALCOLOR|rpGEOMETRYLIGHT);
    }
    return atomic;
}
void Rotate(const char *name, CVector axis, float degrees) {
    auto f = frames.find(name); if (f == frames.end()) return;
    *RwFrameGetMatrix(f->second) = originals[name];
    RwFrameRotate(f->second, &axis, degrees, rwCOMBINEPRECONCAT); RwFrameUpdateObjects(f->second);
}
void SetRotation(const char *name, const CVector &rotation) {
    auto f=frames.find(name); if(f==frames.end()) return;
    CMatrix mat(RwFrameGetMatrix(f->second));
    const CVector pos=mat.GetPosition();
    mat.SetRotate(rotation.x,rotation.y,rotation.z); mat.GetPosition()=pos; mat.UpdateRW();
    RwFrameUpdateObjects(f->second);
}
void SetLocalPosition(const char *name, const CVector &position) {
    auto f=frames.find(name); if(f==frames.end()) return;
    RwFrameGetMatrix(f->second)->pos=position;
    RwFrameUpdateObjects(f->second);
}
CVector RotationOf(const char *name) {
    auto f=frames.find(name); if(f==frames.end()) return CVector(0,0,0);
    RwMatrix *m=RwFrameGetMatrix(f->second);
    const float sy=sqrtf(m->right.x*m->right.x+m->up.x*m->up.x);
    return CVector(atan2f(-m->at.y,m->at.z),atan2f(-m->at.x,sy),atan2f(m->up.x,m->right.x));
}
void Variation();
void UpdateDoors(CAutomobile *car) {
    const char *dummy[] = {"door_lf_dummy", "door_rf_dummy"};
    const char *visual[] = {"fxdoorlf_", "fxdoorrf_"};
    const char *strut[] = {"doorlfstrut_", "doorrfstrut_"};
    const char *piston[] = {"doorlfstrutp", "doorrfstrutp"};
    const eDoors doors[] = {DOOR_FRONT_LEFT, DOOR_FRONT_RIGHT};
    for(int i=0;i<2;i++) {
        if(doorSounds.Opened(i,car->IsDoorClosed(doors[i]))){
            const char *voice=i?"door-right":"door-left";
            DonorAudio::Play("delorean/door.wav",false,voice);
            DonorAudio::Configure(voice,i?1.0f:-1.0f,0,0,10);
            if(DonorAudio::IsPlaying("delorean/cold.wav")){
                const char *iceVoice=i?"door-ice-right":"door-ice-left";
                DonorAudio::Play("delorean/door_ice.wav",false,iceVoice);
                DonorAudio::Configure(iceVoice,i?1.0f:-1.0f,0,0,10);
            }
        }
        // The donor's visible gullwing is a sibling of GTA's door dummy.
        // Counter-rotate it exactly as vc-cleo-plugins does.
        const CVector r=RotationOf(dummy[i]);
        SetRotation(visual[i],CVector(-r.x,-r.y,-r.z));
        const float angle=fabsf(car->Doors[doors[i]].m_fAngle);
        const float length=sqrtf(0.105361f*0.105361f+0.364084f*0.364084f-
            2.0f*0.105361f*0.364084f*cosf(angle));
        const float tilt=(DEGTORAD(86.6f)-asinf(Bound(sinf(angle)*0.105361f/length,-1,1)))*(i?-1:1);
        auto s=frames.find(strut[i]);
        if(s!=frames.end()) {
            CMatrix mat(RwFrameGetMatrix(s->second));
            const CVector pos=mat.GetPosition();
            mat.SetRotateY(tilt); mat.GetPosition()=pos; mat.UpdateRW();
            RwFrameUpdateObjects(s->second);
        }
        auto p=frames.find(piston[i]);
        if(p!=frames.end()) {
            RwFrameGetMatrix(p->second)->pos.z=length-0.259f;
            RwFrameUpdateObjects(p->second);
        }
    }
}
void UpdatePanels(CAutomobile *car) {
    // Donor FixDoors mirrors GTA's live bonnet and rear engine-cover angles
    // onto the custom component hierarchy after every reset/damage update.
    SetRotation("bonnet_dummy",CVector(car->Doors[DOOR_BONNET].m_fAngle,0,0));
    SetRotation("boot_dummy",CVector(car->Doors[DOOR_BOOT].m_fAngle,0,0));
}
void UpdateWheels(CAutomobile *car) {
    const char *suffix[]={"lf","lb","rf","rb"};
    const char *fx[]={"fxwheellf_","fxwheellb_","fxwheelrf_","fxwheelrb_"};
    const char *holder[]={"holderlf_","holderlb_","holderrf_","holderrb_"};
    const char *arm[]={"strutarmlf_","strutarmlb_","strutarmrf_","strutarmrb_"};
    const char *dummy[]={"wheel_lf_dummy","wheel_lb_dummy","wheel_rf_dummy","wheel_rb_dummy"};
    const float damper=1.0f-hoverExtension/0.3f;
    const float steeringDamper=1.0f-hoverPivot/90.0f;
    for(int i=0;i<4;i++) {
        const float steer=i==0||i==2 ? car->m_fSteerAngle*16.0f/30.0f*steeringDamper : 0.0f;
        SetRotation(holder[i],CVector(0,0,steer));
        // Keep the live GTA tire spin while its hover parent changes angle.
        SetRotation(fx[i],CVector(car->m_aWheelRotation[i],0,0));
        auto d=frames.find(dummy[i]);
        if(d!=frames.end()) {
            const float base=(i==0||i==2)?-0.280126f:-0.304341f;
            SetLocalPosition(arm[i],CVector(0,0,(RwFrameGetMatrix(d->second)->pos.z-base)*damper));
        }
    }
}
void HoverSmoke(CAutomobile *car) {
    const float points[8][3]={{.75f,.8f,-.425f},{-.75f,.8f,-.425f},{.625f,-.6f,-.425f},{-.625f,-.6f,-.425f},
        {.795f,1.35f,-.3f},{-.795f,1.35f,-.3f},{.795f,-1.295f,-.3f},{-.795f,-1.295f,-.3f}};
    for(const auto &p:points) {
        CVector pos=car->GetPosition()+car->GetRight()*p[0]+car->GetForward()*p[1]+car->GetUp()*p[2];
        for(int i=0;i<3;i++) CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_ROCKET_SMOKE),pos,CVector(car->GetMoveSpeed().x*.5f,car->GetMoveSpeed().y*.5f,.03f),nil,.5f,RwRGBA{255,255,255,255},0,0,0,5000);
    }
}
void ApplyHoverModel() {
    const float pistonExt=DEGTORAD(hoverPivot)*-0.043315f; // donor: degrees * -0.000756
    SetLocalPosition("shockpistonrb_",CVector(0,0,pistonExt));
    SetLocalPosition("shockpistonrf_",CVector(0,0,pistonExt));
    SetLocalPosition("shockpistonlb_",CVector(0,0,pistonExt));
    SetLocalPosition("shockpistonlf_",CVector(0,0,pistonExt));
    SetLocalPosition("strutunitrb_",CVector( hoverExtension,0,0));
    SetLocalPosition("strutunitrf_",CVector( hoverExtension,0,0));
    SetLocalPosition("strutunitlb_",CVector(-hoverExtension,0,0));
    SetLocalPosition("strutunitlf_",CVector(-hoverExtension,0,0));
    const float shock=DEGTORAD(hoverPivot*-0.002f);
    SetRotation("shockrb_",CVector(0, shock,0)); SetRotation("shockrf_",CVector(0, shock,0));
    SetRotation("shocklb_",CVector(0,-shock,0)); SetRotation("shocklf_",CVector(0,-shock,0));
    SetRotation("hoverjointrb_",CVector(0, DEGTORAD(hoverPivot),0));
    SetRotation("hoverjointrf_",CVector(0, DEGTORAD(hoverPivot),0));
    SetRotation("hoverjointlb_",CVector(0,-DEGTORAD(hoverPivot),0));
    SetRotation("hoverjointlf_",CVector(0,-DEGTORAD(hoverPivot),0));
}
void ApplyBttf3FrontSuspension(CAutomobile *car, bool raise) {
    if(!car) return;
    CPlayerInfo *playerInfo=&CWorld::Players[CWorld::PlayerInFocus];
    if(!raise) {
        if(car->bUsingSpecialColModel && playerInfo->m_pVehicleEx == car) {
            playerInfo->m_pVehicleEx=nil;
            car->bUsingSpecialColModel=false;
            car->SetupSuspensionLines();
        }
        return;
    }
    if(car->bUsingSpecialColModel && playerInfo->m_pVehicleEx == car)
        return;
    if(playerInfo->m_pVehicleEx && playerInfo->m_pVehicleEx != car)
        playerInfo->m_pVehicleEx->bUsingSpecialColModel=false;
    CVehicleModelInfo *mi=(CVehicleModelInfo*)CModelInfo::GetModelInfo(car->GetModelIndex());
    CColModel *normalColModel=mi->GetColModel();
    playerInfo->m_pVehicleEx=car;
    playerInfo->m_ColModel=*normalColModel;
    car->bUsingSpecialColModel=true;
    CColModel *specialColModel=&playerInfo->m_ColModel;
    const float wheelRadius=mi->m_wheelScale*0.5f;
    const float normalUpper=car->pHandling->fSuspensionUpperLimit;
    const float normalLower=car->pHandling->fSuspensionLowerLimit;
    const float normalSpringLength=normalUpper-normalLower;
    const float normalLineLength=normalSpringLength+wheelRadius;
    const float frontSpringLength=normalSpringLength;
    const float rearSpringLength=normalSpringLength;
    CVector pos;
    for(int i=0;i<4;i++) {
        const bool rear=(i==CARWHEEL_REAR_LEFT || i==CARWHEEL_REAR_RIGHT);
        mi->GetWheelPosn(i,pos);
        car->m_aWheelPosition[i]=pos.z;
        // User-selected BTTF3 rear height; retain the raised whitewall front.
        pos.z += rear ? 0.02f : normalUpper-0.15f;
        specialColModel->lines[i].p0=pos;
        pos.z -= normalLineLength;
        specialColModel->lines[i].p1=pos;
        car->m_aSuspensionSpringLength[i]=rear ? rearSpringLength : frontSpringLength;
        car->m_aSuspensionLineLength[i]=specialColModel->lines[i].p0.z-specialColModel->lines[i].p1.z;
        car->m_aSuspensionSpringRatio[i]=1.0f;
    }
    mi->GetWheelPosn(0,pos);
    const float minz=pos.z + normalLower - 0.15f - wheelRadius;
    if(minz < specialColModel->boundingBox.min.z)
        specialColModel->boundingBox.min.z=minz;
    const float radius=Max(specialColModel->boundingBox.min.Magnitude(),specialColModel->boundingBox.max.Magnitude());
    if(specialColModel->boundingSphere.radius < radius)
        specialColModel->boundingSphere.radius=radius;
}
void StartHoverConversion(CAutomobile *car) {
    if(hoverTransition) return;
    car->bNotDamagedUpsideDown=true;
    if(hover) {
        if(!autoLanding) {
            autoLanding=true;
            hoverTargetZ=car->GetPosition().z;
            Help("Automatic landing engaged.");
            Log("Automatic hover landing engaged");
        }
    } else {
        hover=true; autoLanding=false; hoverTargetZ=car->GetPosition().z; hoverTransition=1; Variation(); HoverSmoke(car);
        Sound("delorean/hover_extend.wav"); Log("Hover flight conversion started");
    }
}
void UpdateHoverConversion(CAutomobile *car) {
    if(!hoverTransition) return;
    // CLEO wait 10 yields for at least a rendered frame. Preserve the donor's
    // 0.01-unit and 5-degree increments at the game's normalized frame rate.
    const float step=Bound(CTimer::GetTimeStep(),0.0f,3.0f);
    if(hoverTransition>0) {
        if(hoverExtension<0.3f) hoverExtension=Min(0.3f,hoverExtension+step*.01f);
        else hoverPivot=Min(90.0f,hoverPivot+step*5.0f);
        if(hoverPivot>=90.0f) {
            hoverTransition=0; Log("Hover flight conversion complete");
        }
    } else {
        if(hoverPivot>0.0f) hoverPivot=Max(0.0f,hoverPivot-step*5.0f);
        else hoverExtension=Max(0.0f,hoverExtension-step*.01f);
        if(hoverExtension<=0.0f) {
            hoverTransition=0; hover=false; autoLanding=false; car->bNotDamagedUpsideDown=false; Variation(); DonorAudio::Stop("delorean/landspeeder_loop_lower_pitch.wav"); boostActive=false; Log("Hover landing conversion complete");
        }
    }
    ApplyHoverModel(); UpdateWheels(car);
}
void HoverEffects(CAutomobile *car,bool focused) {
    if(cinematicTravel.active)return;
    if(!hover || hoverPivot<89.0f) {
        hoverAccelerating=false;hoverThrusterActive=false;return;
    }
    CPad *pad=CPad::GetPad(0);
    const bool controls=focused && FindPlayerVehicle()==car;
    const bool boost=controls && pad->GetHandBrake() && pad->GetAccelerate()>153;
    const float speed=car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND;
    const bool accelerating=controls && pad->GetAccelerate()>=150;
    if(accelerating && !hoverAccelerating && speed>30.0f)
        SoundAt("delorean/landspeeder_accelerate_2_lower_pitch.wav",0,0,0,10);
    if(!accelerating && hoverAccelerating && speed>30.0f)
        SoundAt("delorean/landspeeder_decelerate_2_lower_pitch.wav",0,0,0,10);
    hoverAccelerating=accelerating;
    const bool rising=controls && pad->GetSteeringUpDown()<-30 && !accelerating;
    if(rising && !hoverThrusterActive)SoundAt("delorean/wheel_thrust.wav",0,0,0,10);
    hoverThrusterActive=rising;
    if(boost && !boostActive) { Sound("delorean/boost.wav"); Log("Hover boost engaged"); }
    boostActive=boost;
    const char *thrusters[]={"fxthrusterbttf2lfon","fxthrusterbttf2rfon","fxthrusterbttf2lbon","fxthrusterbttf2rbon"};
    Show(THRUSTER_COMPONENTS);
    Show({thrusters[0],thrusters[1],thrusters[2],thrusters[3],
        "fxthrusterbttf2lfth","fxthrusterbttf2rfth","fxthrusterbttf2lbth","fxthrusterbttf2rbth"});
    if(boost) Show({"inner_vents","inner_ventsglow"});
    else { hidden.insert("inner_vents"); hidden.insert("inner_ventsglow"); }
    ApplyVisibility();
    // The stock spark texture is not the donor's replacement glow artwork.
    // Use the car's authored thruster meshes until that artwork is ported.
    if(false) {
        const float x[4]={-1.18f,1.18f,-1.18f,1.18f}, y[4]={1.353f,1.353f,-1.296f,-1.296f};
        for(int i=0;i<4;i++) {
            CVector pos=car->GetPosition()+car->GetRight()*x[i]+car->GetForward()*y[i]-car->GetUp()*.65f;
            CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_WATER_SPARK),pos,CVector(0,0,0),nil,.10f,RwRGBA{255,200,175,255},0,0,0,10);
        }
        if(boost) {
            for(int side=-1;side<=1;side+=2) {
                CVector pos=car->GetPosition()+car->GetRight()*(side*.45f)-car->GetForward()*1.8f+car->GetUp()*.55f;
                CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_WATER_SPARK),pos,CVector(0,0,0),nil,.20f,RwRGBA{255,170,80,255},0,0,0,10);
            }
        }
    }
}
void ReleaseRefuelControl() {
    if(refuelControlLocked) CPad::GetPad(0)->SetEnablePlayerControls(RefuelControlMask);
    refuelControlLocked=false;
}
void CancelRefuel() {
    ReleaseRefuelControl();
    if(!refueling) return;
    refueling=false;
    if(Car() && Car()->m_rwObject) {
        SetRotation("fusionlatch",CVector(0,0,0)); SetRotation("fusion",CVector(0,0,0));
        SetRotation("reactorlidbttf1",CVector(0,0,0));
        SetLocalPosition("reactorlidbttf1",CVector(0,0,0));
        Variation();
    }
}
void BeginRefuel(CAutomobile *car) {
    if(refueling || !FindPlayerPed()) return;
    if(FindPlayerPed()->bInVehicle) { Help("Exit the vehicle to refuel the DeLorean."); return; }
    if(fuel) { Help("The time machine is already fueled."); return; }
    if(variant==1 && plutonium<=0) { Help("No plutonium cans remain."); return; }
    if(car->GetMoveSpeed().Magnitude()>=0.02f || CPad::GetPad(0)->ArePlayerControlsDisabled()) return;
    refueling=true; refuelStarted=CTimer::GetTimeInMilliseconds();
    nextFusionSteam=refuelStarted; plutoniumStage=-1;
    CPad::GetPad(0)->SetDisablePlayerControls(RefuelControlMask); refuelControlLocked=true;
    CPlayerPed *ped=FindPlayerPed();
    ped->m_fRotationCur=ped->m_fRotationDest=car->GetForward().Heading();
    ped->SetHeading(ped->m_fRotationCur);
    if(UsesMrFusion()) Sound("delorean/fusion_open.wav");
    Help(UsesMrFusion()?"Refueling Mr. Fusion...":"Loading plutonium...");
    Log("Refuel sequence started");
}
void UpdateRefuel() {
    if(!refueling) return;
    CAutomobile *car=Car(); CPlayerPed *ped=FindPlayerPed();
    if(!car || !ped || ped->m_fHealth<=0 || car->GetStatus()==STATUS_WRECKED || ped->bInVehicle ||
       (ped->GetPosition()-car->GetPosition()).Magnitude()>6.0f) { CancelRefuel(); return; }
    const uint32 now=CTimer::GetTimeInMilliseconds();
    const double t=(now-refuelStarted)/1000.0;
    const DonorSystems::Stage pose=UsesMrFusion()?DonorSystems::Fusion(t):DonorSystems::Reactor(t);
    const int stage=pose.index; const float q=pose.progress;
    const int closedStage=UsesMrFusion()?6:16;
    // Walk across every crossed stage so a slow frame cannot skip a sound cue.
    for(int event=plutoniumStage+1;event<=stage;++event) {
        if(UsesMrFusion()) {
            if(event==3) {char clip[48];snprintf(clip,sizeof(clip),"delorean/fusion_trash%d.wav",1+rand()%3);Sound(clip);}
            if(event==4) Sound("delorean/fusion_close.wav");
        } else {
            if(event==1) Sound("delorean/nuke_1.wav");
            if(event==6) Sound("delorean/nuke_2.wav");
            if(event==13) Sound("delorean/nuke_3.wav");
        }
    }
    plutoniumStage=stage;
    if(UsesMrFusion()) {
        const float latch=stage==0?45*q:stage<5?45:stage==5?45*(1-q):0;
        const float lid=stage<1?0:stage==1?-90*q:stage<4?-90:stage==4?-90*(1-q):0;
        SetRotation("fusionlatch",CVector(DEGTORAD(latch),0,0));
        SetRotation("fusion",CVector(DEGTORAD(lid),0,0));
        // User-requested earlier steam starts with the red latch; donor locations.
        if(stage<4 && now>=nextFusionSteam) {
            const CVector upper=car->GetPosition()-car->GetForward()*1.69f+car->GetUp()*.6f;
            const CVector lower=car->GetPosition()-car->GetForward()*1.69f-car->GetUp()*1.6f;
            CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_BULLETHIT_SMOKE),upper,CVector(0,0,0),nil,.01f);
            CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_ENGINE_STEAM),lower,CVector(0,0,0),nil,.01f);
            nextFusionSteam=now+67;
        }
    } else {
        const bool canVisible=stage>=4 && stage<=10;
        if(canVisible) Show({"plutcan","plutcanliquid","plutcaninterior"});
        else Hide({"plutcan","plutcanliquid","plutcaninterior"});
        if(stage>=4 && stage<=7) Show({"plut"}); else Hide({"plut"});
        if(stage>=3 && stage<12) hidden.insert("reactorlidbttf1"); else hidden.erase("reactorlidbttf1");
        float lidAngle=stage==0?q*90:stage<15?90:stage==15?(1-q)*90:0;
        float lidZ=stage<1?0:stage==1?q*.05f:stage<13?.05f:stage==13?(1-q)*.05f:0;
        SetRotation("reactorlidbttf1",CVector(0,0,DEGTORAD(lidAngle)));
        SetLocalPosition("reactorlidbttf1",CVector(0,0,lidZ));
        float canZ=stage<5?.2f:stage==5?.2f-.21f*q:stage<9?-.01f:stage==9?.2f*q:.2f;
        float canAngle=stage<6?90:stage==6?(1-q)*90:0;
        for(const char *name:{"plutcan","plutcanliquid","plutcaninterior","plut"}) {
            SetLocalPosition(name,CVector(0,0,stage==7 && strcmp(name,"plut")==0?-.01f-.2f*q:canZ));
            SetRotation(name,CVector(0,0,DEGTORAD(canAngle)));
        }
    }
    if(stage>=closedStage) hidden.insert("pchamberemptylight");
    if(stage>=closedStage+1) ReleaseRefuelControl();
    if(stage>=closedStage+2) {
        refueling=false; fuel=true;
        lowPower=false; startAttempts=0;
        emptyAudioActive=false;
        if(variant==1 && plutonium>0) --plutonium;
        Variation();Help("Time machine refueled.");Log("Refuel sequence complete");
    }
    ApplyVisibility();
}
void Variation() {
    LeafMods::SetRailWheelVehicle(variant==3 && hookMode==2?Car():nil);
    hidden.clear();
    for(const auto &f : frames) {
        const std::string &n=f.first;
        if(n.find("wormhole")==0 || n.find("digitalspeedodigit")==0 || n.find("plutcan")==0 ||
           n.find("dtmonth")==0 || n.find("ptmonth")==0 || n.find("ltdmonth")==0 ||
           n.find("dtday")==0 || n.find("ptday")==0 || n.find("ltdday")==0 ||
           n.find("dtyear")==0 || n.find("ptyear")==0 || n.find("ltdyear")==0 ||
           n.find("dthour")==0 || n.find("pthour")==0 || n.find("ltdhour")==0 ||
           n.find("dtmin")==0 || n.find("ptmin")==0 || n.find("ltdmin")==0)
            if(!n.empty() && n.back()!='_') hidden.insert(n);
    }
    // Keep grouping frames visible; individual effect frames are selected below.
    hidden.erase("wormholes_");
    Hide(STOCK_COMPONENTS); Hide(TIME_MACHINE_COMPONENTS); Hide(PLATE_STOCK_COMPONENTS);
    Hide(PLATE_OUTATIME_COMPONENTS); Hide(PLATE_BARCODE_COMPONENTS); Hide(HOOK_UP_COMPONENTS);
    Hide(HOOK_SIDE_COMPONENTS); Hide(HOODBOX_COMPONENTS); Hide(HITCH_COMPONENTS);
    Hide(PLUTONIUM_CHAMBER_COMPONENTS); Hide(MR_FUSION_COMPONENTS);
    Hide(DRIVETRAIN_STOCK_COMPONENTS); Hide(DRIVETRAIN_HOVER_COMPONENTS);
    Hide(WHEEL_STOCK_COMPONENTS); Hide(WHEEL_WHITEWALLS_COMPONENTS); Hide(WHEEL_RAILROAD_COMPONENTS);
    Hide(THRUSTER_COMPONENTS); Hide(PLUTONIUM_COMPONENTS); Hide(GLOWING_HIDDEN_COMPONENTS);
    Hide(FROSTED_COMPONENTS); Hide(BULOVA_CLOCK_COMPONENTS); Hide(FIREBOX_GAUGE_COMPONENTS);
    Show(TIME_MACHINE_COMPONENTS); Show(WHEEL_STOCK_COMPONENTS);
    Show({"gloveboxgauges","gloveboxgaugesglass","gloveboxgaugeslights",
          "pchamberneedle","pchamberrefneedle","ppowerneedle","primaryneedle"});
    if (variant == 2) { Show(MR_FUSION_COMPONENTS); Show(DRIVETRAIN_HOVER_COMPONENTS); if(hookMode==0) Show(PLATE_BARCODE_COMPONENTS); }
    else if (variant == 3) {
        Show(MR_FUSION_COMPONENTS);
        Show(DRIVETRAIN_HOVER_COMPONENTS);
        Show(HOODBOX_COMPONENTS);
        Show({"xchassiscoverbttf2"});
        Hide(WHEEL_STOCK_COMPONENTS);
        if(hookMode==2) {
            Show(WHEEL_RAILROAD_COMPONENTS);
            Show(FIREBOX_GAUGE_COMPONENTS);
        } else {
            Show(WHEEL_WHITEWALLS_COMPONENTS);
            if(hookMode==1) Show(HITCH_COMPONENTS);
        }
    }
    else {
        Show(PLUTONIUM_CHAMBER_COMPONENTS);
        Show(DRIVETRAIN_STOCK_COMPONENTS);
        if(hookMode==0) { Show(PLATE_OUTATIME_COMPONENTS); Hide(PLATE_STOCK_COMPONENTS); hidden.erase("plate"); }
        if(hookMode==1) Show(HOOK_HOLDER_COMPONENTS);
        else if(hookMode==2) {
            Show(hookDeployed && !hookAnimating?HOOK_UP_COMPONENTS:HOOK_SIDE_COMPONENTS);
            if(hookDeployed && !hookAnimating) hidden.insert("hookcableplugbttf1");
        }
    }
    if(detachedPlate.detached) {
        // The detached mesh is the only visible plate during the BTTF1
        // departure; keep every alternate plate frame hidden underneath it.
        Hide(PLATE_STOCK_COMPONENTS); Hide(PLATE_BARCODE_COMPONENTS);
        if(detachedPlate.active && variant==1) hidden.erase("plate");
        else hidden.insert("plate");
    }
    if (hover) Show(THRUSTER_COMPONENTS);
    if (circuits) { hidden.erase("tcdswitchlighton"); hidden.erase("tcdkeypadlightson"); hidden.erase("fluxcapacitorlightson"); }
    else hidden.erase("tcdswitchlightoff");
    if (fuel) hidden.insert("pchamberemptylight"); else hidden.erase("pchamberemptylight");
    // Restore every plate frame exactly to its donor transform before
    // selecting the active variant below.
    for(const char *plateName:{"plate","platebttf2","platestock","plate_back"}) {
        auto f=frames.find(plateName); auto o=originals.find(plateName);
        if(f!=frames.end() && o!=originals.end()) {
            *RwFrameGetMatrix(f->second)=o->second;
            RwFrameUpdateObjects(f->second);
        }
    }
    if(detachedPlate.detached)
        ShowOnlyPlate(nil);
    else if(variant==2)
        ShowOnlyPlate("platebttf2");
    else if(variant==1 && hookMode==0)
        ShowOnlyPlate("plate");
    else
        ShowOnlyPlate(nil);
    Hide({"fxwheelbttf2rbon","fxwheelbttf2rfon","fxwheelbttf2lbon","fxwheelbttf2lfon",
          "fxthrusterbttf2rbon","fxthrusterbttf2rbth","fxthrusterbttf2rfon","fxthrusterbttf2rfth",
          "fxthrusterbttf2lbon","fxthrusterbttf2lbth","fxthrusterbttf2lfon","fxthrusterbttf2lfth"});
    ApplyBttf3FrontSuspension(Car(), variant==3 && hookMode!=2);
    // VehicleFlags.txt selects traction and exhaust from the fitted parts.
    // The native component list calls the whitewall wheel fxwheelbttf3rb.
    handling.Flags &= ~(HANDLING_GOOD_INSAND | HANDLING_NO_EXHAUST);
    if(variant==3 && hookMode!=2) handling.Flags |= HANDLING_GOOD_INSAND;
    if(hidden.count("exhaustmodel")) handling.Flags |= HANDLING_NO_EXHAUST;
    if(Car()) Car()->pHandling->Flags=handling.Flags;
    ApplyVisibility();
}
#include "RaisedFrost.h"
#include "FrostShader.h"
#include "CabinDetail.h"
#include "TravelArcs.h"
#include "HoodboxEffects.h"
#include "Plasma.h"
#include "FireTrails.h"
RpAtomic *RememberCabinEmitter(RpAtomic *,void *);
void Spawn() {
    DonorMirrors::Clear();
    if (!enabled || !FindPlayerPed() || FindPlayerVehicle()) return;
    if (Car()) { Help("Your DeLorean is already spawned."); return; }
    CAutomobile *car = new CAutomobile(Model, MISSION_VEHICLE);
    if (!car) return;
    // Put it beside and slightly ahead of the player so a saved parked car
    // cannot steal the entry input intended for the DeLorean.
    CVector p = FindPlayerCoors() + FindPlayerPed()->GetRight() * 6.0f + FindPlayerPed()->GetForward() * 3.0f;
    bool ground = false;
    float z = CWorld::FindGroundZFor3DCoord(p.x, p.y, p.z + 3.0f, &ground);
    p.z = ground ? z + car->GetHeightAboveRoad() + 0.25f : p.z + 1.0f;
    car->SetPosition(p); car->SetOrientation(0,0,FindPlayerPed()->GetForward().Heading());
    car->SetStatus(STATUS_ABANDONED); car->m_nDoorLock = CARLOCK_UNLOCKED;
    car->bIsLocked = false;
    car->bNotDamagedUpsideDown = false;
    // HV carcols_additional.dat: stainless body and grey trim.
    car->m_currentColour1=79; car->m_currentColour2=71;
    // The stock Deluxo handling can mark all doors as missing.  The donor
    // DeLorean has real gullwing door frames and actuators, so make sure the
    // front door damage state starts usable for entry and animation.
    car->Damage.SetDoorStatus(DOOR_FRONT_LEFT, DOOR_STATUS_OK);
    car->Damage.SetDoorStatus(DOOR_FRONT_RIGHT, DOOR_STATUS_OK);
    // Keep reVC's native door controller; the donor animation mirrors it below.
    CWorld::Add(car); vehicleRef = CPools::GetVehicleRef(car);
    hookDoorReserved=false;
    frames.clear(); originals.clear();
    Remember(RpClumpGetFrame((RpClump*)car->m_rwObject), nil);
    cabinEmitterFrame=nil;
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,RememberCabinEmitter,nil);
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,BuildRaisedFrost,nil);
    InitialiseFrostShader();
    frostRenderers.clear();
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,AttachFrostShader,nil);
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,DonorGlowAtomic,nil);
    cabinDetailOriginals.clear();
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,AttachCabinDetail,nil);
    // Upload hidden frost/variant meshes before the first cinematic arrival.
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,[](RpAtomic *a,void*)->RpAtomic*{
        a->getPipeline()->instance(a);return a;
    },nil);
    hover = false; fuel=false; lowPower=true; circuits=false; dashboard=DonorSystems::Dashboard();
    ignitionCycle={}; cranking=false; crankUntil=0; startAttempts=0;
    stallDeadline=CTimer::GetTimeInMilliseconds()+5000+(rand()%15001);
    reactorGauges={};
    if(fuel && circuits){
        reactorGauges.power=23.0f;
        reactorGauges.geiger=45.0f;
    }
    arrival.next=3;
    departureTrail={};detachedPlate={};nextPlasmaParticle=0;sparkLoopActive=false;
    hoverAccelerating=false;hoverThrusterActive=false;
    emergencyLight={}; emergencyKeyHeld=false; leftWindowKeyHeld=false; rightWindowKeyHeld=false; keypadConfirmUntil=0;
    destinationConfirmActive=false;
    displayStartupActive=false;
    underbodyLights={}; cabinShifter={}; cabinWipers={}; cabinPedals={}; cabinSignals={}; cabinWindows={};
    timeCircuitShutter=0.0f; timeCircuitShutterPending=0.0;
    consoleClock.Reset(departedTime/100,departedTime%100);
    compassState={}; previousCompassVelocity=CVector(0,0,0); compassVelocityReady=false;
    doorSounds={};
    engineSounds={};
    for(int &level:sidLevels) level=0; sidPending=0;
    Variation();
    Help("DeLorean: C converts hover; M changes travel mode; Tab refuels; period changes variation.");
    char info[160]; snprintf(info,sizeof(info),"DeLorean spawned: pool=%d frames=%u",vehicleRef,(unsigned)frames.size()); Log(info);
}
void PlaceDebugPlayerAtAirport(){
    if(debugPlayerPlaced || !enabled || !FindPlayerPed() || FindPlayerVehicle())return;
    // Escobar International occupies the southwest island (negative X/Y).
    // This point is the middle of its long east-west runway.
    CVector p(-1290.231f,-1096.985f,14.868f);bool ground=false;
    const float z=CWorld::FindGroundZFor3DCoord(p.x,p.y,p.z+3.0f,&ground);
    if(ground)p.z=z+1.0f;
    FindPlayerPed()->SetPosition(p);
    FindPlayerPed()->SetOrientation(0,0,2.080991f);
    debugPlayerPlaced=true;
    Log("Debug player spawn moved to airport runway");
}
void CaptureDebugSpawnCoordinates(){
    if(!FindPlayerPed())return;
    const CVector p=FindPlayerPed()->GetPosition();
    const float heading=FindPlayerPed()->GetForward().Heading();
    char line[256];
    snprintf(line,sizeof(line),"player_spawn = CVector(%.3ff, %.3ff, %.3ff); heading = %.6ff;",p.x,p.y,p.z,heading);
    FILE *f=fopen((root+"/debug_spawn.txt").c_str(),"wb");
    if(f){fputs(line,f);fputc('\n',f);fclose(f);}
    Help("Debug coordinates saved to debug_spawn.txt");
    Log(line);
}
// Source hover.cpp equations adapted to reVC's matrices and handling names.
void Fly(CAutomobile *car,bool focused) {
    float step = Bound(CTimer::GetTimeStep(),0.0f,3.0f);
    CPad *pad = CPad::GetPad(0);
    const bool occupied=FindPlayerVehicle()==car;
    const bool controls=occupied && focused;
    float pedal = controls?(pad->GetAccelerate()-pad->GetBrake())/255.0f:0.0f;
    float fwd = DotProduct(car->GetMoveSpeed(),car->GetForward());
    float thrust = handling.Transmission.fEngineAcceleration * 5.0f;
    float falloff = fwd > 0 || pedal > 0 ? 0.5f/handling.Transmission.fMaxVelocity : -1.0f/handling.Transmission.fMaxReverseVelocity;
    float accel = (pedal-falloff*fwd)*thrust*(controls && pad->GetHandBrake() && pedal > 0.6f ? 0.8f : 0.3f);
    bool groundFound=false;
    const CVector position=car->GetPosition();
    const float groundZ=CWorld::FindGroundZFor3DCoord(position.x,position.y,position.z+3.0f,&groundFound);
    // Hover.txt converts unattended cars within five metres of the ground.
    // Continue the altitude controller until touchdown instead of dropping
    // lift immediately when the driver leaves.
    if(!occupied && !hoverTransition && hoverPivot>=89 && groundFound && position.z-groundZ<5.0f)
        autoLanding=true;
    // Hold the altitude at which flight mode finished converting.  Pressing C
    // again lowers that target toward the road and leaves the wheels deployed
    // until the car is close enough to touch down safely.
    if(autoLanding && groundFound)
        hoverTargetZ=Max(groundZ+car->GetHeightAboveRoad()+0.35f,hoverTargetZ-0.035f*step);
    float attitude = asinf(Bound(car->GetForward().z,-1,1));
    float heading = atan2f(car->GetForward().y,car->GetForward().x);
    CVector up(cosf(attitude+HALFPI)*cosf(heading),cosf(attitude+HALFPI)*sinf(heading),sinf(attitude+HALFPI));
    const float verticalSpeed=car->GetMoveSpeed().z;
    const float heightError=hoverTargetZ-position.z;
    const float holdCorrection=Bound(heightError*0.65f-verticalSpeed*7.0f,-0.65f,0.85f);
    float upForce = (1.0f - 5.0f*DotProduct(car->GetMoveSpeed(),up))*cosf(attitude);
    car->SetMoveSpeed(car->GetMoveSpeed() * powf(0.999f,step));
    car->ApplyMoveForce(up*(GRAVITY*upForce*car->m_fMass*step));
    car->ApplyMoveForce(CVector(0,0,1)*(GRAVITY*holdCorrection*car->m_fMass*step));
    car->ApplyMoveForce(car->GetForward()*(accel*car->m_fMass*step));
    float side = -DotProduct(car->GetMoveSpeed(),car->GetRight());
    car->ApplyMoveForce(car->GetRight()*(0.15f*side*fabsf(side)*car->m_fMass*step));
    float pitch = controls?pad->GetSteeringUpDown()/128.0f:0.0f;
    float roll = controls?-pad->GetSteeringLeftRight()/128.0f:0.0f;
    float yaw = controls?pad->GetCarGunLeftRight()/128.0f:0.0f;
    car->ApplyTurnForce(car->GetUp()*(pitch*0.0035f*car->m_fTurnMass*step), car->GetForward());
    car->ApplyTurnForce(car->GetUp()*(roll*0.0065f*car->m_fTurnMass*step), car->GetRight());
    car->ApplyTurnForce(car->GetRight()*(-0.001f*yaw*car->m_fTurnMass*step), -car->GetForward());
    const CVector turnSpeed = car->GetTurnSpeed() * powf(0.9f,step);
    car->SetTurnSpeed(turnSpeed.x, turnSpeed.y, turnSpeed.z);
    if(autoLanding && groundFound && position.z-groundZ <= car->GetHeightAboveRoad()+0.55f && fabsf(verticalSpeed)<0.08f) {
        autoLanding=false;
        hoverTransition=-1;
        Sound("delorean/hover_retract.wav");
        Log("Automatic touchdown reached; hover wheels retracting");
    }
}
bool DateValid(int date, int time) {
    int year=date/10000, month=(date/100)%100, day=date%100;
    if (year<1 || year>9999 || month<1 || month>12 || time<0 || time/100>23 || time%100>59) return false;
    int n=DonorSystems::DaysInMonth(year,month);
    return day>=1 && day<=n;
}
void DisplayDigits(const std::string &prefix, int value, int count) {
    // Model suffix 1 is the units wheel, suffix 2 the tens wheel, etc.
    for(int place=1; place<=count; ++place) {
        int selected=DonorSystems::DisplayDigit(value,place);
        for(int digit=0;digit<10;++digit) {
            std::string name=prefix+std::to_string(place*10+digit);
            if(circuits && digit==selected) hidden.erase(name); else hidden.insert(name);
        }
    }
}
void DisplayDate(const std::string &prefix,int date,int time) {
    for(int month=1;month<=12;++month) {
        std::string name=prefix+"month"+std::to_string(month);
        if(circuits && month==(date/100)%100) hidden.erase(name); else hidden.insert(name);
    }
    DisplayDigits(prefix+"day",date%100,2);
    DisplayDigits(prefix+"year",date/10000,4);
    DisplayDigits(prefix+"hour",(time/100)%12 ? (time/100)%12 : 12,2);
    DisplayDigits(prefix+"min",time%100,2);
    hidden.insert(prefix+"am"); hidden.insert(prefix+"pm");
    if(circuits) hidden.erase(prefix+(time<1200?"am":"pm"));
}
void UpdateDestinationConfirmation() {
    if(displayStartupActive){
        const unsigned age=CTimer::GetTimeInMilliseconds()-displayStartupStarted;
        DisplayDate("dt",destinationDate,destinationTime);
        DisplayDate("pt",presentDate,CClock::GetHours()*100+CClock::GetMinutes());
        DisplayDate("ltd",departedDate,departedTime);
        if(!circuits || age>=550)displayStartupActive=false;
        else {
            // On.txt uses the same section order as Flash.txt for all three
            // rows: blank, partial illumination, then fully powered digits.
            const unsigned mask=DonorSystems::DestinationBlankMask(age,variant==3);
            for(const char *row:{"dt","pt","ltd"})for(const auto &entry:frames){
                const std::string &n=entry.first;
                if(n.empty() || n.back()<'0' || n.back()>'9')continue;
                const std::string prefix=row;
                if(((mask&8) && n.find(prefix+"month")==0) ||
                   ((mask&4) && n.find(prefix+"day")==0) ||
                   ((mask&2) && n.find(prefix+"year")==0) ||
                   ((mask&1) && (n.find(prefix+"hour")==0 || n.find(prefix+"min")==0)))hidden.insert(n);
            }
        }
        ApplyVisibility();
    }
    if(!destinationConfirmActive || displayStartupActive)return;
    const unsigned elapsed=CTimer::GetTimeInMilliseconds()-destinationConfirmStarted;
    DisplayDate("dt",destinationDate,destinationTime);
    if(!circuits || elapsed>=550){destinationConfirmActive=false;ApplyVisibility();return;}
    const unsigned mask=DonorSystems::DestinationBlankMask(elapsed,variant==3);
    for(const auto &entry:frames){
        const std::string &n=entry.first;
        if(n.empty() || n.back()<'0' || n.back()>'9')continue;
        if(((mask&8) && n.find("dtmonth")==0) ||
           ((mask&4) && n.find("dtday")==0) ||
           ((mask&2) && n.find("dtyear")==0) ||
           ((mask&1) && (n.find("dthour")==0 || n.find("dtmin")==0)))hidden.insert(n);
    }
    ApplyVisibility();
}
void UpdateArrival(){
    if(CTimer::GetTimeInMilliseconds()<arrivalStarted)return;
    int step;
    while((step=arrival.Take(CTimer::GetTimeInMilliseconds()-arrivalStarted))>=0){
        if(const char *clip=arrival.Clip(step)){
            char voice[32];snprintf(voice,sizeof(voice),"reentry-boom-%d",step);
            if(DonorAudio::Play(clip,false,voice))DonorAudio::ConfigureWorld(voice,arrivalPosition.x,arrivalPosition.y,arrivalPosition.z,50);
        }
        // Donor particle 43, blue re-entry burst, without damaging explosions.
        for(int i=0;i<10;i++)CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_EXPLOSION_LARGE),arrivalPosition,CVector(0,0,0),nil,5.f,RwRGBA{20,50,255,255});
        CPad::GetPad(0)->StartShake(500,255);
        if(step==2){
            const char *voice="reentry-tail";
            if(DonorAudio::Play("delorean/reentry.wav",false,voice))DonorAudio::ConfigureWorld(voice,arrivalPosition.x,arrivalPosition.y,arrivalPosition.z,50);
        }
    }
}
struct HookDrop {
    int ref=-1;float age=0;CVector center,localCenter,drift;
    CQuaternion start,finish;
};
std::vector<HookDrop> hookDrops;
void UpdateHookDrops(float dt){
    for(auto it=hookDrops.begin();it!=hookDrops.end();){
        CObject *object=CPools::GetObjectPool()->GetAt(it->ref);
        if(!object || !object->m_rwObject){it=hookDrops.erase(it);continue;}
        it->age+=dt;
        const float t=Bound(it->age/.65f,0,1),smooth=t*t*(3-2*t);
        CQuaternion q=it->start;q*=1-smooth;CQuaternion end=it->finish;end*=smooth;q+=end;q.Normalise();
        RwMatrix matrix;RwMatrixSetIdentity(&matrix);q.Get(&matrix);
        CVector center=it->center+it->drift*Min(it->age,.8f);
        center.z-=4.905f*it->age*it->age;
        CVector offset=CVector(matrix.right)*it->localCenter.x+CVector(matrix.up)*it->localCenter.y+CVector(matrix.at)*it->localCenter.z;
        matrix.pos=center-CVector(offset);
        RpAtomic *atomic=(RpAtomic*)object->m_rwObject;auto geometry=atomic->geometry;
        float bottom=1.e10f;
        for(int i=0;i<geometry->numVertices;i++){
            RwV3d v;RwV3dTransformPoints(&v,&geometry->morphTargets[0].vertices[i],1,&matrix);
            bottom=Min(bottom,v.z);
        }
        bool found=false;float ground=CWorld::FindGroundZFor3DCoord(center.x,center.y,it->center.z+3,&found);
        const bool landed=found && bottom<ground+.025f;
        if(landed) {
            matrix.pos.z+=ground+.025f-bottom;
            // Settle onto the ground without the upright/edge-on pose that
            // the donor attachment can have while mounted on the rear.
            CVector yaw(matrix.at.x,matrix.at.y,0); if(yaw.Magnitude()<.01f) yaw=CVector(0,1,0); yaw.Normalise();
            matrix.at=yaw; matrix.up=CVector(0,0,1); matrix.right=CVector(yaw.y,-yaw.x,0);
        }
        CWorld::Remove(object);*RwFrameGetMatrix(RpAtomicGetFrame(atomic))=matrix;
        object->GetMatrix().Update();object->UpdateRwFrame();CWorld::Add(object);
        if(landed && t>=1)it=hookDrops.erase(it);else ++it;
    }
}
void DetachTravelHook(CAutomobile *car){
    if(variant!=1 || hookMode!=2)return;
    RpAtomic *source=nil;
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,[](RpAtomic *a,void *data)->RpAtomic*{
        const char *name=GetFrameNodeName(RpAtomicGetFrame(a));
        if(name && !strcmp(name,"hookbttf1"))*static_cast<RpAtomic**>(data)=a;
        return a;
    },&source);
    if(!source){Log("Hook detachment failed: donor hook atomic missing");return;}
    CObject *object=new CObject();if(!object)return;
    object->SetModelIndexNoCreate(MI_CAR_PANEL);object->RefModelInfo(Model);
    RpAtomic *copy=RpAtomicClone(source);RwFrame *frame=RwFrameCreate();
    *RwFrameGetMatrix(frame)=*RwFrameGetLTM(RpAtomicGetFrame(source));
    RpAtomicSetFrame(copy,frame);RpAtomicSetFlags(copy,rpATOMICRENDER);
    CVisibilityPlugins::SetAtomicRenderCallback(copy,nil);
    object->AttachToRwObject((RwObject*)copy);object->bDontStream=true;
    object->m_fMass=10;object->m_fTurnMass=25;object->m_fAirResistance=.99f;
    object->m_fElasticity=.1f;object->m_fBuoyancy=10*GRAVITY/.75f;
    object->ObjectCreatedBy=TEMP_OBJECT;CObject::nNoTempObjects++;
    // The generic car-panel collision shape does not fit this thin rod.
    // Control the drop against its real mesh bounds to avoid ground impulses.
    object->SetIsStatic(true);object->bUsesCollision=false;object->bAffectedByGravity=false;
    object->bIsPickup=false;object->bUseVehicleColours=true;
    object->m_colour1=car->m_currentColour1;object->m_colour2=car->m_currentColour2;
    object->m_nEndOfLifeTime=CTimer::GetTimeInMilliseconds()+20000;
    object->SetMoveSpeed(CVector(0,0,0));object->SetTurnSpeed(0,0,0);CWorld::Add(object);
    HookDrop drop;drop.ref=CPools::GetObjectRef(object);
    CVector lo(1.e10f,1.e10f,1.e10f),hi(-1.e10f,-1.e10f,-1.e10f);
    auto geometry=copy->geometry;
    for(int i=0;i<geometry->numVertices;i++){
        const auto &v=geometry->morphTargets[0].vertices[i];
        lo.x=Min(lo.x,v.x);lo.y=Min(lo.y,v.y);lo.z=Min(lo.z,v.z);
        hi.x=Max(hi.x,v.x);hi.y=Max(hi.y,v.y);hi.z=Max(hi.z,v.z);
    }
    drop.localCenter=(lo+hi)*.5f;
    RwV3d center;RwV3dTransformPoints(&center,&drop.localCenter,1,RwFrameGetMatrix(frame));drop.center=center;
    CVector forward=car->GetForward();forward.z=0;forward.Normalise();
    drop.drift=hookDeployed?forward*-3.0f:CVector(0,0,0);
    RwMatrix flat=*RwFrameGetMatrix(frame);
    // Let the hook leave with the exact angle it had on the car.  The old
    // code forced a horizontal quaternion, which made the rear hook flip in
    // mid-air before landing.  Gravity and mesh-ground clearance below now
    // provide the natural drop without inventing a rotation.
    drop.start.Set(*RwFrameGetMatrix(frame));drop.finish=drop.start;
    if(drop.start.x*drop.finish.x+drop.start.y*drop.finish.y+drop.start.z*drop.finish.z+drop.start.w*drop.finish.w<0)drop.finish=-drop.finish;
    hookDrops.push_back(drop);UpdateHookDrops(0);
    hookMode=1;hookDeployed=false;hookAnimating=false;hookMotion=0;
    Variation();Log("Hook detached with controlled mesh-ground drop; holder retained");
}
void BeginDepartureEffects(CAutomobile *car) {
    DetachTravelHook(car);
    const uint32 now=CTimer::GetTimeInMilliseconds();
    departureTrail.active=false;
    departureTrail.ignited=false;
    departureTrail.grounded=car->m_nWheelsOnGround>0;
    departureTrail.started=now;
    departureTrail.nextEmission=now;
    // The implosion is the trail origin. The two tracks then grow forward in
    // the stored departure heading, so re-entry happens at their far end.
    departureTrail.left=car->GetPosition()+car->GetRight()*.98f-car->GetUp()*.75f;
    departureTrail.right=car->GetPosition()-car->GetRight()*.98f-car->GetUp()*.75f;
    CVector direction=car->GetMoveSpeed();
    if(direction.Magnitude()>.001f)direction.Normalise();else direction=car->GetForward();
    // Fixed half-metre spacing keeps the forward growth independent of speed.
    departureTrail.delta=car->GetForward()*.5f;
    if(variant!=1 || hookMode!=0 || detachedPlate.detached || frames.find("plate")==frames.end())return;
    RwFrame *plate=frames["plate"];
    RpAtomic *source=nil;
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,[](RpAtomic *a,void *data)->RpAtomic*{
        const char *n=GetFrameNodeName(RpAtomicGetFrame(a));
        if(n && !strcmp(n,"plate"))*static_cast<RpAtomic**>(data)=a;return a;
    },&source);
    if(!source)return;
    CObject *object=new CObject();if(!object)return;
    object->SetModelIndexNoCreate(MI_CAR_PANEL);object->RefModelInfo(Model);
    RpAtomic *copy=RpAtomicClone(source);RwFrame *copyFrame=RwFrameCreate();
    *RwFrameGetMatrix(copyFrame)=*RwFrameGetLTM(plate);RpAtomicSetFrame(copy,copyFrame);
    RpAtomicSetFlags(copy,rpATOMICRENDER);CVisibilityPlugins::SetAtomicRenderCallback(copy,nil);
    object->AttachToRwObject((RwObject*)copy);object->bDontStream=true;
    object->SetIsStatic(true);object->bUsesCollision=false;object->bAffectedByGravity=false;
    object->ObjectCreatedBy=TEMP_OBJECT;CObject::nNoTempObjects++;
    object->m_nEndOfLifeTime=CTimer::GetTimeInMilliseconds()+20000;CWorld::Add(object);
    detachedPlate.objectRef=CPools::GetObjectRef(object);
    detachedPlate.detached=detachedPlate.active=true;
    hidden.insert("plate");ApplyVisibility();
    detachedPlate.landed=false;detachedPlate.started=now;
    detachedPlate.position=RwFrameGetLTM(plate)->pos;
    detachedPlate.velocity=car->GetMoveSpeed()*0.18f - car->GetForward()*0.18f + car->GetUp()*0.13f;
    detachedPlate.spin=car->GetForward().Heading()*57.29578f;
    detachedPlate.pitch=-65.0f;
    detachedPlate.spinSpeed=8.4f;
    detachedPlate.pitchSpeed=14.4f;
    SoundWorld("delorean/plate.wav","departure-plate",detachedPlate.position,5);
    Log("OUTATIME plate detached");
}
struct ImplosionState {
    bool active=false;
    uint32 started=0;
    CVector position=CVector(0,0,0);
} implosion;
void StartImplosion(const CVector &position) {
    implosion.active=true;implosion.started=CTimer::GetTimeInMilliseconds();implosion.position=position;
    // Departure is the hard audio cut in the donor sequence: no cold loop,
    // refuel, indicator, or other mod voice may carry into the implosion.
    DonorAudio::StopAll();
    CPad::GetPad(0)->StartShake(700,255);
    SoundWorld("delorean/timetravel.wav","departure-implosion",position,30);
    Log("Departure implosion started");
}
void UpdateImplosion() {
    if(!implosion.active)return;
    const uint32 elapsed=CTimer::GetTimeInMilliseconds()-implosion.started;
    if(elapsed>=21*90){implosion.active=false;return;}
    const float strength=Sin(float(elapsed)/(21*90)*3.14159265f);
    CPointLights::AddLight(CPointLights::LIGHT_POINT,implosion.position,CVector(0,0,0),
        4.5f,0.08f+.20f*strength,0.18f+.30f*strength,0.38f+.32f*strength,CPointLights::FOG_NONE,false);
}
void UpdateDepartureTrail() {
    if(!departureTrail.active)return;
    const uint32 now=CTimer::GetTimeInMilliseconds();
    const bool rain=CWeather::OldWeatherType==WEATHER_RAINY || CWeather::OldWeatherType==WEATHER_HURRICANE;
    const uint32 age=now-departureTrail.started;
    if(age>=2450 ||
       (now-departureTrail.started>=1000 && (!departureTrail.grounded || rain))) {
        departureTrail.active=false;return;
    }
    if(fireTrailShader)return;
    if(now<departureTrail.nextEmission)return;
    departureTrail.nextEmission=now+100;
    // The ignition front takes 650ms to traverse 25m. Each section burns
    // independently, so the origin dies down before the far end.
    const int formed=Min(50,int(age/13));
    CVector across=CrossProduct(departureTrail.delta,CVector(0,0,1));across.Normalise();
    for(int p=0;p<=formed;p+=1) {
        const float localAge=float(age-p*13);
        const float burn=Bound((1800.f-localAge)/850.f,0,1);
        if(burn<=0)continue;
        const CVector offset=departureTrail.delta*float(p);
        for(int lane=-1;lane<=1;lane++){
        const float flicker=.85f+float(rand()%31)/100.f;
        const CVector width=across*(lane*.11f+float(rand()%9-4)/100.f);
        const float size=.34f*burn*flicker;
        const RwRGBA color={255,uint8(170+rand()%45),75,uint8(235*burn)};
        CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_CARFLAME),departureTrail.left+offset+width,CVector(0,0,.002f),nil,size,color,0,0,0,400);
        CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_CARFLAME),departureTrail.right+offset+width,CVector(0,0,.002f),nil,size,color,0,0,0,400);
        }
    }
}
void UpdateDetachedPlate(CAutomobile *car) {
    if(!detachedPlate.active)return;
    CObject *object=CPools::GetObjectPool()->GetAt(detachedPlate.objectRef);
    if(!object){detachedPlate.active=false;return;}
    RwFrame *frame=RpAtomicGetFrame((RpAtomic*)object->m_rwObject);
    const uint32 now=CTimer::GetTimeInMilliseconds();
    const float step=Bound(CTimer::GetTimeStep(),0.0f,3.0f);
    if(!detachedPlate.landed) {
        detachedPlate.velocity.z-=GRAVITY*step;
        detachedPlate.velocity*=powf(.993f,step);
        detachedPlate.position+=detachedPlate.velocity*step;
        detachedPlate.spin+=detachedPlate.spinSpeed*step;
        bool found=false;
        const float ground=CWorld::FindGroundZFor3DCoord(detachedPlate.position.x,detachedPlate.position.y,detachedPlate.position.z+1.0f,&found);
        if(found && detachedPlate.position.z<=ground+.055f) {
            detachedPlate.position.z=ground+.055f;detachedPlate.velocity=CVector(0,0,0);
            detachedPlate.landed=true;detachedPlate.pitch=-90.0f;detachedPlate.spinSpeed=0.0f;detachedPlate.pitchSpeed=0.0f;
            SoundWorld("delorean/plate_fall.wav","departure-plate-fall",detachedPlate.position,5);
        } else {
            detachedPlate.pitch+=detachedPlate.pitchSpeed*step;
        }
    }
    CWorld::Remove(object);
    CMatrix matrix(RwFrameGetMatrix(frame));
    matrix.SetRotate(DEGTORAD(detachedPlate.pitch),0,DEGTORAD(detachedPlate.spin));
    matrix.GetPosition()=detachedPlate.position;matrix.UpdateRW();
    object->GetMatrix().Update();object->UpdateRwFrame();CWorld::Add(object);
    if(now-detachedPlate.started>=20000 || (detachedPlate.position-car->GetPosition()).Magnitude()>100.0f) {
        detachedPlate.active=false;hidden.insert("plate");ApplyVisibility();
    }
}
void BeginArrivalCarEffects(){
    coldStarted=CTimer::GetTimeInMilliseconds(); nextColdParticle=coldStarted;
    coldDelay=50; coldEmissions=0;
    DonorAudio::Stop("delorean/cold.wav");
    SoundLoop("delorean/cold.wav");
    nextReactorSteam=coldStarted+10000; reactorVentPlayed=false;
    emptyFlashUntil=CTimer::GetTimeInMilliseconds()+2400;
    emptyAudioActive=(variant==1);emptyAudioPhase=-1;
}
void Travel(CAutomobile *car,bool departure=true,bool travelSound=true,uint32 arrivalDelay=0) {
    if(departure)BeginDepartureEffects(car);
    if(!cinematicTravel.active)BeginArrivalCarEffects();
    if(!cinematicTravel.active){
        departedDate=presentDate; departedTime=CClock::GetHours()*100+CClock::GetMinutes();
        consoleClock.Reset(departedTime/100,departedTime%100);
    }
    arrival.Begin(presentDate,destinationDate);arrivalStarted=CTimer::GetTimeInMilliseconds()+arrivalDelay;arrivalPosition=car->GetPosition();
    UpdateArrival();
    presentDate=destinationDate; CClock::SetGameClock(destinationTime/100,destinationTime%100);
    travelCalendar.Rebase(CClock::GetHours());
    fuel=false; lowPower=true; startAttempts=0;
    if(cinematicTravel.active){emptyFlashUntil=0;emptyAudioActive=false;emptyAudioPhase=-1;}
    stallDeadline=CTimer::GetTimeInMilliseconds()+5000+(rand()%15001);
    cooldown=CTimer::GetTimeInMilliseconds()+10000;
    flashUntil=cinematicTravel.active?0:CTimer::GetTimeInMilliseconds()+120;
    if (FindPlayerPed()) FindPlayerPed()->m_pWanted->SetWantedLevel(0);
    // Donor ExplosionSound uses the short instant mix for an occupied car and
    // the exterior mix for remote/cutscene travel.
    if(travelSound)Sound(FindPlayerVehicle()==car?"instant_timetravel.wav":"delorean/timetravel.wav");
    Help("Temporal displacement complete. Reactor empty; use Tab at the rear to refuel.");
    char msg[160]; snprintf(msg,sizeof(msg),"Time travel: %08d %04d -> %08d %04d",departedDate,departedTime,presentDate,destinationTime); Log(msg);
    Variation();
}
void BeginCinematicTravel(CAutomobile *car) {
    if(cinematicTravel.active)return;
    coldStarted=0;DonorAudio::Stop("delorean/cold.wav");
    departedDate=presentDate;departedTime=CClock::GetHours()*100+CClock::GetMinutes();
    consoleClock.Reset(departedTime/100,departedTime%100);
    cinematicTravel.active=true;cinematicTravel.started=CTimer::GetTimeInMilliseconds();
    cinematicTravel.vanished=cinematicTravel.fading=cinematicTravel.reentered=cinematicTravel.revealed=false;
    cinematicTravel.revealAt=0;
    cinematicTravel.matrix=car->GetMatrix();
    cinematicTravel.velocity=car->GetMoveSpeed();cinematicTravel.turn=car->GetTurnSpeed();
    cinematicTravel.savedFirstPerson=CCamera::bLeafFirstPerson;CCamera::bLeafFirstPerson=false;
    cinematicTravel.savedDriverVisible=FindPlayerPed()?FindPlayerPed()->bIsVisible:true;
    cinematicTravel.savedCollision=car->bUsesCollision;
    cinematicTravel.savedSpecialFov=TheCamera.m_bUseSpecialFovTrain;
    cinematicTravel.savedFovForTrain=TheCamera.m_fFovForTrain;
    TheCamera.m_bUseSpecialFovTrain=true;
    TheCamera.m_fFovForTrain=92.0f;
    const CVector camera=car->GetPosition()-car->GetForward()*44.0f+car->GetRight()*9.0f+car->GetUp()*7.0f;
    // Aim at the car so the camera retains both the disappearing car and the
    // fire trails that form behind its rear wheels.
    // The second argument is an up-vector offset, not a look-at target.
    // Passing the car position here rolled the fixed camera onto its side.
    TheCamera.SetCamPositionForFixedMode(camera,CVector(0,0,0));
    TheCamera.TakeControl(car,CCam::MODE_FIXED,JUMP_CUT,CAMCONTROL_SCRIPT);
    CPad::GetPad(0)->SetDisablePlayerControls(PLAYERCONTROL_CUTSCENE);
    BeginDepartureEffects(car);
    Help("Cinematic time-travel sequence.");Log("Cinematic time travel started");
}
void FinishCinematicTravel(CAutomobile *car) {
    BeginArrivalCarEffects();
    car->bIsVisible=true; car->bUsesCollision=cinematicTravel.savedCollision;
    if(FindPlayerPed())FindPlayerPed()->bIsVisible=cinematicTravel.savedDriverVisible;
    CPad::GetPad(0)->SetEnablePlayerControls(PLAYERCONTROL_CUTSCENE);
    TheCamera.Restore();
    TheCamera.m_bUseSpecialFovTrain=cinematicTravel.savedSpecialFov;
    TheCamera.m_fFovForTrain=cinematicTravel.savedFovForTrain;
    CCamera::bLeafFirstPerson=cinematicTravel.savedFirstPerson;
    car->SetMoveSpeed(cinematicTravel.velocity);
    car->SetTurnSpeed(cinematicTravel.turn.x,cinematicTravel.turn.y,cinematicTravel.turn.z);
    car->bEngineOn=true;
    stallDeadline=CTimer::GetTimeInMilliseconds()+5000+(rand()%15001);
    cinematicTravel.active=false;Log("Cinematic time travel complete");
}
void CancelCinematicTravel() {
    if(!cinematicTravel.active)return;
    CAutomobile *car=Car();
    if(car){car->bIsVisible=true;car->bUsesCollision=cinematicTravel.savedCollision;}
    if(FindPlayerPed())FindPlayerPed()->bIsVisible=cinematicTravel.savedDriverVisible;
    CPad::GetPad(0)->SetEnablePlayerControls(PLAYERCONTROL_CUTSCENE);
    TheCamera.RestoreWithJumpCut();
    TheCamera.m_bUseSpecialFovTrain=cinematicTravel.savedSpecialFov;
    TheCamera.m_fFovForTrain=cinematicTravel.savedFovForTrain;
    CCamera::bLeafFirstPerson=cinematicTravel.savedFirstPerson;
    cinematicTravel.active=false;
    Log("Cinematic time travel cancelled");
}
void UpdateCinematicTravel(CAutomobile *car) {
    if(!cinematicTravel.active)return;
    const uint32 elapsed=CTimer::GetTimeInMilliseconds()-cinematicTravel.started;
    car->GetMatrix()=cinematicTravel.matrix;car->GetMatrix().UpdateRW();car->UpdateRwFrame();
    car->SetMoveSpeed(CVector(0,0,0));car->SetTurnSpeed(0,0,0);
    // Keep the cinematic camera spatially tied to the stored departure pose.
    // The old sequence held one fixed point for its whole duration, which made
    // the implosion look flat and let the trail/wormhole drift toward the edge
    // of the frame on wide views.  A short eased dolly gives the departure a
    // readable scale while preserving the donor heading and fixed-mode control.
    if(!cinematicTravel.reentered){
        const float move=Bound(float(elapsed)/2200.0f,0.0f,1.0f);
        const float ease=move*move*(3.0f-2.0f*move);
        const CVector origin=cinematicTravel.matrix.GetPosition();
        const CVector camera=origin-cinematicTravel.matrix.GetForward()*(44.0f-7.0f*ease)
            +cinematicTravel.matrix.GetRight()*(9.0f-1.5f*ease)
            +cinematicTravel.matrix.GetUp()*(7.0f-1.5f*ease);
        TheCamera.SetCamPositionForFixedMode(camera,CVector(0,0,0));
        TheCamera.m_fFovForTrain=92.0f-8.0f*ease;
    }
    if(elapsed>=50 && !cinematicTravel.vanished){
        car->bIsVisible=false;car->bUsesCollision=false;cinematicTravel.vanished=true;wormholeFrame=0;
        StartImplosion(cinematicTravel.matrix.GetPosition());
    }
    if(cinematicTravel.vanished && !cinematicTravel.revealed){
        car->bIsVisible=false;
        if(FindPlayerPed())FindPlayerPed()->bIsVisible=false;
        if(elapsed>=550 && !departureTrail.ignited){
            // Let the implosion establish the departure point before the fire
            // trails ignite; they then grow away from the stored heading.
            departureTrail.active=true;
            departureTrail.ignited=true;
            departureTrail.started=CTimer::GetTimeInMilliseconds();
            departureTrail.nextEmission=departureTrail.started;
        }
        ApplyVisibility();
    }
    if(elapsed>=5000 && !cinematicTravel.fading){
        departureTrail.active=false;
        CParticle::RemovePSystem((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_CARFLAME));
        TheCamera.SetFadeColour(0,0,0);TheCamera.Fade(.75f,FADE_OUT);cinematicTravel.fading=true;
    }
    if(elapsed>=7750 && !cinematicTravel.reentered){
        cinematicTravel.matrix.GetPosition() += departureTrail.delta*50.0f;
        car->GetMatrix()=cinematicTravel.matrix;car->GetMatrix().UpdateRW();car->UpdateRwFrame();
        Travel(car,false,false,600);
        TheCamera.Fade(.5f,FADE_IN);
        cinematicTravel.reentered=true;
        // Reveal inside the third blue burst, after its first visible frame.
        cinematicTravel.revealAt=elapsed+600+arrival.offsets[2]+100;
    }
    if(cinematicTravel.reentered && cinematicTravel.revealAt && !cinematicTravel.revealed && elapsed>=cinematicTravel.revealAt){
        cinematicTravel.revealed=true;car->bIsVisible=true;
        if(FindPlayerPed())FindPlayerPed()->bIsVisible=cinematicTravel.savedDriverVisible;
        ApplyVisibility();
        cinematicTravel.velocity=car->GetForward()*(48.1f/GAME_SPEED_TO_METERS_PER_SECOND);
        FinishCinematicTravel(car);
    }
}
bool Initialise(const LeafHost *h) {
    if (!h || h->abiVersion!=LEAF_ABI_VERSION) return false;
    host=*h; root=h->packageDirectory;
    Log("Loading native DeLorean module.");
    model=(CVehicleModelInfo*)CModelInfo::GetModelInfo(Model);
    if (!model || model->GetNumRefs()) { Log("Deluxo already in use; model replacement deferred until a fresh game."); return false; }
    oldTxd=model->GetTxdSlot(); oldCol=model->GetColModel(); oldColOwn=model->DoesOwnColModel();
    oldWheelScale=model->m_wheelScale; strcpy(oldName,model->GetModelName()); memcpy(oldGameName,model->m_gameName,10);
    oldHandling=*mod_HandlingManager.GetHandlingData((tVehicleType)model->m_handlingId);
    handling={}; handling.nIdentifier=oldHandling.nIdentifier;
    // Never inherit the stock no-door flag: it makes CAutomobile mark every
    // door missing before the donor frames can be animated.
    handling.Flags = 0x4308187 & ~HANDLING_NO_DOORS;
    // HV handling_additional.cfg: 1850 kg, 2 x 4.4 x 1.3 m, rear drive, 300 maximum.
    handling.fMass=1850; handling.fInvMass=1.0f/1850; handling.Dimension=CVector(2,4.4f,1.3f);
    handling.CentreOfMass=CVector(0,0,-0.3f);
    handling.nPercentSubmerged=70;
    handling.fTractionMultiplier=0.85f; handling.fTractionLoss=0.75f; handling.fTractionBias=0.47f;
    handling.Transmission.nNumberOfGears=5; handling.Transmission.nDriveType='R'; handling.Transmission.nEngineType='P';
    handling.Transmission.Flags=handling.Flags;
    handling.Transmission.fMaxVelocity=300.0f;
    // Match the handling.cfg parser, then let the engine convert every derived field.
    handling.Transmission.fEngineAcceleration=30.0f*0.4f;
    handling.fBrakeDeceleration=11.0f; handling.fBrakeBias=0.65f; handling.bABS=0;
    handling.fSteeringLock=30.0f;
    handling.fSuspensionForceLevel=1.80f; handling.fSuspensionDampingLevel=0.19f;
    handling.fSeatOffsetDistance=0.37f; handling.fCollisionDamageMultiplier=0.72f;
    handling.nMonetaryValue=95000;
    handling.fSuspensionUpperLimit=0.08f; handling.fSuspensionLowerLimit=-0.02f;
    handling.fSuspensionBias=0.50f; handling.fSuspensionAntidiveMultiplier=0.40f;
    handling.FrontLights=1; handling.RearLights=1;
    mod_HandlingManager.ConvertDataToGameUnits(&handling);
    txd=CTxdStore::AddTxdSlot("leaf_delorean");
    Log("Reading DeLorean texture dictionary.");
    if(txd<0 || !CTxdStore::LoadTxd(txd,(root+"/assets/delorean.txd").c_str())) { Log("DeLorean TXD failed."); return false; }
    CTxdStore::AddRef(txd);
    // Keep the donor-only particle artwork isolated in this package.  Loading
    // it into its own dictionary validates the leaf asset without replacing
    // the game's shared particle.txd.
    particleTxd=CTxdStore::AddTxdSlot("leaf_delorean_particles");
    if(particleTxd<0 || !CTxdStore::LoadTxd(particleTxd,(root+"/particles_additional.txd").c_str())) {
        Log("Additional DeLorean particle TXD failed."); return false;
    }
    CTxdStore::AddRef(particleTxd);
    // The leaf stores frost with supplemental effects. Transfer its runtime
    // ownership to the vehicle dictionary so DFF material lookup finds it.
    RwTexture *frost=RwTexDictionaryFindNamedTexture(CTxdStore::GetSlot(particleTxd)->texDict,"frost");
    if(!frost){ Log("Additional frost texture missing"); return false; }

    RwTexDictionaryAddTexture(CTxdStore::GetSlot(txd)->texDict,frost);

    CTxdStore::PushCurrentTxd(); CTxdStore::SetCurrentTxd(particleTxd);
    for(int i=0;i<70;i++) {
        char name[24]; snprintf(name,sizeof(name),"wormhole%d",i+1);
        wormholeTextures[i]=RwTextureRead(name,nil);
        snprintf(name,sizeof(name),"wormholer%d",i+1);
        wormholeRedTextures[i]=RwTextureRead(name,nil);
    }
    CTxdStore::PopCurrentTxd();
    for(int i=0;i<21;i++) {
        char path[MAX_PATH];snprintf(path,sizeof(path),"%s/effects/implosion/%d.rgba",root.c_str(),i);
        implosionTextures[i]=LoadIceSurfaceTexture(path);
        if(implosionTextures[i]){
            implosionTextures[i]->setAddressU(rw::Texture::CLAMP);
            implosionTextures[i]->setAddressV(rw::Texture::CLAMP);
        }
    }
    // Reuse the stock radial light texture for feathered beam slices.
    InitialiseImplosionShader();
    InitialiseWormholeShader();
    InitialisePlasmaShader();
    InitialiseFireTrailShader();
    // Load the stock radial texture explicitly; the game may not have loaded
    // its shared particle dictionary when Leaf modules initialize.
    std::string gameRoot=root;
    size_t modsPos=gameRoot.find("/mods/"); if(modsPos==std::string::npos) modsPos=gameRoot.find("\\mods\\");
    if(modsPos!=std::string::npos) gameRoot.resize(modsPos);
    beamTxd=CTxdStore::AddTxdSlot("leaf_delorean_stock_particles");
    if(beamTxd>=0) { CTxdStore::LoadTxd(beamTxd,(gameRoot+"/models/particle.txd").c_str()); CTxdStore::AddRef(beamTxd); }
    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(beamTxd>=0?beamTxd:CTxdStore::FindTxdSlot("particle"));
    beamTexture=RwTextureRead("shad_exp",nil);
    CTxdStore::PopCurrentTxd();
    if(!wormholeTextures[0] || !wormholeTextures[69] || !wormholeRedTextures[0] || !wormholeRedTextures[69] || !implosionTextures[0] || !implosionTextures[20]) {
        Log("Additional DeLorean effect frames failed."); return false;
    }
    if(!mod_ParticleSystemManager.LoadAdditional((root+"/particles_additional.cfg").c_str())) { Log("Additional DeLorean particle CFG parse failed."); return false; }
    if(!LoadPresetTextures(particleTxd)){Log("Donor preset particle textures missing");return false;}
    Log("Additional DeLorean particle presets parsed into isolated slots and textures loaded.");
    Log("Texture dictionary loaded; replacing model metadata.");
    model->DeleteRwObject();
    model->SetTexDictionary("leaf_delorean"); model->SetModelName("delorean");
    strcpy(model->m_gameName,"DELUXO"); model->m_wheelScale=0.79f;
    *mod_HandlingManager.GetHandlingData((tVehicleType)model->m_handlingId)=handling;
    changedModel=true;
    FILE *f=fopen((root+"/assets/delorean.col").c_str(),"rb");
    if(!f) return false;
    unsigned char col[8192]; size_t n=fread(col,1,sizeof(col),f); fclose(f);
    if(n<40 || memcmp(col,"COLL",4)!=0) return false;
    CColModel *collision=new CColModel;
    CFileLoader::LoadCollisionModel(col+32,*collision,(char*)"delorean"); model->SetColModel(collision,true);
    LeafMods::RegisterPreservedVehicleModel(Model,true);
    Log("Collision loaded; reading DeLorean clump.");
    CTxdStore::PushCurrentTxd(); CTxdStore::SetCurrentTxd(txd);
    std::string file=root+"/assets/delorean.dff";
    RwStream *stream=RwStreamOpen(rwSTREAMFILENAME,rwSTREAMREAD,(void*)file.c_str());
    bool ok=stream && CFileLoader::LoadClumpFile(stream,Model);
    if(stream) RwStreamClose(stream,nil);
    CTxdStore::PopCurrentTxd();
    if(!ok) { Log("DeLorean DFF failed."); return false; }
    CStreaming::ms_aInfoForModel[Model].m_loadState=STREAMSTATE_LOADED;
    CStreaming::ms_aInfoForModel[Model].m_flags|=STREAMFLAGS_DONT_REMOVE|STREAMFLAGS_SCRIPTOWNED;
    if(!DonorAudio::Init(root+"/sounds")) Log("OpenAL DeLorean audio initialization failed");
    for(const char *clip:{"delorean/timetravel.wav","instant_timetravel.wav","delorean/cold.wav",
        "delorean/reentry.wav","delorean/reentry_short.wav","delorean/reentry_single.wav",
        "delorean/reentry_long.wav","delorean/plut_gauge.wav"})
        if(!DonorAudio::Preload(clip))Log("Travel audio preload failed");
    enabled=true;
    Log("DeLorean model and textures loaded; period spawns it.");
    return true;
}
void UpdatePlasmaAndSparks(CAutomobile *car,float speed,uint32 now) {
    const bool charged=circuits && now>=cooldown && coilAlpha>=225.0f;
    const bool sparks=charged && speed>=44.5f && (fuel || (!hover && car->m_nWheelsOnGround>0));
    UpdateTravelArcs(car,sparks,now);
    if(sparks) {
        if(!sparkLoopActive) {
            SoundAt("delorean/sparks.wav",0,0,0,20,true);
            sparkLoopActive=true;
        }
    } else if(sparkLoopActive) {
        DonorAudio::Stop("delorean/sparks.wav");sparkLoopActive=false;
    }
    // The dedicated world-pass shader draws the wheel plasma every frame.
    // Keep the legacy sprite path only as a shader-creation fallback.
    if(plasmaShader || hover || !charged || now<nextPlasmaParticle)return;
    nextPlasmaParticle=now+33;
    struct PlasmaPoint {float y,z,size;uint8 r,g,b,threshold;};
    static const PlasmaPoint rear[]={
        {-1.70f,-.35f,.15f,64,128,255,225},{-1.75f,-.40f,.15f,96,144,240,227},
        {-1.80f,-.45f,.15f,128,160,224,229},{-1.85f,-.50f,.15f,160,176,192,231},
        {-1.90f,-.55f,.15f,192,192,176,233},{-1.95f,-.55f,.15f,224,224,160,235},
        {-2.00f,-.55f,.15f,255,255,128,237},{-2.05f,-.56f,.14f,255,255,128,239},
        {-2.10f,-.57f,.13f,255,255,128,241},{-2.15f,-.58f,.12f,255,255,128,243},
        {-2.20f,-.59f,.11f,255,255,128,245},{-2.25f,-.60f,.10f,255,255,128,247}};
    static const PlasmaPoint front[]={
        {1.00f,-.45f,.15f,64,128,255,225},{.95f,-.475f,.15f,96,144,240,227},
        {.90f,-.50f,.15f,128,160,224,229},{.85f,-.525f,.15f,160,176,192,231},
        {.80f,-.55f,.15f,192,192,176,233},{.75f,-.55f,.15f,224,224,160,235},
        {.70f,-.55f,.15f,224,224,160,237},{.65f,-.55f,.15f,255,175,128,239},
        {.60f,-.55f,.15f,255,175,128,241},{.55f,-.55f,.15f,255,175,128,243},
        {.50f,-.55f,.15f,255,175,128,245},{.45f,-.56f,.14f,255,175,128,247},
        {.40f,-.57f,.13f,255,175,128,249},{.35f,-.58f,.12f,255,175,128,251},
        {.30f,-.59f,.11f,255,175,128,253},{.25f,-.60f,.10f,255,175,128,255}};
    auto emit=[&](const PlasmaPoint *points,size_t count){
        for(size_t i=0;i<count;++i)if(coilAlpha>=points[i].threshold)for(int side=-1;side<=1;side+=2){
            // reVC submits particles in the current frame. The donor CLEO
            // prediction offset compensated its delayed opcode and moves the
            // plasma ahead of the car when repeated here.
            CVector position=car->GetPosition()+car->GetRight()*(.80f*side)+car->GetForward()*points[i].y+car->GetUp()*points[i].z;
            CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_FLAME),position,CVector(0,0,0),nil,points[i].size,RwRGBA{points[i].r,points[i].g,points[i].b,255},0,0,0,10);
        }
    };
    emit(rear,sizeof(rear)/sizeof(rear[0]));emit(front,sizeof(front)/sizeof(front[0]));
}
void UpdateTemporalEffects(CAutomobile *car) {
    const uint32 now=CTimer::GetTimeInMilliseconds();
    const float speed=car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND;
    // HV Coils.txt fades by ten alpha units per script tick. Wormhole.txt
    // advances one image per tick and loops back ten at the speed/charge cap.
    wormholeTicks+=Bound(CTimer::GetTimeStep(),0.0f,3.0f);
    while(wormholeTicks>=1.0f) {
        wormholeTicks-=1.0f;
        previousWormholeFrame=wormholeFrame;
        const float target=circuits ? (now<cooldown ? (coldStarted && now-coldStarted<1000?255.0f:0.0f) : Bound((speed-40.5f)*64.0f,0,255)) : 0;
        coilAlpha+=Bound(target-coilAlpha,-10,10);
        const int maxFrame=circuits && fuel && now>=cooldown ?
            int(Min(Bound((speed-44.9f)*.5f,0,1),coilAlpha/255.0f)*70.0f) : 0;
        if(maxFrame==0 && wormholeFrame<=1) wormholeFrame=0;
        else {
            ++wormholeFrame;
            if(wormholeFrame>maxFrame) wormholeFrame-=10;
            wormholeFrame=(int)Bound(float(wormholeFrame),1,70);
        }
    }
    // Coils.txt: hoodbox variants choose each bank independently every 50 ms.
    // The donor maps random values 5->1 and 6->3, favouring those segments;
    // zero blanks a bank. Above 47 m/s all banks settle on segment four.
    const bool hoodFlicker=variant==3 && circuits && coilAlpha>0 && speed<=47.0f;
    if(hoodFlicker && now>=nextHoodCoilFlicker){
        nextHoodCoilFlicker=now+50;
        for(int &segment:hoodCoilSegments){
            segment=rand()%7;
            if(segment==5)segment=1;
            else if(segment==6)segment=3;
        }
    }
    int bank=0;
    for(const char *prefix:{"fluxcoilsonlb","fluxcoilsonrb","fluxcoilsonf"}) {
        const int selected=hoodFlicker?hoodCoilSegments[bank]:4;
        ++bank;
        for(int i=1;i<=5;i++) {
            const std::string name=std::string(prefix)+std::to_string(i);
            if(i==selected && coilAlpha>0) hidden.erase(name); else hidden.insert(name);
        }
    }
    if(coilAlpha>0) hidden.erase("fluxemitteron"); else hidden.insert("fluxemitteron");
    UpdatePlasmaAndSparks(car,speed,now);
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,CoilAtomic,nil);
    ApplyVisibility();
}
RpMaterial *SetEffectAlpha(RpMaterial *material,void *data){
    RwRGBA color=*RpMaterialGetColor(material);
    color.alpha=*(uint8*)data;
    RpMaterialSetColor(material,&color);
    return material;
}
RpAtomic *ReactorEffectAtomic(RpAtomic *atomic,void *){
    const char *name=GetFrameNodeName(RpAtomicGetFrame(atomic));
    if(!name)return atomic;
    int alpha=-1;
    if(strcmp(name,"fluxcapacitorlightson")==0) alpha=int(fluxCabinAlpha);
    else for(const auto &frost:FROSTED_COMPONENTS)if(frost==name){
        const uint32 age=coldStarted?(CTimer::GetTimeInMilliseconds()-coldStarted):49725;
        alpha=age>=49725?0:255-int(age/195);
        break;
    }
    if(alpha>=0){
        uint8 value=(uint8)alpha;
        RpGeometry *geometry=RpAtomicGetGeometry(atomic);
        RpGeometryForAllMaterials(geometry,SetEffectAlpha,&value);
        RpGeometrySetFlags(geometry,RpGeometryGetFlags(geometry)|rpGEOMETRYMODULATEMATERIALCOLOR);
    }
    return atomic;
}
void UpdateReactorEffects(CAutomobile *car){
    if(cinematicTravel.active)return;
    const uint32 now=CTimer::GetTimeInMilliseconds();
    const bool active=circuits && !hidden.count("fluxcapacitor");
    const bool charged=active && fuel && car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND>=44.5f;
    for(int i=1;i<=6;i++){
        const std::string name="flux"+std::to_string(i);
        if(active && (charged || i==int((now/100)%6+1))) hidden.erase(name);
        else hidden.insert(name);
    }
    if(active) SoundAt("delorean/flux_idle.wav",0,-.775f,.265f,1,true);
    else DonorAudio::Stop("delorean/flux_idle.wav");
    const float target=active?(charged?255.0f:60.0f):0;
    const float step=(charged?30.0f:15.0f)*30*Max(0.0f,CTimer::GetTimeStepInSeconds());
    fluxCabinAlpha+=Bound(target-fluxCabinAlpha,-step,step);
    if(fluxCabinAlpha>0)hidden.erase("fluxcapacitorlightson");
    else hidden.insert("fluxcapacitorlightson");
    const bool frosted=coldStarted && now-coldStarted<49725;
    if(frosted) Show(FROSTED_COMPONENTS);else Hide(FROSTED_COMPONENTS);
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,ReactorEffectAtomic,nil);
    ApplyVisibility();
}
void UpdateColdEffects(CAutomobile *car) {
    if(cinematicTravel.active)return;
    if(!coldStarted) return;
    const uint32 now=CTimer::GetTimeInMilliseconds();
    // Donor Steam.txt: plutonium version vents ten seconds after arrival
    // for five seconds, from both rear outlets, following vehicle velocity.
    if(variant==1 && now-coldStarted>=10000 && now-coldStarted<15000 && now>=nextReactorSteam) {
        if(!reactorVentPlayed) { Sound("delorean/vent.wav"); reactorVentPlayed=true; }
        const float speed=car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND/40.0f-.2f;
        const CVector velocity=car->GetForward()*speed;
        for(int side=-1;side<=1;side+=2) {
            const CVector pos=car->GetPosition()+car->GetRight()*(.4f*side)-car->GetForward()*2.3f+car->GetUp()*.5f;
            CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_DRY_ICE),pos,CVector(velocity.x,velocity.y,0),nil,.85f,RwRGBA{235,240,242,220});
        }
        nextReactorSteam=now+33;
    }
    if(now-coldStarted>52225) {
        DonorAudio::Stop("delorean/cold.wav");
        coldStarted=0;
        return;
    }
    if(now<nextColdParticle) return;
    // Cold.txt: two wisps per emission, plus three for the plutonium variant;
    // 255 frost steps at 195 ms, with a gradually increasing emission delay.
    const bool finalWisps=now-coldStarted>=49725;
    if(finalWisps && car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND>=10){
        nextColdParticle=now+500; return;
    }
    const bool fresh=now-coldStarted<6000;
    const int count=finalWisps?2:(variant==1?(fresh?7:5):(fresh?4:2));
    for(int i=0;i<count;i++) {
        const float x=-1.2f+2.4f*float(rand())/RAND_MAX;
        const float y=-2.5f+5.0f*float(rand())/RAND_MAX;
        const float z=.65f*float(rand())/RAND_MAX;
        CVector pos=car->GetPosition()+car->GetRight()*x+car->GetForward()*y+car->GetUp()*z;
        CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_DRY_ICE),pos,CVector(0,0,-.05f),nil,fresh?.38f:.26f,RwRGBA{180,190,195,140});
    }
    if(++coldEmissions%2==0) ++coldDelay;
    nextColdParticle=now+(now-coldStarted>=49725?500:coldDelay);
}
void UpdateDashboard(CAutomobile *car) {
    const bool occupied=FindPlayerVehicle()==car;
    const float gas=occupied?CPad::GetPad(0)->GetAccelerate()/255.0f:0;
    const float brake=occupied?CPad::GetPad(0)->GetBrake()/255.0f:0;
    dashboard.Update(CTimer::GetTimeStepInSeconds(),car->bEngineOn,occupied,car->m_nCurrentGear,gas,brake,car->bLightsOn);
    // The donor speedo increases clockwise from its zero mark.
    Rotate("speedoneedle",CVector(0,1,0),Min(240.0f,car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND*3.6f*1.42f));
    Rotate("steering_wheel",CVector(0,1,0),-car->m_fSteerAngle*57.29578f*12.0f);
    // The animated switch is the underscored donor frame; tcdswitch is only
    // the mesh label and rotating it moves the wrong dashboard piece.
    Rotate("tcdhandle",CVector(0,1,0),circuits?-30.0f:0.0f);
    if(circuits){hidden.erase("tcdswitchlighton");hidden.insert("tcdswitchlightoff");hidden.erase("tcdkeypadlightson");hidden.erase("fluxcapacitorlightson");}
    else {hidden.insert("tcdswitchlighton");hidden.erase("tcdswitchlightoff");hidden.insert("tcdkeypadlightson");hidden.insert("fluxcapacitorlightson");}
    SetRotation("voltsneedle",CVector(0,DEGTORAD(dashboard.volts),0));
    SetRotation("oilneedle",CVector(0,DEGTORAD(dashboard.oil),0));
    SetRotation("tempneedle",CVector(0,DEGTORAD(dashboard.temp),0));
    SetRotation("fuelneedle",CVector(0,DEGTORAD(dashboard.petrol),0));
    SetRotation("ignitionkey",CVector(DEGTORAD(dashboard.ignition),0,0));
    SetRotation("ignition",CVector(DEGTORAD(dashboard.ignition),0,0));
    auto light=[](const char *name,bool on){if(on)hidden.erase(name);else hidden.insert(name);};
    light("oillight",dashboard.oilLight); light("batterylight",dashboard.batteryLight);
    const uint32 warningNow=CTimer::GetTimeInMilliseconds();
    if(emptyAudioActive && !fuel && occupied && warningNow<emptyFlashUntil) {
        int phase=(int)((warningNow-(emptyFlashUntil-2400))/400);
        if(phase!=emptyAudioPhase && phase<6) { emptyAudioPhase=phase; if(phase==0) Sound("delorean/klaxon.wav"); }
    }
    const bool emptyFlash=!fuel && occupied && (warningNow>=emptyFlashUntil || ((warningNow/400)&1)==0);
    light("seatbeltlight",dashboard.seatbelt); light("fuellight",fuel && dashboard.fuelLight);
    // BTTF1 uses the timed three-flash warning after time travel. Mr. Fusion
    // variants keep the same empty indicator steadily lit while unfueled.
    const bool emptyIndicator = !fuel &&
        (UsesMrFusion() || (occupied && (warningNow>=emptyFlashUntil || emptyFlash)));
    light("pchamberemptylight",emptyIndicator);
    light("lowbeamslight",dashboard.lowBeams); light("lambdalight",car->m_fHealth<390);
    ApplyVisibility();
}
RpMaterial *UnderbodyMaterial(RpMaterial *material,void *) {
    RwRGBA color=*RpMaterialGetColor(material);
    color.alpha=(uint8)underbodyLights.alpha;
    RpMaterialSetColor(material,&color);
    return material;
}
RpAtomic *RememberCabinEmitter(RpAtomic *atomic,void *){
    const char *name=GetFrameNodeName(RpAtomicGetFrame(atomic));
    if(name && strcmp(name,"overheadconsoleelight")==0){
        cabinEmitterFrame=RpAtomicGetFrame(atomic);
        cabinEmitterCenter=atomic->geometry->morphTargets[0].calculateBoundingSphere().center;
    }
    return atomic;
}
bool CabinLightOrigin(CAutomobile *car,CVector &origin){
    if(!cabinEmitterFrame)return false;
    RwFrameUpdateObjects(cabinEmitterFrame);
    RwV3d world;
    rw::V3d::transformPoints(&world,&cabinEmitterCenter,1,RwFrameGetLTM(cabinEmitterFrame));
    origin=CVector(world)-car->GetUp()*.012f;
    return true;
}
void UpdateLightBeams(CAutomobile *car){
    if(!emergencyLight.on) return;
    CVector origin;
    if(!CabinLightOrigin(car,origin))return;
    CPointLights::AddLight(CPointLights::LIGHT_POINT,origin,CVector(0,0,0),
        1.35f,.75f,.62f,.40f,CPointLights::FOG_NONE,false);
}
void UpdateHookAnimation() {
    CAutomobile *car=Car();
    if(car){
        // Reserve only the passenger entry path while the hook obstructs it.
        const bool blocked=variant==1 && hookMode==2 && (!hookDeployed || hookAnimating);
        if(blocked){
            if(!(car->m_nGettingInFlags & CAR_DOOR_FLAG_RF)) hookDoorReserved=true;
            car->m_nGettingInFlags |= CAR_DOOR_FLAG_RF;
        }else if(hookDoorReserved){
            car->m_nGettingInFlags &= ~CAR_DOOR_FLAG_RF;
            hookDoorReserved=false;
        }
    }
    if(variant!=1 || hookMode!=2){ hookWindReady=false; return; }
    const float target=hookDeployed?1.0f:0.0f;
    // Donor sequence: open guide, lift, swing to rear, then insert.
    // Use elapsed game time so rendering frequency cannot accelerate it.
    const float step=Max(0.0f,CTimer::GetTimeStepInSeconds())*30.0f/44.0f;
    hookMotion += Bound(target-hookMotion,-step,step);
    const float tick=hookMotion*44.0f;
    const float guide=Bound(tick/6.0f,0,1);
    const float lift=Bound((tick-6.0f)/8.0f,0,1);
    const float swing=Bound((tick-14.0f)/25.0f,0,1);
    const float insert=Bound((tick-39.0f)/5.0f,0,1);
    SetLocalPosition("hookbttf1_",CVector(.922921f*(1-swing),-1.46645f+.09895f*swing,.422211f+.424213f*lift));
    SetRotation("hookbttf1_",CVector(DEGTORAD(178.7f+122.163157f*swing),DEGTORAD(-.04f*(1-swing)),DEGTORAD(-1.72f*(1-swing))));
    SetRotation("hookguidebttf1_",CVector(DEGTORAD(89.5f*(1-guide)),0,0));
    SetLocalPosition("hookbttf1",CVector(0,.25f*insert,0));
    hookAnimating=hookMotion!=target;
    const bool connected=hookDeployed && !hookAnimating;
    // Cosmetic elastic flex about the inserted hook base; no vehicle forces.
    const float dt=CTimer::GetTimeStepInSeconds();
    const CVector velocity=car->GetMoveSpeed()*GAME_SPEED_TO_METERS_PER_SECOND;
    if(!connected || !hookWindReady || dt>.1f){
        hookFlexPitch=hookFlexRoll=hookFlexPitchSpeed=hookFlexRollSpeed=0;
        hookPreviousVelocity=velocity;
        hookWindReady=connected;
    }
    if(connected && dt>0 && dt<=.1f){
        const CVector acceleration=(velocity-hookPreviousVelocity)/dt;
        const float forward=DotProduct(velocity,car->GetForward());
        const float lateral=DotProduct(velocity,car->GetRight());
        const float phase=CTimer::GetTimeInMilliseconds()*.001f;
        const float flutter=Min(1.0f,velocity.Magnitude()/35.0f);
        const float pitchTarget=Bound(-forward*Abs(forward)*.0015f-
            DotProduct(acceleration,car->GetForward())*.09f+(Sin(phase*6.2f)*1.4f+Sin(phase*10.7f)*.6f)*flutter,-4,4);
        const float rollTarget=Bound(-lateral*Abs(lateral)*.003f-
            DotProduct(acceleration,car->GetRight())*.12f+(Sin(phase*5.7f)*1.6f+Sin(phase*11.3f)*.5f)*flutter,-4,4);
        const int steps=Max(1,(int)ceilf(dt*120));
        const float h=dt/steps;
        for(int i=0;i<steps;i++){
            hookFlexPitchSpeed+=((pitchTarget-hookFlexPitch)*65-hookFlexPitchSpeed*7)*h;
            hookFlexRollSpeed+=((rollTarget-hookFlexRoll)*65-hookFlexRollSpeed*7)*h;
            hookFlexPitch=Bound(hookFlexPitch+hookFlexPitchSpeed*h,-6,6);
            hookFlexRoll=Bound(hookFlexRoll+hookFlexRollSpeed*h,-6,6);
        }
    }
    hookPreviousVelocity=velocity;
    // Rotate only the inserted child so the guide and cable socket stay fixed.
    SetRotation("hookbttf1",CVector(DEGTORAD(hookFlexPitch),DEGTORAD(hookFlexRoll),0));
    hidden.erase("hookbttf1");
    if(connected){
        hidden.erase("hookcablesonbttf1");
        hidden.insert("hookcablesoffbttf1");
        hidden.insert("hookcableplugbttf1");
    }else{
        hidden.insert("hookcablesonbttf1");
        hidden.erase("hookcablesoffbttf1");
        hidden.erase("hookcableplugbttf1");
    }
    ApplyVisibility();
}
RpAtomic *UnderbodyAtomic(RpAtomic *atomic,void *) {
    const char *name=GetFrameNodeName(RpAtomicGetFrame(atomic));
    if(name && strcmp(name,"bottomlights")==0){
        RpGeometry *geometry=RpAtomicGetGeometry(atomic);
        RpGeometryForAllMaterials(geometry,UnderbodyMaterial,nil);
        RpGeometrySetFlags(geometry,RpGeometryGetFlags(geometry)|rpGEOMETRYMODULATEMATERIALCOLOR);
    }
    return atomic;
}
// Donor Delorean/Include/SID.txt: ten independent speed indicator columns.
void UpdateSID(CAutomobile *car){
    const float speed=car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND;
    int targets[10]={};
    if(circuits && CTimer::GetTimeInMilliseconds()>=cooldown && !hidden.count("sid")){
        for(int c=0;c<10;c++){
            if(speed>=46.5f) targets[c]=20;
            else if(speed>=43.5f) targets[c]=c==2?13:c==7?10:c==9?18:20;
            else if(speed>=40.5f){
                const int pattern[]={19,10,6,10,0,12,10,0,20,10};
                targets[c]=pattern[c];
            }else{
                const float mph=speed*1.835f;
                targets[c]=(c==4 || c==7)?0:int(c==0?mph/5:c==2?mph/16:c==5?mph/8:c==8?mph/4:Min(10.0f,mph/3));
            }
        }
    }
    sidPending+=Min(.1f,Max(0.0f,CTimer::GetTimeStepInSeconds()));
    while(sidPending>=1.0/30){
        sidPending-=1.0/30;
        for(int c=0;c<10;c++) sidLevels[c]+=sidLevels[c]<targets[c]?1:sidLevels[c]>targets[c]?-1:0;
    }
    for(int c=1;c<=10;c++)for(int row=1;row<=20;row++){
        const int index=row*(c==10?100:10)+c;
        const std::string name="sidledsline"+std::to_string(index);
        if(row<=sidLevels[c-1])hidden.erase(name);else hidden.insert(name);
    }
    ApplyVisibility();
}
void UpdateCabin(CAutomobile *car,bool focused) {
    const double dt=CTimer::GetTimeStepInSeconds();
    const bool occupied=FindPlayerVehicle()==car;
    const uint32 keypadNow=CTimer::GetTimeInMilliseconds();
    auto held=[&](int key){return focused && occupied && !cinematicTravel.active && (GetAsyncKeyState(key)&0x8000)!=0;};
    // Donor Keypad.txt moves each numbered key while either binding is held.
    for(int i=0;i<10;++i){
        const std::string frame="tcdkeypadbutton"+std::to_string(i);
        SetLocalPosition(frame.c_str(),CVector(0,(held('0'+i)||held(VK_NUMPAD0+i))?.005f:0,0));
    }
    SetLocalPosition("tcdkeypadbuttonenter",CVector(0,(held(VK_SUBTRACT)||held(VK_OEM_MINUS))?.002f:0,0));
    if(circuits && keypadNow<keypadConfirmUntil) hidden.erase("tcdkeypadenterlighton");
    else hidden.insert("tcdkeypadenterlighton");
    timeCircuitShutterPending+=dt;
    while(timeCircuitShutterPending+1e-9>=1.0/30.0){
        timeCircuitShutterPending-=1.0/30.0;
        timeCircuitShutter+=Bound((circuits?-90.0f:0.0f)-timeCircuitShutter,-5.0f,5.0f);
    }
    for(const char *name:{"dtshutter","ptshutter","ltdshutter"})
        SetRotation(name,CVector(DEGTORAD(timeCircuitShutter),0,0));
    const double previousBlink=consoleClock.blinkTime;
    consoleClock.Update(dt);
    if(circuits && consoleClock.blinkTime<previousBlink)
        SoundAt("delorean/timecircuits/colon_beep.wav",0,.32f,.14f,1);
    for(const char *name:{"dtcolon","ptcolon","ltdcolon"}){
        if(circuits && consoleClock.Colon())hidden.erase(name);else hidden.insert(name);
    }

    CPad *pad=CPad::GetPad(0);
    const int gas=occupied && focused?pad->GetAccelerate():0;
    const int brake=occupied && focused?pad->GetBrake():0;
    const int digitalSpeed=DonorSystems::DigitalSpeed(car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND);
    for(int place=1;place<=2;place++)for(int digit=0;digit<10;digit++){
        const int selected=place==1?digitalSpeed%10:(digitalSpeed>=10?digitalSpeed/10:-1);
        const std::string name="digitalspeedodigit"+std::to_string(place*10+digit);
        if(!hidden.count("digitalspeedo") && digit==selected)hidden.erase(name);else hidden.insert(name);
    }
    hidden.insert("digitalspeedodigit20bttf3");
    // The onboard clock continues from the last departure through the jump.
    // The console clock uses suffix 1 for units and suffix 2 for tens.
    auto clockDigit=[&](const char *prefix,int place,int selected){
        for(int digit=0;digit<10;digit++){
            const std::string name=std::string(prefix)+std::to_string(place*10+digit);
            if(digit==selected)hidden.erase(name);else hidden.insert(name);
        }
    };
    const int clockHour=consoleClock.DisplayHour();
    clockDigit("consoleclockdigithour",1,clockHour%10);
    clockDigit("consoleclockdigithour",2,clockHour>=10?clockHour/10:-1);
    clockDigit("consoleclockdigitmin",1,consoleClock.minute%10);
    clockDigit("consoleclockdigitmin",2,consoleClock.minute/10);
    if(consoleClock.Colon())hidden.erase("consoleclockdigitcolon");else hidden.insert("consoleclockdigitcolon");
    SetRotation("hourhand",CVector(0,DEGTORAD(consoleClock.HourHand()),0));
    SetRotation("minutehand",CVector(0,DEGTORAD(consoleClock.MinuteHand()),0));
    const CVector velocity=car->GetMoveSpeed();
    const CVector accel=compassVelocityReady?(velocity-previousCompassVelocity)/Max(dt,.001f):CVector(0,0,0);
    const float speed=velocity.Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND;
    compassState.UpdateVehicle(dt,car->GetForward().Heading(),car->GetForward().z,car->GetRight().z,car->GetUp().z,
        Bound(-DotProduct(accel,car->GetForward())*.08f,-12,12),Bound(DotProduct(accel,car->GetRight())*.08f,-12,12),speed);
    previousCompassVelocity=velocity; compassVelocityReady=true;
    SetRotation("compass",CVector(DEGTORAD(compassState.pitch),DEGTORAD(compassState.roll),DEGTORAD(compassState.yaw)));
    SetLocalPosition("turnsignallever",CVector(occupied && focused && pad->GetHorn()?-.522f:-.532f,.378f,.079f));
    const bool emergencyPressed=held('P');
    const bool toggleEmergency=emergencyPressed && !emergencyKeyHeld;
    emergencyKeyHeld=emergencyPressed;
    emergencyLight.Update(dt,toggleEmergency);
    if(toggleEmergency)SoundAt("delorean/emergency.wav",.1f,-.55f,.55f,1);
    if(emergencyLight.on)hidden.erase("overheadconsoleelight");else hidden.insert("overheadconsoleelight");
    SetRotation("overheadconsoleelighth",CVector(DEGTORAD(emergencyLight.handle),0,0));
    underbodyLights.Update(dt,variant==2,hoverPivot>89);
    for(int i=1;i<=5;i++){
        const std::string name="chaserlights"+std::to_string(i);
        if(i==underbodyLights.chaser)hidden.erase(name);else hidden.insert(name);
    }
    if(variant==2 && underbodyLights.alpha>0)hidden.erase("bottomlights");
    else hidden.insert("bottomlights");
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,UnderbodyAtomic,nil);
    cabinShifter.Update(dt,car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND,
        car->m_nCurrentGear,hoverPivot>89,car->m_aWheelSpeed[CARWHEEL_REAR_RIGHT]);
    // Supplied DFF uses uppercase N/R; the donor lookup was case-insensitive.
    const char *gearFrames[]={"shifterN","shifterR","shifter1","shifter2","shifter3","shifter4","shifter5"};
    for(int i=0;i<7;i++){
        if(i==cabinShifter.gear+1)hidden.erase(gearFrames[i]);else hidden.insert(gearFrames[i]);
    }
    SetRotation("rpmneedle",CVector(0,DEGTORAD(cabinShifter.rpmNeedle),0));
    cabinWipers.Update(dt,car->bEngineOn,occupied,held('U'),held('I'));
    static float lastWiperStalk=999;
    static int lastWiperMode=-1;
    if(lastWiperStalk!=cabinWipers.stalk || lastWiperMode!=cabinWipers.mode){
        if(focused && occupied) Help(cabinWipers.mode==1?"Wipers: intermittent. U parks blades.":
            cabinWipers.stalk==-10?"Wipers: fast. U parks blades.":
            cabinWipers.stalk==-5?"Wipers: normal. U selects fast.":
            cabinWipers.stalk==5?"Wipers: single sweep; hold I for intermittent.":"Wipers: off / parking.");
        lastWiperStalk=cabinWipers.stalk;lastWiperMode=cabinWipers.mode;
    }
    SetRotation("wiperslever",CVector(0,DEGTORAD(cabinWipers.lever),0));
    SetRotation("wiperdriver",CVector(0,DEGTORAD(cabinWipers.angle/5.5f),DEGTORAD(cabinWipers.angle)));
    SetRotation("wiperpassenger",CVector(0,DEGTORAD(cabinWipers.angle/12.5f),DEGTORAD(cabinWipers.angle)));
    if(cabinWipers.upSound && !DonorAudio::IsPlaying("delorean/wiper_up.wav")) SoundAt("delorean/wiper_up.wav",0,.88f,.24f,2);
    if(cabinWipers.downSound && !DonorAudio::IsPlaying("delorean/wiper_down.wav")) SoundAt("delorean/wiper_down.wav",0,.88f,.24f,2);

    cabinPedals.Update(dt,occupied,car->m_nCurrentGear,hoverPivot>89,gas/255.0f,brake/255.0f,focused && pad->GetHandBrake());
    SetRotation("gaspedal",CVector(DEGTORAD(cabinPedals.gas),0,0));
    SetRotation("brakepedal",CVector(DEGTORAD(cabinPedals.brake),0,0));
    SetRotation("clutchpedal",CVector(DEGTORAD(cabinPedals.clutch),0,0));
    // Log input/pose changes at most once a second for user-driven diagnosis.
    static uint32 nextPedalLog=0;
    static int previousPedalInput=-1;
    const int pedalInput=(gas>0?1:0)|(brake>0?2:0);
    if(occupied && focused && (pedalInput || pedalInput!=previousPedalInput) && CTimer::GetTimeInMilliseconds()>=nextPedalLog){
        char msg[240];
        snprintf(msg,sizeof(msg),"Cabin pedals: input=%d/%d gear=%d angles=%.1f/%.1f/%.1f frames=%d/%d/%d actualGasX=%.3f",
            gas,brake,car->m_nCurrentGear,cabinPedals.gas,cabinPedals.brake,cabinPedals.clutch,
            int(frames.count("gaspedal")),int(frames.count("brakepedal")),int(frames.count("clutchpedal")),RotationOf("gaspedal").x);
        Log(msg);previousPedalInput=pedalInput;nextPedalLog=CTimer::GetTimeInMilliseconds()+1000;
    }
    SetRotation("ebrake",CVector(DEGTORAD(cabinPedals.handbrake),0,0));
    auto light=[](const char *name,bool on){if(on)hidden.erase(name);else hidden.insert(name);};
    light("brakelight",cabinPedals.handbrake>1);
    // Donor light components: R=rear running, S=stop, RV=reverse.
    // Use processed drivetrain pedals so backing up is not mistaken for braking.
    const bool running=car->bLightsOn;
    const bool stopping=car->m_fBrakePedal>0.05f;
    const bool reversing=car->bEngineOn && car->m_fGasPedal<0.0f;
    light("lightRl",running); light("lightRr",running);
    light("lightSl",stopping); light("lightSr",stopping);
    light("lightRVl",reversing); light("lightRVr",reversing);

    light("doorajarlight",Abs(car->Doors[DOOR_FRONT_LEFT].m_fAngle)>.01f || Abs(car->Doors[DOOR_FRONT_RIGHT].m_fAngle)>.01f);

    cabinSignals.UpdateControls(dt,occupied,held('L'),held(VK_LSHIFT),held(VK_RSHIFT));
    const bool left=cabinSignals.mode==1 || cabinSignals.mode==3;
    const bool right=cabinSignals.mode==2 || cabinSignals.mode==3;
    light("turnlightlf",left?cabinSignals.blink:car->bLightsOn);
    light("turnlightrf",right?cabinSignals.blink:car->bLightsOn);
    light("turnlightlb",left && cabinSignals.blink); light("turnlightrb",right && cabinSignals.blink);
    light("turnsignalllight",left && cabinSignals.blink); light("turnsignalrlight",right && cabinSignals.blink);
    SetRotation("turnsignallever",CVector(0,DEGTORAD(cabinSignals.lever),0));
    SetLocalPosition("hazardsw",CVector(0,cabinSignals.HazardButton(),0));
    if(cabinSignals.onSound) SoundAt("delorean/turn_signal_on.wav",0,.32f,.14f,1);
    if(cabinSignals.offSound) SoundAt("delorean/turn_signal_off.wav",0,.32f,.14f,1);

    const bool leftWindowPressed=held('J');
    const bool rightWindowPressed=held('K');
    if(leftWindowPressed && !leftWindowKeyHeld && !DonorAudio::IsPlaying("delorean/window_up_left.wav") && !DonorAudio::IsPlaying("delorean/window_down_left.wav")) {
        cabinWindows.leftDown=!cabinWindows.leftDown;
        SoundAt(cabinWindows.leftDown?"delorean/window_down_left.wav":"delorean/window_up_left.wav",-.9f,-.2f,.25f,1);
    }
    if(rightWindowPressed && !rightWindowKeyHeld && !DonorAudio::IsPlaying("delorean/window_up_right.wav") && !DonorAudio::IsPlaying("delorean/window_down_right.wav")) {
        cabinWindows.rightDown=!cabinWindows.rightDown;
        SoundAt(cabinWindows.rightDown?"delorean/window_down_right.wav":"delorean/window_up_right.wav",.9f,-.2f,.25f,1);
    }
    leftWindowKeyHeld=leftWindowPressed;
    rightWindowKeyHeld=rightWindowPressed;
    cabinWindows.Update(dt);
    const float l=cabinWindows.left,r=cabinWindows.right;
    SetLocalPosition("door_lf_hi_window_",CVector(l*.5f-.7346f,l-.344396f,l-.256058f));
    SetRotation("door_lf_hi_window_",CVector(0,DEGTORAD(l*5),DEGTORAD(l*(-200.0f/3))));
    SetLocalPosition("door_rf_hi_window_",CVector(r*-.5f+.7346f,r-.344396f,r-.256058f));
    SetRotation("door_rf_hi_window_",CVector(0,DEGTORAD(r*-5),DEGTORAD(r*(200.0f/3))));
    SetRotation("wcontrollf",CVector(DEGTORAD(cabinWindows.leftSwitch),0,0));
    SetRotation("wcontrolrf",CVector(DEGTORAD(cabinWindows.rightSwitch),0,0));

    // The physical plutonium chamber needle is present in both donor variants.
    // Keep it driven by the loaded plutonium state; the BTTF1-only empty warning
    // remains separately gated in UpdateDashboard.
    reactorGauges.Update(dt,circuits,fuel,true);
    SetRotation("pchamberrefneedle",CVector(0,DEGTORAD(15),0));
    SetRotation("pchamberneedle",CVector(0,DEGTORAD(reactorGauges.Needle()),0));
    SetRotation("ppowerneedle",CVector(0,DEGTORAD(reactorGauges.power),0));
    SetRotation("primaryneedle",CVector(0,DEGTORAD(reactorGauges.power),0));
    light("gloveboxgaugeslights",reactorGauges.lights);
    if(reactorGauges.startupSound && variant==1) SoundAt("delorean/plut_gauge.wav",.45f,.24f,.2f,2);

    const bool folded=hoverPivot>89;
    const bool grip=car->m_aWheelState[CARWHEEL_REAR_LEFT]==WHEEL_STATE_NORMAL && car->m_aWheelState[CARWHEEL_REAR_RIGHT]==WHEEL_STATE_NORMAL;
    if(cinematicTravel.active) {
        DonorAudio::Stop("delorean/engine_idle.wav");
        DonorAudio::Stop("delorean/engine_accelerate.wav");
        DonorAudio::Stop("delorean/landspeeder_loop_lower_pitch.wav");
        engineSounds={};
    } else engineSounds.Update(dt,car->bEngineOn,occupied,gas,brake,car->m_nCurrentGear,folded,car->m_nWheelsOnGround>0,grip);
    if(engineSounds.start) SoundAt("delorean/engine_start.wav",0,-2,0,10);
    if(engineSounds.stop) {
        SoundAt("delorean/engine_stop.wav",0,-2,0,10);
        if(folded) SoundAt("delorean/landspeeder_decelerate_lower_pitch.wav",0,-2,0,10,false,.5f);
    }
    if(engineSounds.accelerate) SoundAt("delorean/engine_accelerate.wav",0,-2,0,20);
    if(engineSounds.decelerate) SoundAt("delorean/engine_decelerate.wav",0,-2,0,20);
    if(engineSounds.stopAcceleration) DonorAudio::Stop("delorean/engine_accelerate.wav");
    if(!cinematicTravel.active && car->bEngineOn && folded) SoundAt("delorean/engine_idle.wav",0,-2,0,20,true);
    else DonorAudio::Stop("delorean/engine_idle.wav");
    if(engineSounds.hoverGain>0) SoundAt("delorean/landspeeder_loop_lower_pitch.wav",0,-2,0,20,true,engineSounds.hoverGain);
    else DonorAudio::Stop("delorean/landspeeder_loop_lower_pitch.wav");
    ApplyVisibility();
}
void Update() {
    PlaceDebugPlayerAtAirport();
    if(!enabled) return;
    CAutomobile *audioCar=Car();
    const CVector source=audioCar?audioCar->GetPosition():TheCamera.GetPosition();
    const bool paused=FrontEndMenuManager.m_bMenuActive || CTimer::GetIsPaused();
    DonorAudio::Update(&TheCamera.GetPosition().x,&TheCamera.GetForward().x,&TheCamera.GetUp().x,&source.x,paused,Bound(FrontEndMenuManager.m_PrefsSfxVolume/64.0f,0,1),
        audioCar?&audioCar->GetRight().x:nil,audioCar?&audioCar->GetForward().x:nil,audioCar?&audioCar->GetUp().x:nil);
    if(!FindPlayerPed()) {CancelRefuel();return;}
    if(paused) return;
    presentDate=travelCalendar.Update(presentDate,CClock::GetHours());
    UpdateHookDrops(CTimer::GetTimeStepInSeconds());
    DWORD pid=0; GetWindowThreadProcessId(GetForegroundWindow(),&pid);
    const bool focused=pid==GetCurrentProcessId();
    if(focused && Press(VK_OEM_4)) travelHudVisible=!travelHudVisible;
    const bool variationPressed=focused && Press(VK_OEM_PERIOD);
    if(variationPressed && !FindPlayerVehicle()) Spawn();
    CAutomobile *car=Car(); if(!car || car->GetStatus()==STATUS_WRECKED) {
        CancelCinematicTravel(); CancelRefuel(); DonorAudio::StopAll();
        circuits=false; reactorGauges.lights=false; reactorGauges.startupSound=false;
        for(const auto &entry:frames) {
            const std::string &n=entry.first;
            if(n.find("light")!=std::string::npos || n.find("glow")!=std::string::npos ||
               n.find("flux")!=std::string::npos || n.find("coil")!=std::string::npos ||
               n.find("led")!=std::string::npos || n.find("digit")!=std::string::npos ||
               n.find("dt")==0 || n.find("pt")==0 || n.find("ltd")==0) hidden.insert(n);
        }
        LeafMods::SetRailWheelVehicle(nil);
        ApplyVisibility(); vehicleRef=-1; return;
    }
    UpdateCinematicTravel(car);
    // Donor low-power ignition: after travel the reactor can stall at a
    // variable time; throttle presses make repeated restart attempts.
    const uint32 powerNow=CTimer::GetTimeInMilliseconds();
    if(lowPower && !refueling && !cinematicTravel.active && powerNow>=stallDeadline && car->bEngineOn) {
        car->bEngineOn=false; startAttempts=0;
        ignitionCycle={};
        emptyFlashUntil=0; emptyAudioPhase=-1; emptyAudioActive=false;
        Help("Reactor power is low. Hold throttle to restart.");
    }
    const bool occupied=FindPlayerVehicle()==car;
    const bool exiting=occupied && focused && CPad::GetPad(0)->GetExitVehicle();
    // Donor Ignition.txt shuts down on the mapped exit control. Keep the
    // native exit/door state machine in charge of actually leaving the car.
    if(exiting && !cinematicTravel.active && !hover && !refueling)car->bEngineOn=false;
    const bool throttleStart=focused && occupied && !exiting && !refueling &&
        !cinematicTravel.active && !car->bEngineOn && CPad::GetPad(0)->GetAccelerate()>=150;
    const double ignitionDt=Min(.1f,Max(0.0f,CTimer::GetTimeStepInSeconds()));
    // Only draw randomness at a completed turnover, never once per frame.
    const bool attemptDue=throttleStart && ignitionCycle.elapsed+ignitionDt+1e-9>=.2;
    const bool started=ignitionCycle.Update(ignitionDt,throttleStart,lowPower,attemptDue?rand():1);
    if(started){
        car->bEngineOn=true;
        stallDeadline=powerNow+7000+(rand()%12001);
        // UpdateCabin owns the single engine_start sound on this transition.
    }
    cranking=throttleStart && !started;
    if(cranking)SoundLoop("delorean/engine_turnover.wav");
    else DonorAudio::Stop("delorean/engine_turnover.wav");
    // Some VC damage/streaming paths reapply the model's door state.  The
    // donor always has intact front gullwings, so repair only an erroneous
    // missing flag while leaving ordinary open/damaged states alone.
    handling.Flags &= ~HANDLING_NO_DOORS;
    mod_HandlingManager.GetHandlingData((tVehicleType)model->m_handlingId)->Flags &= ~HANDLING_NO_DOORS;
    if(car->Damage.GetDoorStatus(DOOR_FRONT_LEFT)==DOOR_STATUS_MISSING) car->Damage.SetDoorStatus(DOOR_FRONT_LEFT,DOOR_STATUS_OK);
    if(car->Damage.GetDoorStatus(DOOR_FRONT_RIGHT)==DOOR_STATUS_MISSING) car->Damage.SetDoorStatus(DOOR_FRONT_RIGHT,DOOR_STATUS_OK);
    if(variant==3 && hookMode==2 && !cinematicTravel.active && car->GetMoveSpeed().Magnitude()>.04f){
        for(int i=0;i<4;i++)if(car->m_aSuspensionSpringRatio[i]<1.0f){
            CParticle::AddParticle((tParticleType)(PARTICLE_LEAF_FIRST+PARTICLE_SPARK_SMALL),car->m_aWheelColPoints[i].point,
                car->GetMoveSpeed()*-.2f+CVector(0,0,.025f),nil,.012f,RwRGBA{255,200,100,255},0,0,0,120);
        }
    }
    UpdateDoors(car); UpdatePanels(car); UpdateWheels(car); UpdateHoverConversion(car); HoverEffects(car,focused); UpdateRefuel();
    UpdateColdEffects(car);
    UpdateArrival();
    UpdateImplosion();
    UpdateDepartureTrail();
    UpdateDetachedPlate(car);
    UpdateTemporalEffects(car);
    UpdateHoodboxEffects(car);
    UpdateDashboard(car);
    UpdateCabin(car,focused);
    // Dashboard animation runs first; the held starting-key pose wins while
    // cranking and naturally restores from the dashboard after release.
    if(cranking){
        SetRotation("ignitionkey",CVector(DEGTORAD(135),0,0));
        SetRotation("ignition",CVector(DEGTORAD(135),0,0));
    } else SetRotation("ignition",CVector(DEGTORAD(dashboard.ignition),0,0));
    UpdateSID(car);
    UpdateReactorEffects(car);
    UpdateHookAnimation();
    // Physics must continue while the window is unfocused; only keyboard
    // interactions are suppressed in the background.
    if(hover && !cinematicTravel.active) Fly(car,focused);
    if(!focused || cinematicTravel.active) {
        // Consume key edges while input is unavailable. Holding a number,
        // plus or minus through arrival must not queue a cockpit action.
        for(int key=0;key<256;++key)keys[key]=(GetAsyncKeyState(key)&0x8000)!=0;
        UpdateDestinationConfirmation(); return;
    }
    // Original HV interaction: Tab at the rear opens/refuels the reactor.
    if(Press(VK_TAB)) {
        const CVector relative=FindPlayerCoors()-car->GetPosition();
        const float localX=DotProduct(relative,car->GetRight());
        const float localY=DotProduct(relative,car->GetForward());
        const bool nearRear=!FindPlayerPed()->bInVehicle && localY < -2.35f && Abs(localX)<1.15f && Abs(DotProduct(relative,car->GetUp()))<2.0f;
        const bool nearSide=!FindPlayerPed()->bInVehicle && Abs(localX)>0.75f && Abs(localX)<3.0f && Abs(localY)<2.5f && Abs(DotProduct(relative,car->GetUp()))<2.25f;
        const bool hookAction = (hookMode==2 && !hookDeployed && nearSide && localX>0.0f) ||
            (hookMode==2 && hookDeployed && nearRear);
        if(variant==1 && hookAction && !hookAnimating && !refueling && !FindPlayerPed()->bInVehicle) {
            hookDeployed=!hookDeployed;
            Variation();
            hookAnimating=true;
            Help(hookDeployed?"Hook deployed.":"Hook stored.");
            Log(hookDeployed?"Hook deployed":"Hook stored");
        } else if(nearRear) BeginRefuel(car);
        else {
            Help("Stand at the side or rear of the DeLorean.");
        }
    }
    if(FindPlayerVehicle()!=car) return;
    if(Press('M') && !cinematicTravel.active) {
        instantTravelMode=!instantTravelMode;
        Help(instantTravelMode?"Mode: Instant Time Travel Sequence":"Mode: Cinematic Time Travel Sequence");
        Log(instantTravelMode?"Time travel mode: instant":"Time travel mode: cinematic");
    }
    if(cinematicTravel.active)return;
    // Donor TimeCircuits/Keypad.s: NumPad + toggles the time-circuit power
    // switch.  The switch is independent from the destination keypad and
    // immediately gates the display, flux animation, and time-travel trigger.
    if(Press(VK_ADD) || Press(VK_OEM_PLUS)) {
        circuits=!circuits;
        displayStartupActive=true;
        displayStartupStarted=CTimer::GetTimeInMilliseconds();
        destinationConfirmActive=false;
        Sound(circuits ? "delorean/timecircuits/on.wav" : "delorean/timecircuits/off.wav");
        Help(circuits ? "Time circuits ON." : "Time circuits OFF.");
        Log(circuits ? "Time circuits enabled" : "Time circuits disabled");
    }
    if(variationPressed) {
        if(refueling || hookAnimating || hover || hoverTransition || car->GetMoveSpeed().Magnitude()>=0.02f)
            Help("Land and stop before switching DeLorean variants.");
        else {
            if(frames.count("plate") && originals.count("plate")) {
                *RwFrameGetMatrix(frames["plate"])=originals["plate"];
                RwFrameUpdateObjects(frames["plate"]);
            }
            detachedPlate={};departureTrail={};
            if(variant==2) { variant=1; hookMode=0; }
            else if(variant==1 && hookMode==0) hookMode=1;
            else if(variant==1 && hookMode==1) hookMode=2;
            else if(variant==1) { variant=3; hookMode=0; }
            else if(variant==3 && hookMode==0) hookMode=1;
            else if(variant==3 && hookMode==1) hookMode=2;
            else { variant=2; hookMode=0; }
            hookDeployed=false; hookMotion=0; hookAnimating=false;
            reactorGauges={};
            if(fuel && circuits){
                reactorGauges.power=23.0f;
                reactorGauges.geiger=45.0f;
            }
            autoLanding=false; boostActive=false; hoverExtension=0; hoverPivot=0;
            wormholeFrame=0; wormholeTicks=0; coilAlpha=0;
            DonorAudio::Stop("delorean/sparks.wav");sparkLoopActive=false;
            ApplyHoverModel(); Variation();
            const char *label=variant==2?"BTTF II / Mr. Fusion":
                (variant==3?(hookMode==2?"BTTF III / railroad firebox":(hookMode==1?"BTTF III / horse hook":"BTTF III / whitewalls")):
                (hookMode==1?"BTTF I / holder only":(hookMode==2?"BTTF I / side hook":"BTTF I / plutonium reactor")));
            Help(label); Log(label);
        }
    }
    if(Press('C')) {
        if(variant==2) StartHoverConversion(car);
        else {
            consoleClock.Reset(departedTime/100,departedTime%100);
            Help("Console clock reset.");
            Log("Console clock reset to last departure time");
        }
    }
    for(int i=0;i<10;++i) if(circuits && (Press('0'+i) | Press(VK_NUMPAD0+i))) {
        if(digits.size()<12) digits.push_back('0'+i);
        char keypadSound[16];
        // Donor opcode 3F90 maps each number directly to bttfhv/sound/N.wav.
        snprintf(keypadSound,sizeof(keypadSound),"%d.wav",i);
        Sound(keypadSound);
    }
    if(Press(VK_SUBTRACT) | Press(VK_OEM_MINUS)) {
        if(!circuits) {
            Help("Time circuits are OFF.");
        } else if(digits.size()==4 || digits.size()==8 || digits.size()==12) {
            int date=destinationDate,time=destinationTime;
            if(DonorSystems::ParseDestination(digits,date,time) && DateValid(date,time)) {
                destinationDate=date;destinationTime=time;
                if(digits.size()>=8){destinationConfirmActive=true;destinationConfirmStarted=CTimer::GetTimeInMilliseconds();}
                keypadConfirmUntil=CTimer::GetTimeInMilliseconds()+450;Sound("delorean/timecircuits/enter.wav");Help("Destination accepted.");
                char msg[96]; snprintf(msg,sizeof(msg),"Destination accepted: %08d %04d",date,time); Log(msg);
            }
            else { keypadConfirmUntil=CTimer::GetTimeInMilliseconds()+100; Sound("delorean/timecircuits/error.wav"); Help("Invalid date or time."); Log("Destination rejected: invalid date or time"); }
        } else { keypadConfirmUntil=CTimer::GetTimeInMilliseconds()+100; Sound("delorean/timecircuits/error.wav"); Help("Enter HHMM, MMDDYYYY or MMDDYYYYHHMM, then minus."); Log("Destination rejected: expected 4, 8 or 12 digits"); }
        digits.clear();
    }
    uint32 now=CTimer::GetTimeInMilliseconds();
    if(circuits && fuel && now>=cooldown && DonorSystems::DigitalSpeed(car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND)>=88) {
        if(instantTravelMode)Travel(car);
        else BeginCinematicTravel(car);
    }
    if(now-lastAnim>=100) {
        lastAnim=now;
        DisplayDate("dt",destinationDate,destinationTime);
        DisplayDate("pt",presentDate,CClock::GetHours()*100+CClock::GetMinutes());
        DisplayDate("ltd",departedDate,departedTime);
        for(int i=1;i<=70;++i) {
            hidden.insert("wormhole"+std::to_string(i));
            hidden.insert("wormholer"+std::to_string(i));
        }
        // The donor's advanced-particle opcode draws the TXD animation; the
        // old component meshes remain hidden here and Draw() renders it.
        // Flux lamps are maintained every update by UpdateReactorEffects.
        ApplyVisibility();
    }
    UpdateDestinationConfirmation();
}
void DrawLightBeams() {
    CAutomobile *car=Car();
    // The native beam pass is independent of stock vehicle visibility. Gate it
    // explicitly so headlights and the cabin lamp cannot remain visible during
    // the hidden phase of cinematic departure.
    if(!enabled || !car || !car->bIsVisible || cinematicTravel.active || FrontEndMenuManager.m_bMenuActive) return;
    if(!car->bLightsOn && !emergencyLight.on) return;
    const RwRenderState states[]={rwRENDERSTATETEXTURERASTER,rwRENDERSTATEZTESTENABLE,
        rwRENDERSTATEZWRITEENABLE,rwRENDERSTATEVERTEXALPHAENABLE,rwRENDERSTATESRCBLEND,
        rwRENDERSTATEDESTBLEND,rwRENDERSTATEFOGENABLE,rwRENDERSTATECULLMODE};
    void *saved[8];for(int i=0;i<8;i++) RwRenderStateGet(states[i],&saved[i]);
    uint32 alphaFunc=rw::GetRenderState(rw::ALPHATESTFUNC);
    rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,beamTexture?RwTextureGetRaster(beamTexture):nil);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,(void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,(void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,(void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,(void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,(void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,(void*)FALSE);
    RwRenderStateSet(rwRENDERSTATECULLMODE,(void*)rwCULLMODECULLNONE);
    auto lamp=frames.find("headlights");
    if(car->bLightsOn && lamp!=frames.end()) {
        CVector rightLamp=RwFrameGetLTM(lamp->second)->pos;
        float offset=DotProduct(rightLamp-car->GetPosition(),car->GetRight());
        for(int side=0;side<2;side++) {
            CVector origin=rightLamp-car->GetRight()*(side?2.f*offset:0.f)+car->GetForward()*.06f;
            CVector direction=car->GetForward()-car->GetUp()*.04f;
            direction.Normalise();
            float length=18.f;
            CColPoint hit;CEntity *entity=nil;
            if(CWorld::ProcessLineOfSight(origin,origin+direction*length,hit,entity,true,false,false,true,false,true))
                length=(hit.point-origin).Magnitude();
            if(length>.1f && beamTexture) {
                CVector screen; float sw,sh;
                for(int slice=1;slice<=18;slice++) {
                    float t=float(slice)/18.f;
                    CVector p=origin+direction*(length*t);
                    if(CSprite::CalcScreenCoors(p,&screen,&sw,&sh,true)) {
                        float size=(.10f+length*.12f*t)*sw;
                        CSprite::RenderOneXLUSprite_Rotate_Aspect(screen.x,screen.y,screen.z,size,size,
                            220,230,255,23*(1.f-t),1.f/screen.z,0,190);
                    }
                }
            }
        }
    }
    CVector origin;
    if(emergencyLight.on && CabinLightOrigin(car,origin)) {
        if(beamTexture) {
            CVector direction=-car->GetUp(), screen; float sw,sh;
            for(int slice=1;slice<=12;slice++) {
                float t=float(slice)/12.f; CVector p=origin+direction*(.90f*t);
                if(CSprite::CalcScreenCoors(p,&screen,&sw,&sh,true)) {
                    float size=(.025f+.40f*t)*sw;
                    CSprite::RenderOneXLUSprite_Rotate_Aspect(screen.x,screen.y,screen.z,size,size,
                        255,224,163,38*(1.f-t),1.f/screen.z,0,190);
                }
            }
        }
    }
    for(int i=0;i<8;i++) RwRenderStateSet(states[i],saved[i]);
    rw::SetRenderState(rw::ALPHATESTFUNC,alphaFunc);
}
void DrawWormhole() {
    static bool passLogged=false;
    if(!passLogged) { Log("Wormhole world render callback reached"); passLogged=true; }
    if(!enabled || !Car() || FindPlayerVehicle()!=Car() || FrontEndMenuManager.m_bMenuActive) return;
    CAutomobile *car=Car();
    const float mph=car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND*(88.0f/48.1f);
    if(wormholeFrame>0) {
        const int frame=wormholeFrame-1;
        RwV3d screen; float width,height;
        const CVector world=car->GetPosition()+car->GetForward()*4.6f+car->GetUp()*.45f;
        RwTexture **activeWormholes = variant==3 ? wormholeRedTextures : wormholeTextures;
        if(activeWormholes[frame]) {
            rw::gl3::Shader *previousEffectShader=rw::gl3::im3dOverrideShader;
            rw::gl3::im3dOverrideShader=nil;
            if(wormholeShader){
                rw::gl3::im3dOverrideShader=wormholeShader;wormholeShader->use();
                const int previous=previousWormholeFrame>0?previousWormholeFrame-1:frame;
                rw::gl3::setTexture(1,activeWormholes[previous]);
                glUniform1i(glGetUniformLocation(wormholeShader->program,"tex0"),0);
                glUniform1i(glGetUniformLocation(wormholeShader->program,"tex1"),1);
                glUniform1f(glGetUniformLocation(wormholeShader->program,"frameBlend"),Bound(wormholeTicks,0,1));
                glUniform1f(glGetUniformLocation(wormholeShader->program,"charge"),Bound((mph-75.f)/13.f,0,1));
                glUniform1f(glGetUniformLocation(wormholeShader->program,"effectTime"),CTimer::GetTimeInMilliseconds()*.001f);
                glUniform1f(glGetUniformLocation(wormholeShader->program,"redMode"),variant==3?1.f:0.f);
            }
            const uint32 oldAlphaFunc=rw::GetRenderState(rw::ALPHATESTFUNC);
            const uint32 oldAlphaRef=rw::GetRenderState(rw::ALPHATESTREF);
            rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
            rw::SetRenderState(rw::ALPHATESTREF,0);
            const RwRenderState states[]={rwRENDERSTATETEXTURERASTER,rwRENDERSTATEZTESTENABLE,rwRENDERSTATEZWRITEENABLE,rwRENDERSTATEVERTEXALPHAENABLE,rwRENDERSTATESRCBLEND,rwRENDERSTATEDESTBLEND,rwRENDERSTATEFOGENABLE,rwRENDERSTATECULLMODE};
            void *saved[8]={};
            for(int i=0;i<8;i++) RwRenderStateGet(states[i],&saved[i]);
            RwRenderStateSet(rwRENDERSTATECULLMODE,(void*)rwCULLMODECULLNONE);
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE,(void*)TRUE);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,(void*)FALSE);
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,(void*)TRUE);
            RwRenderStateSet(rwRENDERSTATESRCBLEND,(void*)rwBLENDSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND,(void*)rwBLENDINVSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEFOGENABLE,(void*)FALSE);
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER,RwTextureGetRaster(activeWormholes[frame]));
            // Reuse the donor dome's actual mesh, transform and UVs. The
            // animation remains in the supplemental TXD rather than the DFF.
            struct DomeDraw { int frame; bool red; bool drawn; } dome={frame+1,variant==3,false};
            RpClumpForAllAtomics((RpClump*)car->m_rwObject,[](RpAtomic *atomic,void *data)->RpAtomic*{
                DomeDraw &draw=*static_cast<DomeDraw*>(data);
                const char *name=GetFrameNodeName(RpAtomicGetFrame(atomic));
                const std::string prefix=draw.red ? "wormholer" : "wormhole";
                const std::string selected=prefix+std::to_string(draw.frame);
                // One fixed dome and UV layout for every animation frame.
                // Changing frames must only change the texture, never the mesh.
                if(!name || std::string(name)!=prefix+"1")return atomic;
                if(draw.drawn)return atomic;
                rw::Geometry *geometry=atomic->geometry;
                if(!geometry || !geometry->numVertices || !geometry->numTriangles || !geometry->texCoords[0])return atomic;
                std::vector<RwIm3DVertex> vertices(geometry->numVertices);
                std::vector<RwImVertexIndex> indices(geometry->numTriangles*3);
                const RwMatrix *matrix=RwFrameGetLTM(RpAtomicGetFrame(atomic));
                for(int v=0;v<geometry->numVertices;v++){
                    RwV3d world;RwV3dTransformPoints(&world,&geometry->morphTargets[0].vertices[v],1,matrix);
                    RwIm3DVertexSetPos(&vertices[v],world.x,world.y,world.z);
                    RwIm3DVertexSetRGBA(&vertices[v],255,255,255,255);
                    RwIm3DVertexSetU(&vertices[v],geometry->texCoords[0][v].u);
                    RwIm3DVertexSetV(&vertices[v],geometry->texCoords[0][v].v);
                }
                for(int t=0;t<geometry->numTriangles;t++)for(int j=0;j<3;j++)indices[t*3+j]=geometry->triangles[t].v[j];
                if(RwIm3DTransform(vertices.data(),vertices.size(),nil,rwIM3D_VERTEXXYZ|rwIM3D_VERTEXRGBA|rwIM3D_VERTEXUV)){
                    RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST,indices.data(),indices.size());RwIm3DEnd();
                }
                draw.drawn=true;return atomic;
            },&dome);
            for(int i=0;i<8;i++) RwRenderStateSet(states[i],saved[i]);
            rw::SetRenderState(rw::ALPHATESTFUNC,oldAlphaFunc);
            rw::SetRenderState(rw::ALPHATESTREF,oldAlphaRef);
            rw::gl3::im3dOverrideShader=previousEffectShader;
        }
    }
}
void DrawImplosion() {
    if(!enabled || !implosion.active || FrontEndMenuManager.m_bMenuActive)return;
    const uint32 elapsed=CTimer::GetTimeInMilliseconds()-implosion.started;
    const int frame=Min(20,int(elapsed/90));
    RwTexture *texture=implosionTextures[frame];if(!texture)return;
    rw::gl3::Shader *previousShader=rw::gl3::im3dOverrideShader;
    if(implosionShader){
        rw::gl3::im3dOverrideShader=implosionShader;
        implosionShader->use();
        glUniform1f(glGetUniformLocation(implosionShader->program,"frameBlend"),float(elapsed%90)/90.0f);
        rw::gl3::setTexture(1,implosionTextures[Min(20,frame+1)]);
    }
    const RwRenderState states[]={rwRENDERSTATETEXTURERASTER,rwRENDERSTATEZTESTENABLE,rwRENDERSTATEZWRITEENABLE,
        rwRENDERSTATEVERTEXALPHAENABLE,rwRENDERSTATESRCBLEND,rwRENDERSTATEDESTBLEND,rwRENDERSTATEFOGENABLE,rwRENDERSTATECULLMODE};
    void *saved[8]={};for(int i=0;i<8;i++)RwRenderStateGet(states[i],&saved[i]);
    const uint32 alphaFunc=rw::GetRenderState(rw::ALPHATESTFUNC);
    rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,RwTextureGetRaster(texture));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,(void*)TRUE);
    // Composite after the transparent trails, testing against solid world
    // geometry. A blended quad must not write its transparent corners into
    // scene depth (the ray-marched trails read that depth as an opaque wall).
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,(void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,(void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,(void*)rwBLENDSRCALPHA);RwRenderStateSet(rwRENDERSTATEDESTBLEND,(void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,(void*)FALSE);RwRenderStateSet(rwRENDERSTATECULLMODE,(void*)rwCULLMODECULLNONE);
    RwIm3DVertex vertices[4];
    const CVector right=TheCamera.GetRight()*5.4f,up=TheCamera.GetUp()*5.4f;
    const CVector points[]={implosion.position-right+up,implosion.position-right-up,implosion.position+right-up,implosion.position+right+up};
    const float uv[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(int i=0;i<4;i++){
        RwIm3DVertexSetPos(&vertices[i],points[i].x,points[i].y,points[i].z);RwIm3DVertexSetRGBA(&vertices[i],255,255,255,255);
        RwIm3DVertexSetU(&vertices[i],uv[i][0]);RwIm3DVertexSetV(&vertices[i],uv[i][1]);
    }
    if(RwIm3DTransform(vertices,4,nil,rwIM3D_VERTEXXYZ|rwIM3D_VERTEXRGBA|rwIM3D_VERTEXUV)){
        RwImVertexIndex indices[]={0,1,2,0,2,3};RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST,indices,6);RwIm3DEnd();
    }
    for(int i=0;i<8;i++)RwRenderStateSet(states[i],saved[i]);rw::SetRenderState(rw::ALPHATESTFUNC,alphaFunc);
    rw::gl3::im3dOverrideShader=previousShader;
    rw::gl3::setTexture(1,nil);
}
void Draw() {
    if(!enabled || !Car() || FindPlayerVehicle()!=Car() || FrontEndMenuManager.m_bMenuActive) return;
    wchar line[256]; char msg[240];
    const float sx=SCREEN_WIDTH/640.0f, sy=SCREEN_HEIGHT/448.0f;
    if(CTimer::GetTimeInMilliseconds()<flashUntil)
        CSprite2d::DrawRect(CRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT),CRGBA(210,235,255,180));
    if(!travelHudVisible)return;
    CFont::SetScale(0.38f*sx,0.75f*sy); CFont::SetPropOn(); CFont::SetBackgroundOff(); CFont::SetCentreOff(); CFont::SetRightJustifyOff(); CFont::SetFontStyle(FONT_STANDARD);
    CFont::SetWrapx(620.0f*sx); CFont::SetColor(CRGBA(240,170,65,255));
    snprintf(msg,sizeof(msg),"DEST %08d %04d   %s   %s",destinationDate,destinationTime,circuits?"ON":"OFF",fuel?"READY":"REFUEL");
    AsciiToUnicode(msg,line); CFont::PrintString(190*sx,360*sy,line);
    CFont::SetColor(CRGBA(90,255,120,255));
    snprintf(msg,sizeof(msg),"NOW %08d %02d%02d   %.0f MPH%s",presentDate,CClock::GetHours(),CClock::GetMinutes(),Car()->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND*(88.0f/48.1f),hover?"  HOVER":"");
    AsciiToUnicode(msg,line); CFont::PrintString(190*sx,377*sy,line);
    CFont::SetColor(CRGBA(255,110,75,255));
    snprintf(msg,sizeof(msg),"LAST %08d %04d",departedDate,departedTime);
    AsciiToUnicode(msg,line); CFont::PrintString(190*sx,394*sy,line);
    if(variant==1) {
        snprintf(msg,sizeof(msg),"PLUTONIUM CANS %d",plutonium);
        AsciiToUnicode(msg,line); CFont::PrintString(190*sx,411*sy,line);
    }
    if(!digits.empty()) { AsciiToUnicode(digits.c_str(),line); CFont::PrintString(190*sx,428*sy,line); }
}
void Shutdown() {
    DonorMirrors::Clear();
    for(int i=PARTICLE_LEAF_FIRST;i<MAX_PARTICLES;i++)CParticle::RemovePSystem((tParticleType)i);
    for(auto texture:presetTextures)RwTextureDestroy(texture);presetTextures.clear();presetRasters.clear();
    LeafMods::SetRailWheelVehicle(nil);
    CancelCinematicTravel();
    ShutdownFrostShader();
    if(plasmaShader){plasmaShader->destroy();plasmaShader=nil;}
    DestroyFireVolumeResources();
    if(fireTrailShader){fireTrailShader->destroy();fireTrailShader=nil;}
    if(fireTrailTexture){RwTextureDestroy(fireTrailTexture);fireTrailTexture=nil;}
    ShutdownCabinDetail();
    if(implosionShader){implosionShader->destroy();implosionShader=nil;}
    if(wormholeShader){wormholeShader->destroy();wormholeShader=nil;}
    if(hookDoorReserved && Car()) Car()->m_nGettingInFlags &= ~CAR_DOOR_FLAG_RF;
    hookDoorReserved=false;
    enabled=false; CancelRefuel(); DonorAudio::Shutdown();
    if(beamTexture) { RwTextureDestroy(beamTexture); beamTexture=nil; }
    if(beamTxd>=0) { if(CTxdStore::GetNumRefs(beamTxd)>0) CTxdStore::RemoveRefWithoutDelete(beamTxd); if(CTxdStore::GetNumRefs(beamTxd)==0) CTxdStore::RemoveTxdSlot(beamTxd); beamTxd=-1; }
    for(RwTexture *&texture:wormholeTextures) { if(texture) RwTextureDestroy(texture); texture=nil; }
    for(RwTexture *&texture:wormholeRedTextures) { if(texture) RwTextureDestroy(texture); texture=nil; }
    for(RwTexture *&texture:implosionTextures) { if(texture) RwTextureDestroy(texture); texture=nil; }
    if(changedModel && model) *mod_HandlingManager.GetHandlingData((tVehicleType)model->m_handlingId)=oldHandling;
    if(changedModel && model && model->GetNumRefs()==0) {
        model->DeleteRwObject();
        if(model->GetColModel()!=oldCol) model->DeleteCollisionModel();
        model->SetColModel(oldCol,oldColOwn);
        model->SetModelName(oldName); memcpy(model->m_gameName,oldGameName,10);
        model->m_wheelScale=oldWheelScale;
        model->SetTexDictionary(CTxdStore::GetTxdName(oldTxd));
        CStreaming::ms_aInfoForModel[Model].m_loadState=STREAMSTATE_NOTLOADED;
        CStreaming::ms_aInfoForModel[Model].m_flags=0;
    } else if(changedModel && oldColOwn && oldCol && model->GetColModel()!=oldCol) {
        // Live instances are destroyed by the world immediately after this callback.
        delete oldCol;
    }
    if(txd>=0) {
        if(CTxdStore::GetNumRefs(txd)>0) CTxdStore::RemoveRefWithoutDelete(txd);
        if(CTxdStore::GetNumRefs(txd)==0) CTxdStore::RemoveTxdSlot(txd);
    }
    if(particleTxd>=0) {
        if(CTxdStore::GetNumRefs(particleTxd)>0) CTxdStore::RemoveRefWithoutDelete(particleTxd);
        if(CTxdStore::GetNumRefs(particleTxd)==0) CTxdStore::RemoveTxdSlot(particleTxd);
    }
    LeafMods::RegisterPreservedVehicleModel(Model,false);
    vehicleRef=-1; frames.clear(); originals.clear();
    Log("DeLorean module stopped.");
}
const LeafMod mod={LEAF_ABI_VERSION,Initialise,Update,Draw,Shutdown};
bool DriverPedalPose(CPed *ped,bool apply) {
    // The donor does not alter Tommy's leg bones. Keep the native seated pose
    // until a vehicle-space IK target is available; this avoids twisting feet
    // when the car rotates or banks.
    if(!apply) return false;
    CAutomobile *car=Car();
    if(!enabled || !car || !ped || car->pDriver!=ped || ped->GetPedState()!=PED_DRIVING || !ped->bInVehicle)return false;
    if(!apply)return true;
    RpHAnimHierarchy *hier=GetAnimHierarchyFromSkinClump(ped->GetClump());
    if(!hier)return false;
    RwMatrix *matrices=RpHAnimHierarchyGetMatrixArray(hier);
    auto point=[](const RwV3d &v){return DonorLegPose::V(v.x,v.y,v.z);};
    auto pedal=[&](const char *name,CVector &result){
        auto f=frames.find(name);if(f==frames.end())return false;
        result=RwFrameGetLTM(f->second)->pos;return true;
    };
    CVector gas,brake,clutch;
    if(!pedal("gaspedal",gas) || !pedal("brakepedal",brake) || !pedal("clutchpedal",clutch))return false;
    const float braking=Bound(cabinPedals.brake/20.0f,0,1);
    // Targets are ankles, behind the toe contact on each pedal. Keep the
    // native foot orientation; solve knees rather than stretching the legs.
    const CVector ankleOffset=-car->GetForward()*.16f+car->GetUp()*.06f;
    const CVector targets[]={clutch+ankleOffset,gas*(1-braking)+brake*braking+ankleOffset};
    const int nodes[2][3]={{PED_UPPERLEGL,PED_LOWERLEGL,PED_FOOTL},{PED_UPPERLEGR,PED_LOWERLEGR,PED_FOOTR}};
    for(int side=0;side<2;side++){
        int indices[3];bool valid=true;
        for(int i=0;i<3;i++){
            indices[i]=RpHAnimIDGetIndex(hier,ConvertPedNode2BoneTag(nodes[side][i]));
            if(indices[i]<0 || indices[i]>=hier->numNodes)valid=false;
        }
        if(!valid)continue;
        RwMatrix &upper=matrices[indices[0]],&lower=matrices[indices[1]],&foot=matrices[indices[2]];
        const auto hip=point(upper.pos),knee=point(lower.pos),ankle=point(foot.pos);
        DonorLegPose::V nextKnee,nextAnkle;
        if(!DonorLegPose::Solve(hip,knee,ankle,point(targets[side]),point(car->GetUp()),nextKnee,nextAnkle))continue;
        auto orient=[&](RwMatrix &m,DonorLegPose::V from,DonorLegPose::V to){
            for(RwV3d *v:{&m.right,&m.up,&m.at}){
                const auto rotated=DonorLegPose::Align(point(*v),from,to);
                v->x=rotated.x;v->y=rotated.y;v->z=rotated.z;
            }
            RwMatrixUpdate(&m);
        };
        orient(upper,knee-hip,nextKnee-hip);
        orient(lower,ankle-knee,nextAnkle-nextKnee);
        const float pedalAngle=DEGTORAD(side==0?cabinPedals.clutch:
            cabinPedals.gas*(1-braking)+cabinPedals.brake*braking);
        orient(foot,point(car->GetUp()),point(car->GetUp()*Cos(pedalAngle)-car->GetForward()*Sin(pedalAngle)));
        lower.pos=CVector(nextKnee.x,nextKnee.y,nextKnee.z);
        foot.pos=CVector(nextAnkle.x,nextAnkle.y,nextAnkle.z);
        RwMatrixUpdate(&lower);RwMatrixUpdate(&foot);
    }
    return true;
}
}
extern "C" __declspec(dllexport) bool LeafPedPose(void *ped,bool apply) {return DriverPedalPose(static_cast<CPed*>(ped),apply);}
extern "C" __declspec(dllexport) const LeafMod *LeafGetMod() { return &mod; }
// Optional debug-module service; no host ABI change and no debug key in gameplay.
extern "C" __declspec(dllexport) bool LeafSetFireTrailPreview(bool on) {return SetFireTrailPreview(on);}
extern "C" __declspec(dllexport) bool LeafIsFireTrailPreviewActive() {return previewFireTrail.active;}
extern "C" __declspec(dllexport) bool LeafRenderPass(uint32_t stage) {
    if(stage==LEAF_PRE_RENDER && enabled && Car()) { UpdateLightBeams(Car()); DonorMirrors::Update(Car(),variant==3 && hookMode!=2 && !cinematicTravel.active); }
    if(stage==LEAF_BEFORE_VEHICLES) DrawWormhole();
    if(stage==LEAF_WORLD_END) { DrawLightBeams(); DrawTravelArcs(); DrawHoodboxEffects(); DrawWheelPlasma(); DrawFireTrails(); DrawImplosion(); if(enabled && variant==3 && hookMode!=2 && !cinematicTravel.active)DonorMirrors::DrawHubcaps(Car()); }
    return false;
}






