// Native GL3 graphics module. Shader assets and settings live entirely in graphics.leaf.
#define NOMINMAX
#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include "common.h"
#include "leaf_api.h"
#include "Camera.h"
#include "Clock.h"
#include "Weather.h"
#include "Timer.h"
#include "Frontend.h"
#include "Hud.h"
#include "Text.h"
#include "Font.h"
#include "Sprite2d.h"
#include "CloudNoise.h"
#include "Timecycle.h"
#include "Particle.h"

namespace {
LeafHost host;
std::string root;
rw::gl3::Shader *water=nil,*previousWater=nil;
rw::gl3::Shader *sky=nil;
GLuint volume=0;
bool active=false,waterPass=false,keyK=false,keyW=false;
float amplitude=.8f,reflection=.9f;
int preset=-1;
std::string Read(const char *name);
void SkyUniform(const char *name,float a,float b,float c,float d){
    GLint location=glGetUniformLocation(sky->program,name);
    if(location>=0)glUniform4f(location,a,b,c,d);
}
bool MakeSky(){
    std::string fragment=Read("sky.frag");
    const char *vertexCode="uniform vec4 u_xform; VSIN(ATTRIB_POS) vec4 in_pos; void main(){gl_Position=in_pos;gl_Position.xy=gl_Position.xy*u_xform.xy+u_xform.zw;gl_Position.xyz*=gl_Position.w;}";
    const char *vs[]={rw::gl3::shaderDecl,rw::gl3::header_vert_src,vertexCode,nil};
    const char *fs[]={rw::gl3::shaderDecl,rw::gl3::header_frag_src,fragment.c_str(),nil};
    sky=rw::gl3::Shader::create(vs,fs);if(!sky)return false;
    std::vector<unsigned char> pixels(64*64*64*4);
    for(int z=0;z<64;z++)for(int y=0;y<64;y++)for(int x=0;x<64;x++){
        float a=x/64.f,b=y/64.f,c=z/64.f;
        float channels[]={LeafCloud::Noise(a*4,b*4,c*4,4)*.62f+LeafCloud::Noise(a*8,b*8,c*8,8)*.26f+LeafCloud::Noise(a*16,b*16,c*16,16)*.12f,
            LeafCloud::Cellular(a*8,b*8,c*8),LeafCloud::Noise(a*16,b*16,c*16,16)*.65f+LeafCloud::Noise(a*32,b*32,c*32,32)*.35f,
            LeafCloud::Noise(a*8+13,b*8+3,c*8+7,8)};
        for(int i=0;i<4;i++)pixels[((z*64+y)*64+x)*4+i]=(unsigned char)(channels[i]*255);
    }
    GLint old=0;glGetIntegerv(GL_TEXTURE_BINDING_3D,&old);
    glGenTextures(1,&volume);glBindTexture(GL_TEXTURE_3D,volume);
    glTexImage3D(GL_TEXTURE_3D,0,GL_RGBA8,64,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_T,GL_REPEAT);glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_R,GL_REPEAT);
    glBindTexture(GL_TEXTURE_3D,old);return true;
}
std::string Read(const char *name) {
    std::ifstream file(root+"/shaders/"+name,std::ios::binary);
    std::ostringstream content; content<<file.rdbuf(); return content.str();
}
void Uniform4(const char *name,float a,float b,float c,float d) {
    GLint location=glGetUniformLocation(water->program,name);
    if(location>=0) glUniform4f(location,a,b,c,d);
}
void Help(const char *s) { wchar text[128]; AsciiToUnicode(s,text); CHud::SetHelpMessage(text,true); }
bool Init(const LeafHost *h) {
    if(!h || h->abiVersion!=LEAF_ABI_VERSION) return false;
    host=*h;root=h->packageDirectory;
    std::string vs=Read("water.vert"),fs=Read("water.frag");
    if(vs.empty() || fs.empty()) {host.log("Graphics shader assets missing");return false;}
    const char *vertex[]={rw::gl3::shaderDecl,rw::gl3::header_vert_src,vs.c_str(),nil};
    const char *fragment[]={rw::gl3::shaderDecl,rw::gl3::header_frag_src,fs.c_str(),nil};
    water=rw::gl3::Shader::create(vertex,fragment);
    if(!water) {host.log("GL3 water shader compilation failed");return false;}
    if(!MakeSky()){host.log("GL3 sky shader compilation failed");return false;}
    if(!CParticle::LoadLeafDebrisTextures((root+"/assets/particle_original.txd").c_str())) {
        host.log("Original Vice City debris textures failed to load"); return false;
    }
    active=true;host.log("Native graphics: GL3 swell and water lighting active; Ctrl+K toggle, Ctrl+W weather.");
    return true;
}
void Update() {
    if(FrontEndMenuManager.m_bMenuActive || CTimer::GetIsPaused())return;
    DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);if(pid!=GetCurrentProcessId())return;
    bool ctrl=(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0;
    bool k=(GetAsyncKeyState('K')&0x8000)!=0,w=(GetAsyncKeyState('W')&0x8000)!=0;
    if(ctrl&&k&&!keyK){active=!active;Help(active?"Leaf graphics enabled":"Leaf graphics disabled");}
    if(ctrl&&w&&!keyW){
        preset++;if(preset>5)preset=-1;
        if(preset<0)CWeather::ReleaseWeather();else CWeather::ForceWeather(preset);
        const char *names[]={"Automatic weather","Sunny","Cloudy","Rain","Fog","Extra sunny","Hurricane"};
        Help(names[preset+1]);host.log(names[preset+1]);
    }
    keyK=k;keyW=w;
}
bool Pass(uint32_t stage) {
    if(stage==LEAF_WATER_END && waterPass){rw::gl3::im3dOverrideShader=previousWater;waterPass=false;return false;}
    if(!active || !water)return false;
    if(stage==LEAF_SKY && sky){
        rw::gl3::Shader *previous=rw::gl3::im2dOverrideShader;
        rw::gl3::im2dOverrideShader=sky;sky->use();
        GLint oldUnit=0,oldTexture=0;glGetIntegerv(GL_ACTIVE_TEXTURE,&oldUnit);
        glActiveTexture(GL_TEXTURE1);glGetIntegerv(GL_TEXTURE_BINDING_3D,&oldTexture);glBindTexture(GL_TEXTURE_3D,volume);
        glUniform1i(glGetUniformLocation(sky->program,"Volume"),1);
        glActiveTexture(oldUnit);
        float hour=CClock::GetHours()+CClock::GetMinutes()/60.f,angle=(hour-6)*3.14159265f/12;
        float sun=sinf(angle),day=fminf(1,fmaxf(0,(sun+.2f)/.32f));
        const float coverage[]={.30f,.78f,.48f,.65f,0,1};
        int oldType=CWeather::OldWeatherType,newType=CWeather::NewWeatherType;
        if(oldType<0||oldType>5)oldType=0;if(newType<0||newType>5)newType=0;
        float blend=CWeather::InterpolationValue;
        float clouds=coverage[oldType]*(1-blend)+coverage[newType]*blend;
        float storm=(oldType==5?1.f:0.f)*(1-blend)+(newType==5?1.f:0.f)*blend;
        CVector camera=TheCamera.GetPosition(),right=TheCamera.GetRight(),up=TheCamera.GetUp(),forward=TheCamera.GetForward();
        const RwV2d *window=RwCameraGetViewWindow(TheCamera.m_pRwCamera);
        SkyUniform("Camera",camera.x,camera.y,camera.z,0);
        SkyUniform("Right",right.x,right.y,right.z,0);SkyUniform("Up",up.x,up.y,up.z,0);SkyUniform("Forward",forward.x,forward.y,forward.z,0);
        SkyUniform("Projection",window->x,window->y,SCREEN_WIDTH,SCREEN_HEIGHT);
        SkyUniform("Climate",CTimer::GetTimeInMilliseconds()/60000.f,clouds,storm,CWeather::Foggyness);
        SkyUniform("Layer",650-storm*370,650+storm*750,32,CWeather::LightningFlash?1.f:0.f);
        SkyUniform("Solar",cosf(angle),0,sun,day);
        SkyUniform("Fog",CTimeCycle::GetFogRed()/255.f,CTimeCycle::GetFogGreen()/255.f,CTimeCycle::GetFogBlue()/255.f,0);
        SkyUniform("Celestial",expf(-sun*sun*45)*day*(1-storm),1,hour,0);
        SkyUniform("AfterRain",0,CWeather::Rainbow,0,0);SkyUniform("RainPatch",0,0,2800,1);
        CSprite2d::DrawRect(CRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT),CRGBA(255,255,255,255));
        rw::gl3::im2dOverrideShader=previous;
        glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_3D,oldTexture);glActiveTexture(oldUnit);
        return true;
    }
    if(stage==LEAF_WATER_BEGIN && !waterPass){
        previousWater=rw::gl3::im3dOverrideShader;rw::gl3::im3dOverrideShader=water;waterPass=true;
        water->use();
        float hour=CClock::GetHours()+CClock::GetMinutes()/60.f;
        float angle=(hour-6)*3.14159265f/12;
        float day=fminf(1,fmaxf(0,(sinf(angle)+.2f)/.32f));
        CVector camera=TheCamera.GetPosition();
        Uniform4("leafTime",CTimer::GetTimeInMilliseconds()/1000.f,amplitude*(.25f+CWeather::Wind*.75f),CWeather::Wind,day);
        Uniform4("leafCamera",camera.x,camera.y,camera.z,0);
        Uniform4("leafWater",100.f/255,225.f/255,1,reflection);
        Uniform4("leafSun",cosf(angle),0,sinf(angle),0);
    }
    return false;
}
void Shutdown(){
    CParticle::UnloadLeafDebrisTextures();
    if(waterPass){rw::gl3::im3dOverrideShader=previousWater;waterPass=false;}
    if(preset>=0)CWeather::ReleaseWeather();
    if(water){water->destroy();water=nil;}
    if(sky){sky->destroy();sky=nil;}
    if(volume){glDeleteTextures(1,&volume);volume=0;}
    active=false;
}
const LeafMod mod={LEAF_ABI_VERSION,Init,Update,nil,Shutdown};
}
extern "C" __declspec(dllexport) const LeafMod *LeafGetMod(){return &mod;}
extern "C" __declspec(dllexport) bool LeafRenderPass(uint32_t stage){return Pass(stage);}
