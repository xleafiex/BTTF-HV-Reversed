#pragma once
static rw::gl3::Shader *plasmaShader=nil;
static void InitialisePlasmaShader(){
    const char *vertex=R"GLSL(
VSIN(ATTRIB_POS) vec3 in_pos;
VSOUT vec2 uv;
void main(){gl_Position=u_proj*u_view*u_world*vec4(in_pos,1.0);uv=in_tex0;}
)GLSL";
    const char *fragment=R"GLSL(
uniform float effectTime;uniform float charge;uniform float redMode;
FSIN vec2 uv;
void main(){
 float t=uv.x,x=uv.y*2.0-1.0;
 float drift=sin(t*21.0-effectTime*19.0)*.09+sin(t*43.0+effectTime*27.0)*.035;
 float envelope=smoothstep(0.0,.08,t)*(1.0-smoothstep(.45,1.0,t));
 float width=mix(.66,.15,t)*( .88+.12*sin(t*34.0-effectTime*22.0));
 float d=abs(x-drift*t);
 float glow=exp(-d*d/max(.01,width*width))*envelope;
 float core=exp(-d*d/max(.001,width*width*.10))*envelope;
 float tongue=pow(max(0.0,sin(t*37.0-effectTime*29.0+x*11.0)),4.0);
 float filament=exp(-pow((x-drift*t-.17*sin(t*24.0-effectTime*14.0))/.045,2.0));
 vec3 blue=mix(vec3(.04,.25,1.0),vec3(1.0,.12,.025),redMode);
 // Blue electrical fringe surrounds a broader yellow-white flame body.
 // The warm region breaks into tongues toward the trailing end.
 float flameWidth=width*(.47+.12*sin(t*19.0-effectTime*15.0));
 float fire=exp(-d*d/max(.003,flameWidth*flameWidth))*envelope;
 fire*=.65+.35*tongue;
 float warmth=smoothstep(.02,.32,t);
 vec3 hot=mix(vec3(.85,.95,1.0),vec3(1.0,.92,.65),warmth);
 vec3 flame=mix(vec3(1.0,.82,.12),vec3(1.0,.35,.025),smoothstep(.35,.95,t));
 vec3 light=blue*glow*.4+flame*fire*.85+hot*core*.8;
 light+=mix(blue,flame,warmth)*filament*tongue*envelope*.22;
 float alpha=clamp(max(glow,fire)*charge,0.0,1.0);
 FRAGCOLOR(vec4(light,alpha));
}
)GLSL";
    const char *vs[]={rw::gl3::shaderDecl,rw::gl3::header_vert_src,vertex,nil};
    const char *fs[]={rw::gl3::shaderDecl,rw::gl3::header_frag_src,fragment,nil};
    plasmaShader=rw::gl3::Shader::create(vs,fs);
}
static void DrawWheelPlasma(){
    auto car=Car();
    if(!enabled || !car || !car->bIsVisible || hover || cinematicTravel.active || !circuits ||
       car->GetStatus()==STATUS_WRECKED || CTimer::GetTimeInMilliseconds()<cooldown || coilAlpha<225)return;
    if(!plasmaShader)return;
    auto previous=rw::gl3::im3dOverrideShader;rw::gl3::im3dOverrideShader=plasmaShader;plasmaShader->use();
    glUniform1f(glGetUniformLocation(plasmaShader->program,"effectTime"),CTimer::GetTimeInMilliseconds()*.001f);
    glUniform1f(glGetUniformLocation(plasmaShader->program,"charge"),Bound((coilAlpha-220.f)/35.f,0,1));
    glUniform1f(glGetUniformLocation(plasmaShader->program,"redMode"),variant==3?1.f:0.f);
    const RwRenderState states[]={rwRENDERSTATETEXTURERASTER,rwRENDERSTATEZTESTENABLE,rwRENDERSTATEZWRITEENABLE,rwRENDERSTATEVERTEXALPHAENABLE,rwRENDERSTATESRCBLEND,rwRENDERSTATEDESTBLEND,rwRENDERSTATECULLMODE,rwRENDERSTATEFOGENABLE};
    void *saved[8]={};for(int i=0;i<8;i++)RwRenderStateGet(states[i],&saved[i]);
    const uint32 alphaFunc=rw::GetRenderState(rw::ALPHATESTFUNC);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,nil);RwRenderStateSet(rwRENDERSTATEZTESTENABLE,(void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,(void*)FALSE);RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,(void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,(void*)rwBLENDSRCALPHA);RwRenderStateSet(rwRENDERSTATEDESTBLEND,(void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATECULLMODE,(void*)rwCULLMODECULLNONE);RwRenderStateSet(rwRENDERSTATEFOGENABLE,(void*)FALSE);
    RwIm3DVertex vertices[4*12*6];int count=0;
    for(int wheel=0;wheel<4;wheel++){
        const float side=wheel%2?1.f:-1.f;
        const float wheelY=wheel<2?1.12f:-1.58f;
        const CVector origin=ArcWorld(CVector(side*.93f,wheelY,-.43f),car);
        for(int i=0;i<12;i++){
            const float t0=float(i)/12,t1=float(i+1)/12;
            const float travelSign=-1.f;
            const float droop=wheel<2?-.16f:-.08f;
            const CVector p=origin+car->GetForward()*(travelSign*t0*.95f)+car->GetUp()*(droop*t0);
            const CVector q=origin+car->GetForward()*(travelSign*t1*.95f)+car->GetUp()*(droop*t1);
            CVector across=CrossProduct(q-p,TheCamera.GetPosition()-(p+q)*.5f);
            if(across.MagnitudeSqr()<1.e-8f)continue;across.Normalise();across*=.42f;
            const CVector points[]={p-across,p+across,q+across,q-across};
            const float u[]={t0,t0,t1,t1},v[]={0,1,1,0};const int order[]={0,1,2,0,2,3};
            for(int j:order){auto &out=vertices[count++];RwIm3DVertexSetPos(&out,points[j].x,points[j].y,points[j].z);
                RwIm3DVertexSetRGBA(&out,255,255,255,255);RwIm3DVertexSetU(&out,u[j]);RwIm3DVertexSetV(&out,v[j]);}
        }
    }
    RwImVertexIndex indices[4*12*6];for(int i=0;i<count;i++)indices[i]=i;
    if(count && RwIm3DTransform(vertices,count,nil,rwIM3D_VERTEXXYZ|rwIM3D_VERTEXRGBA|rwIM3D_VERTEXUV)){
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST,indices,count);RwIm3DEnd();}
    for(int i=0;i<8;i++)RwRenderStateSet(states[i],saved[i]);rw::SetRenderState(rw::ALPHATESTFUNC,alphaFunc);
    rw::gl3::im3dOverrideShader=previous;
}
