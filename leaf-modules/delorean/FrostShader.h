#pragma once
#include "IceSurface.h"
rw::Texture *iceSurfaceTexture=nullptr;
rw::gl3::Shader *frostShader=nil;
rw::gl3::Shader *frostGlassShader=nil;
rw::gl3::Shader *frostDoorGlassShader=nil;
bool FrostIsGlass(const char *name){return name && (strstr(name,"window") || strstr(name,"windscreen") || strstr(name,"glass"));}
bool FrostIsOriginalDoorGlass(const char *name){return name && (!strcmp(name,"door_lf_hi_ok_glass") || !strcmp(name,"door_rf_hi_ok_glass") || !strcmp(name,"door_lf_hi_ok_window") || !strcmp(name,"door_rf_hi_ok_window"));}
rw::gl3::ObjPipeline *frostPipeline=nil;
std::map<rw::Atomic*,rw::ObjPipeline*> frostRenderers;
bool frostDrawLogged=false;
// Run at the actual GPU draw, including draws deferred by vehicle alpha sorting.
void FrostDraw(rw::Atomic *atomic,rw::gl3::InstanceDataHeader *header){
    rw::gl3::Shader *saved=rw::gl3::defaultShader;
    const char *frameName=GetFrameNodeName(atomic->getFrame());
    const bool glass=FrostIsGlass(frameName);
    const bool originalDoorGlass=FrostIsOriginalDoorGlass(frameName);
    const bool doorGlass=glass && frameName && strstr(frameName,"door_");
    bool interiorView=false;
    CAutomobile *viewCar=Car();
    if(viewCar && FindPlayerVehicle()==viewCar){
        const CVector cameraOffset=TheCamera.GetPosition()-viewCar->GetPosition();
        interiorView=Abs(DotProduct(cameraOffset,viewCar->GetRight()))<1.8f &&
            Abs(DotProduct(cameraOffset,viewCar->GetForward()))<3.0f &&
            Abs(DotProduct(cameraOffset,viewCar->GetUp()))<1.8f;
    }
    const bool insideGlass=glass && !originalDoorGlass && (CCamera::bLeafFirstPerson || interiorView);
    // Original door glass carries the inside coating now. Do not draw the
    // duplicate outer shell through it and through the cabin instruments.
    if(doorGlass && !originalDoorGlass && interiorView)return;
    // Side glass needs the denser crust shader: the thin condensation branch
    // becomes nearly invisible at the door's grazing cockpit angle.
    const bool frostActive=coldStarted && CTimer::GetTimeInMilliseconds()-coldStarted<49725;
    rw::gl3::Shader *selected=originalDoorGlass?(frostActive?frostDoorGlassShader:nil):
        (doorGlass?frostShader:(glass?frostGlassShader:frostShader));
    if(selected && iceSurfaceTexture)rw::gl3::defaultShader=selected;
    if(selected)rw::gl3::setTexture(1,iceSurfaceTexture);
    void *savedCull=nullptr;
    void *savedZTest=nullptr;
    if(glass){
        RwRenderStateGet(rwRENDERSTATECULLMODE,&savedCull);
        RwRenderStateSet(rwRENDERSTATECULLMODE,(void*)rwCULLMODECULLNONE);
        if(insideGlass){
            // The donor frost shell sits just outside the original glass.
            // From inside, that glass has already written a nearer depth.
            RwRenderStateGet(rwRENDERSTATEZTESTENABLE,&savedZTest);
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE,(void*)FALSE);
        }
    }
    rw::gl3::defaultRenderCB(atomic,header);
    if(insideGlass)RwRenderStateSet(rwRENDERSTATEZTESTENABLE,savedZTest);
    if(glass)RwRenderStateSet(rwRENDERSTATECULLMODE,savedCull);
    rw::gl3::defaultShader=saved;
    rw::gl3::setTexture(1,nullptr);
    if(!frostDrawLogged){Log("GPU frost pipeline reached actual draw");frostDrawLogged=true;}
}
RpAtomic *AttachFrostShader(RpAtomic *atomic,void *){
    const char *name=GetFrameNodeName(RpAtomicGetFrame(atomic));
    bool attach=FrostIsOriginalDoorGlass(name);
    for(const auto &n:FROSTED_COMPONENTS)if(name && n==name)attach=true;
    if(attach && frostPipeline){
        frostRenderers[atomic]=atomic->pipeline;
        atomic->pipeline=frostPipeline;
    }
    return atomic;
}
void InitialiseFrostShader(){
    if(!iceSurfaceTexture){
        iceSurfaceTexture=LoadIceSurfaceTexture(root+"/source/ice-surface.rgba");
        Log(iceSurfaceTexture?"Ice surface detail texture loaded":"Ice detail texture missing; donor fallback active");
    }
    if(!frostPipeline){
        frostPipeline=rw::gl3::ObjPipeline::create();
        if(frostPipeline){
            frostPipeline->instanceCB=rw::gl3::defaultInstanceCB;
            frostPipeline->uninstanceCB=rw::gl3::defaultUninstanceCB;
            frostPipeline->renderCB=FrostDraw;
        }
    }
    if(frostShader)return;
    std::ifstream file((root+"/source/frost.frag").c_str());
    std::string fragment((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    const char *vertex=R"GLSL(
VSIN(ATTRIB_POS) vec3 in_pos;
VSOUT float v_snow; VSOUT float v_snowEdge;
VSOUT vec4 v_color; VSOUT vec2 v_tex0; VSOUT float v_fog;
VSOUT vec3 v_local; VSOUT vec3 v_view; VSOUT vec3 v_normal; VSOUT vec3 v_objectNormal;
void main(){
 vec4 world=u_world*vec4(in_pos,1.0);
 vec4 view=u_view*world;
 gl_Position=u_proj*view;
 vec3 normal=mat3(u_world)*in_normal;
 v_color=in_color;
 v_color.rgb+=u_ambLight.rgb*surfAmbient+DoDynamicLight(world.xyz,normal)*surfDiffuse;
 v_color=clamp(v_color,0.0,1.0)*u_matColor;
 v_tex0=in_tex0;v_fog=DoFog(gl_Position.w);
 v_local=in_pos;v_view=view.xyz;
 v_normal=mat3(u_view)*normal;v_objectNormal=in_normal;
 v_snow=abs(in_tex0.x+1024.0)<.1?1.0:0.0;v_snowEdge=in_tex0.y;
}
)GLSL";
    const char *vs[]={rw::gl3::shaderDecl,rw::gl3::header_vert_src,vertex,nil};
    const char *fs[]={rw::gl3::shaderDecl,rw::gl3::header_frag_src,fragment.c_str(),nil};
    frostShader=rw::gl3::Shader::create(vs,fs);
    const char *glassFs[]={rw::gl3::shaderDecl,rw::gl3::header_frag_src,"\n#define FROST_GLASS 1\n",fragment.c_str(),nil};
    frostGlassShader=rw::gl3::Shader::create(vs,glassFs);
    const char *doorGlassFs[]={rw::gl3::shaderDecl,rw::gl3::header_frag_src,"\n#define FROST_GLASS 1\n#define FROST_DOOR_GLASS 1\n",fragment.c_str(),nil};
    frostDoorGlassShader=rw::gl3::Shader::create(vs,doorGlassFs);
    Log(frostGlassShader?"Thin glass condensation shader ready":"Glass condensation shader failed; using donor fallback");
    Log(frostShader?"GPU patchy frost shader ready":"GPU frost shader failed; using geometry fallback");
}
void ShutdownFrostShader(){
    if(Car())RpClumpForAllAtomics((RpClump*)Car()->m_rwObject,[](RpAtomic *a,void*)->RpAtomic*{
        auto it=frostRenderers.find(a);if(it!=frostRenderers.end())a->pipeline=it->second;return a;
    },nil);
    frostRenderers.clear();
    if(frostShader){frostShader->destroy();frostShader=nil;}
    if(frostGlassShader){frostGlassShader->destroy();frostGlassShader=nil;}
    if(frostDoorGlassShader){frostDoorGlassShader->destroy();frostDoorGlassShader=nil;}
    if(frostPipeline){frostPipeline->destroy();frostPipeline=nil;}
    frostDrawLogged=false;
    if(iceSurfaceTexture){iceSurfaceTexture->destroy();iceSurfaceTexture=nullptr;}
}
