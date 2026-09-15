// Optional test conveniences. This module is deliberately separate from gameplay mods.
#define NOMINMAX
#include <windows.h>
#include <direct.h>
#include <string>
#include "common.h"
#include "leaf_api.h"
#include "PCSave.h"
#include "Frontend.h"
#include "Timer.h"
#include "World.h"
#include "CutsceneMgr.h"
#include "Camera.h"
#include "Vehicle.h"
#include "PlayerPed.h"
#include "Pad.h"
#include "Hud.h"
#include "Text.h"
#include "Font.h"
#include <cmath>

namespace {
LeafHost host;
bool active=false;
#ifndef LEAF_RELEASE_DEBUG
#define LEAF_RELEASE_DEBUG 0
#endif
#if !LEAF_RELEASE_DEBUG
bool alphaUp=false, alphaDown=false;
#endif
std::string saveDir;
bool cameraKey=false;
#if !LEAF_RELEASE_DEBUG
bool fireKey=false,firePreview=false;
#endif
void Log(const char *s){if(host.log)host.log(s);}
#if !LEAF_RELEASE_DEBUG
bool FirePreview(bool on){
    HMODULE module=GetModuleHandleA("bttf_delorean.dll");
    typedef bool (*SetPreview)(bool);
    const auto set=module?reinterpret_cast<SetPreview>(GetProcAddress(module,"LeafSetFireTrailPreview")):nullptr;
    return set && set(on);
}
#endif
bool Init(const LeafHost *h){
    if(!h || h->abiVersion!=LEAF_ABI_VERSION)return false;
    host=*h;
    char exe[MAX_PATH]={};GetModuleFileNameA(NULL,exe,sizeof(exe));
#if LEAF_RELEASE_DEBUG
    active=true;Log("Release debug module active: F10 first-person camera only.");
#else
    saveDir=exe;size_t slash=saveDir.find_last_of("\\/");if(slash!=std::string::npos)saveDir.resize(slash+1);
    saveDir += "debug-saves";_mkdir(saveDir.c_str());
    C_PcSave::SetSaveDirectory(saveDir.c_str());
    if (host.setScmEnabled) host.setScmEnabled(true);
    active=true;Log("Debug module active: packaged freeroam SCM; campaign script replaced. F9 toggles test fire trails.");
#endif
    return true;
}
void Update(){
#if !LEAF_RELEASE_DEBUG
    // Track releases even while paused or unfocused so an old held-key latch
    // cannot swallow the next F9 press after returning to the game.
    const bool fireDown=(GetAsyncKeyState(VK_F9)&0x8000)!=0;
    const bool firePressed=fireDown && !fireKey;fireKey=fireDown;
#endif
    if(!active || FrontEndMenuManager.m_bMenuActive || CTimer::GetIsPaused())return;
    DWORD pid=0; GetWindowThreadProcessId(GetForegroundWindow(),&pid);
    if(pid!=GetCurrentProcessId()) return;
    bool camDown=(GetAsyncKeyState(VK_F10)&0x8000)!=0;
    if(camDown && !cameraKey && FindPlayerPed()) {
        CCamera::bLeafFirstPerson = !CCamera::bLeafFirstPerson;
        wchar message[128];
        AsciiToUnicode(CCamera::bLeafFirstPerson ? "First person: ON" : "First person: OFF", message);
        CHud::SetHelpMessage(message,true);
        Log(CCamera::bLeafFirstPerson ? "Head camera v2 enabled" : "Head camera v2 disabled");
    }
    cameraKey=camDown;
#if !LEAF_RELEASE_DEBUG
    if(firePressed){
        HMODULE module=GetModuleHandleA("bttf_delorean.dll");
        typedef bool (*IsPreview)();
        const auto query=module?reinterpret_cast<IsPreview>(GetProcAddress(module,"LeafIsFireTrailPreviewActive")):nullptr;
        if(query)firePreview=query();
        const bool ok=FirePreview(!firePreview);
        if(ok)firePreview=!firePreview;
        const char *message=!ok?"Fire test unavailable: updated DeLorean leaf required":
            firePreview?"Fire trail test ON: stays lit. F9 stops it.":"Fire trail test OFF";
        wchar text[128];AsciiToUnicode(message,text);CHud::SetHelpMessage(text,true);Log(message);
    }
#endif
#if !LEAF_RELEASE_DEBUG
    bool up=(GetAsyncKeyState(VK_ADD)&0x8000)!=0, downAlpha=(GetAsyncKeyState(VK_SUBTRACT)&0x8000)!=0;
    if(up&&!alphaUp) CCamera::bLeafFirstPersonBodyAlpha=std::min(1.0f,CCamera::bLeafFirstPersonBodyAlpha+.1f);
    if(downAlpha&&!alphaDown) CCamera::bLeafFirstPersonBodyAlpha=std::max(.1f,CCamera::bLeafFirstPersonBodyAlpha-.1f);
    if((up&&!alphaUp)||(downAlpha&&!alphaDown)){char msg[80];snprintf(msg,sizeof(msg),"First-person body opacity: %d%%",int(CCamera::bLeafFirstPersonBodyAlpha*100));wchar text[80];AsciiToUnicode(msg,text);CHud::SetHelpMessage(text,true);}
    alphaUp=up;alphaDown=downAlpha;
#endif
}
void Shutdown(){
#if !LEAF_RELEASE_DEBUG
    if(firePreview)FirePreview(false);firePreview=false;
#endif
    CCamera::bLeafFirstPerson=false;active=false;
}
const LeafMod mod={LEAF_ABI_VERSION,Init,Update,nil,Shutdown};
}
extern "C" __declspec(dllexport) const LeafMod *LeafGetMod(){return &mod;}
