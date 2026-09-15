#pragma once
static rw::gl3::Shader *fireTrailShader=nil;
static RwTexture *fireTrailTexture=nil;
static constexpr float fireTrailLength=25.f;
static GLuint fireDepthTexture=0,fireDepthFramebuffer=0;
static int fireDepthWidth=0,fireDepthHeight=0,fireDepthFormat=0;
static void DestroyFireVolumeResources(){
    if(fireDepthTexture)glDeleteTextures(1,&fireDepthTexture);
    if(fireDepthFramebuffer)glDeleteFramebuffers(1,&fireDepthFramebuffer);
    fireDepthTexture=fireDepthFramebuffer=0;fireDepthWidth=fireDepthHeight=fireDepthFormat=0;
}
static void InitialiseFireTrailShader(){
    fireTrailTexture=LoadIceSurfaceTexture(root+"/effects/fire-burst.rgba");
    if(!fireTrailTexture)return;
    fireTrailTexture->setFilter(rw::Texture::LINEAR);
    fireTrailTexture->setAddressU(rw::Texture::CLAMP);fireTrailTexture->setAddressV(rw::Texture::CLAMP);
    const char *vertex=R"GLSL(
VSIN(ATTRIB_POS) vec3 in_pos;
flat VSOUT mat4 inverseViewProjection;
void main(){gl_Position=vec4(in_pos.xy,0.0,1.0);inverseViewProjection=inverse(u_proj*u_view);}
)GLSL";
    const char *fragment=R"GLSL(
uniform sampler2D tex0;uniform sampler2D sceneDepth;
uniform vec4 viewport;
uniform vec3 trailOrigin,trailRight,trailForward,trailUp;
uniform float trailAge,previewMode;uniform float groundHeight[26];
flat FSIN mat4 inverseViewProjection;
vec4 frame(float index,vec2 p){
 vec2 cell=vec2(mod(index,16.0),floor(index/16.0));
 vec2 inset=vec2(.5/128.0,.5/256.0);
 vec2 q=(cell+mix(inset,1.0-inset,clamp(p,0.0,1.0)))/16.0;
 // Ray steps diverge between neighbouring pixels. Implicit derivatives in
 // that loop select undefined mip levels, including other atlas frames.
 return textureLod(tex0,vec2(q.x,1.0-q.y),0.0);
}
vec4 flame(float variant,float age,vec2 uv){
 float f=clamp(age,0.0,1.0)*63.0,offset=variant*64.0;
 float cycle=mod(trailAge*22.0+variant*5.0,20.0);
 if(previewMode>.5)f=cycle+4.0;
 vec4 gas=mix(frame(offset+floor(f),uv),frame(offset+min(63.0,floor(f)+1.0),uv),fract(f));
 if(previewMode>.5 && cycle>16.0){
  float next=cycle-16.0;
  vec4 restart=mix(frame(offset+floor(next),uv),frame(offset+floor(next)+1.0,uv),fract(next));
  gas=mix(gas,restart,smoothstep(16.0,20.0,cycle));
 }
 return gas;
}
float hash(float n){return fract(sin(n*127.1)*43758.5453);}
float ground(float y){float t=clamp(y,0.0,25.0);int i=min(24,int(t));return mix(groundHeight[i],groundHeight[i+1],t-float(i));}
vec3 local(vec3 p){p-=trailOrigin;return vec3(dot(p,trailRight),dot(p,trailForward),dot(p,trailUp));}
bool slab(float origin,float direction,float low,float high,inout float enter,inout float leave){
 if(abs(direction)<.000001)return origin>=low && origin<=high;
 float a=(low-origin)/direction,b=(high-origin)/direction;
 enter=max(enter,min(a,b));leave=min(leave,max(a,b));return leave>enter;
}
vec4 field(vec3 p){
 float age=previewMode>.5?.55:trailAge-p.y*.026;
 if(age<=0.0 || age>=1.78)return vec4(0);
 float z=p.z-ground(p.y);
 if(z<=0.0 || z>=1.05 || abs(p.x)>.42)return vec4(0);
 // Intersect animated gas cross-sections in a warped 3D domain. These
 // define a density field, not geometry or texture planes facing the eye.
 float sway=sin(p.y*10.0+z*6.0-trailAge*5.0)*z*.045;
 float x=p.x+sway;
 float s=p.y/.45+sin(x*9.0+z*5.0-trailAge*3.0)*.11;
 float cell=floor(s*.5),variant=floor(hash(cell+17.0)*4.0);
 float h=z/(.82+.13*sin(p.y*4.1));
 float phase=clamp(age/1.78,0.0,1.0);
 // Sample the active gas region along the track, avoiding the empty margins
 // of the source atlas that would leave a row of separate candle-like tufts.
 float along=.5+(1.0-abs(mod(s,2.0)-1.0)-.5)*.48;
 vec4 a=flame(variant,phase,vec2(along,1.0-h));
 vec4 b=flame(mod(variant+1.0,4.0),phase,vec2(x/.84+.5,1.0-h));
 float ea=max(max(a.r,a.g),a.b),eb=max(max(b.r,b.g),b.b);
 float density=sqrt(max(0.0,ea*eb));
 float edge=(1.0-smoothstep(.31,.42,abs(p.x)))*smoothstep(0.0,.025,z);
 edge*=smoothstep(0.0,.12,p.y)*(1.0-smoothstep(24.88,25.0,p.y));
 edge*=smoothstep(0.0,.09,age)*(1.0-smoothstep(1.55,1.78,age));
 vec3 colour=(a.rgb+b.rgb)/max(.015,ea+eb);
 return vec4(colour,density*edge*10.0);
}
void main(){
 vec2 screen=(gl_FragCoord.xy-viewport.xy)/viewport.zw;
 vec2 ndc=screen*2.0-1.0;
 vec4 n=inverseViewProjection*vec4(ndc,-1,1),f=inverseViewProjection*vec4(ndc,1,1);
 vec3 nearWorld=n.xyz/n.w,farWorld=f.xyz/f.w;
 vec3 ro=local(nearWorld),rd=normalize(local(farWorld)-ro);
 float low=groundHeight[0],high=low;
 for(int i=1;i<26;i++){low=min(low,groundHeight[i]);high=max(high,groundHeight[i]);}
 float enter=0.0,leave=100000.0;
 if(!slab(ro.x,rd.x,-.42,.42,enter,leave) || !slab(ro.y,rd.y,0.0,25.0,enter,leave) ||
    !slab(ro.z,rd.z,low,high+1.05,enter,leave))discard;
 float depth=texture(sceneDepth,screen).r;
 float sceneDistance=leave+1.0;
 if(depth<.9999999){
  vec4 scene=inverseViewProjection*vec4(ndc,depth*2.0-1.0,1);
  if(abs(scene.w)>.0000001)sceneDistance=dot(local(scene.xyz/scene.w)-ro,rd);
 }
 leave=min(leave,sceneDistance);
 if(leave<=enter)discard;
 // Integrate extinction along the ray. Opacity uses step length, preventing
 // the abrupt angle-dependent brightness of intersecting flame cards.
 // A long end-on ray needs more samples, not larger gaps that skip flames.
 int steps=int(clamp(ceil((leave-enter)/.035),12.0,768.0));
 float ds=(leave-enter)/float(steps),t=enter+ds*.5;
 vec4 result=vec4(0);
 for(int i=0;i<768;i++){
  if(i>=steps || result.a>.985)break;
  vec4 gas=field(ro+rd*t);
  float soft=clamp((sceneDistance-t)/.10,0.0,1.0);
  float alpha=1.0-exp(-gas.a*ds*soft);
  result.rgb+=(1.0-result.a)*gas.rgb*alpha;
  result.a+=(1.0-result.a)*alpha;t+=ds;
 }
 if(result.a<.002)discard;
 FRAGCOLOR(result);
}
)GLSL";
    const char *vs[]={rw::gl3::shaderDecl,rw::gl3::header_vert_src,vertex,nil};
    const char *fs[]={rw::gl3::shaderDecl,rw::gl3::header_frag_src,fragment,nil};
    fireTrailShader=rw::gl3::Shader::create(vs,fs);
}
static DepartureTrail previewFireTrail;
static bool SetFireTrailPreview(bool on){
    if(!on){previewFireTrail.active=false;return true;}
    if(!enabled || !fireTrailShader || !FindPlayerPed())return false;
    auto player=FindPlayerPed();CVector forward=player->GetForward();forward.z=0;
    if(forward.MagnitudeSqr()<1.e-6f)forward=CVector(0,1,0);else forward.Normalise();
    CVector across(forward.y,-forward.x,0),origin=player->GetPosition()+forward*4.f;
    previewFireTrail={};previewFireTrail.active=true;previewFireTrail.grounded=true;
    previewFireTrail.started=CTimer::GetTimeInMilliseconds();
    previewFireTrail.left=origin+across*.98f;previewFireTrail.right=origin-across*.98f;previewFireTrail.delta=forward*.5f;
    return true;
}
static bool CaptureFireDepth(const GLint *vp){
    // GL errors belong to the context, not the next operation that reads
    // them. A preceding material error must not suppress a successful copy.
    GLenum prior=GL_NO_ERROR;unsigned priorCount=0;
    while((prior=glGetError())!=GL_NO_ERROR){
        static bool reported=false;
        if(!reported){char msg[128];snprintf(msg,sizeof(msg),"Fire volume: earlier GL error 0x%x isolated before depth capture",unsigned(prior));Log(msg);reported=true;}
        if(++priorCount>=16)break;
    }
    GLint read=0,draw=0,active=0,binding=0,depthBits=0,stencilBits=0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);
    // Core GL profiles reject legacy GL_DEPTH_BITS/GL_STENCIL_BITS queries.
    // Read the actual attached buffer sizes so MSAA resolves match formats.
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,draw?GL_DEPTH_ATTACHMENT:GL_DEPTH,GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,&depthBits);
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,draw?GL_STENCIL_ATTACHMENT:GL_STENCIL,GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,&stencilBits);
    glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(GL_TEXTURE1);glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding);
    const GLenum format=stencilBits?(depthBits>24?GL_DEPTH32F_STENCIL8:GL_DEPTH24_STENCIL8):
        (depthBits>24?GL_DEPTH_COMPONENT32F:depthBits>16?GL_DEPTH_COMPONENT24:GL_DEPTH_COMPONENT16);
    if(!fireDepthTexture || fireDepthWidth!=vp[2] || fireDepthHeight!=vp[3] || fireDepthFormat!=int(format)){
        DestroyFireVolumeResources();fireDepthWidth=vp[2];fireDepthHeight=vp[3];fireDepthFormat=format;
        glGenTextures(1,&fireDepthTexture);glBindTexture(GL_TEXTURE_2D,fireDepthTexture);
        glTexImage2D(GL_TEXTURE_2D,0,format,vp[2],vp[3],0,stencilBits?GL_DEPTH_STENCIL:GL_DEPTH_COMPONENT,
            stencilBits?(depthBits>24?GL_FLOAT_32_UNSIGNED_INT_24_8_REV:GL_UNSIGNED_INT_24_8):GL_FLOAT,nil);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glGenFramebuffers(1,&fireDepthFramebuffer);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fireDepthFramebuffer);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,stencilBits?GL_DEPTH_STENCIL_ATTACHMENT:GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,fireDepthTexture,0);
        glDrawBuffer(GL_NONE);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER,draw);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fireDepthFramebuffer);
    bool valid=glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
    if(valid){
        const GLboolean scissor=glIsEnabled(GL_SCISSOR_TEST);if(scissor)glDisable(GL_SCISSOR_TEST);
        glBlitFramebuffer(vp[0],vp[1],vp[0]+vp[2],vp[1]+vp[3],0,0,vp[2],vp[3],GL_DEPTH_BUFFER_BIT,GL_NEAREST);
        const GLenum error=glGetError();valid=error==GL_NO_ERROR;
        if(!valid){static bool reported=false;if(!reported){char msg[192];snprintf(msg,sizeof(msg),"Fire volume depth error 0x%x: framebuffer=%d depth=%d stencil=%d format=0x%x size=%dx%d",unsigned(error),draw,depthBits,stencilBits,unsigned(format),vp[2],vp[3]);Log(msg);reported=true;}}
        if(scissor)glEnable(GL_SCISSOR_TEST);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER,read);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);
    glBindTexture(GL_TEXTURE_2D,binding);glActiveTexture(active);return valid;
}
static void DrawFireTrailSet(const DepartureTrail &trail,int slot){
    if(!trail.active)return;
    const float age=float(CTimer::GetTimeInMilliseconds()-trail.started)*.001f;
    if(slot==0 && age>2.43f)return;
    CVector forward=trail.delta;if(trail.grounded)forward.z=0;
    if(forward.MagnitudeSqr()<1.e-6f)forward=CVector(0,1,0);else forward.Normalise();
    CVector across(forward.y,-forward.x,0);
    if(across.MagnitudeSqr()<1.e-6f)across=CVector(1,0,0);else across.Normalise();
    CVector up=CrossProduct(across,forward);up.Normalise();
    struct Cache{uint32 stamp=~uint32(0);CVector origin=CVector(1.e10f,0,0);float ground[2][26];};static Cache caches[2];auto &cache=caches[slot];
    if(cache.stamp!=trail.started || (cache.origin-trail.left).MagnitudeSqr()>.001f){
        cache.stamp=trail.started;cache.origin=trail.left;
        for(int side=0;side<2;side++)for(int i=0;i<26;i++){
            CVector origin=side?trail.right:trail.left,p=origin+forward*float(i);float z=p.z;
            if(trail.grounded){bool found=false;float hit=CWorld::FindGroundZFor3DCoord(p.x,p.y,p.z+3.f,&found);if(found)z=hit;}
            cache.ground[side][i]=trail.grounded?z+.012f-origin.z:.012f;
        }
    }
    auto program=fireTrailShader->program;
    glUniform3f(glGetUniformLocation(program,"trailRight"),across.x,across.y,across.z);
    glUniform3f(glGetUniformLocation(program,"trailForward"),forward.x,forward.y,forward.z);
    glUniform3f(glGetUniformLocation(program,"trailUp"),up.x,up.y,up.z);
    glUniform1f(glGetUniformLocation(program,"trailAge"),age);
    glUniform1f(glGetUniformLocation(program,"previewMode"),slot==1?1.f:0.f);
    RwIm3DVertex vertices[6];RwImVertexIndex indices[]={0,1,2,3,4,5};
    const float corners[6][2]={{-1,-1},{1,-1},{1,1},{-1,-1},{1,1},{-1,1}};
    for(int i=0;i<6;i++){RwIm3DVertexSetPos(&vertices[i],corners[i][0],corners[i][1],0);RwIm3DVertexSetRGBA(&vertices[i],255,255,255,255);RwIm3DVertexSetU(&vertices[i],0);RwIm3DVertexSetV(&vertices[i],0);}
    bool rightFirst=(trail.right-TheCamera.GetPosition()).MagnitudeSqr()>(trail.left-TheCamera.GetPosition()).MagnitudeSqr();
    for(int n=0;n<2;n++){
        int side=rightFirst?1-n:n;CVector origin=side?trail.right:trail.left;
        glUniform3f(glGetUniformLocation(program,"trailOrigin"),origin.x,origin.y,origin.z);
        glUniform1fv(glGetUniformLocation(program,"groundHeight"),26,cache.ground[side]);
        if(RwIm3DTransform(vertices,6,nil,rwIM3D_VERTEXXYZ|rwIM3D_VERTEXRGBA|rwIM3D_VERTEXUV)){
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST,indices,6);RwIm3DEnd();}
    }
}
static void DrawFireTrails(){
    if(!enabled || !fireTrailShader || (!departureTrail.active && !previewFireTrail.active))return;
    if(previewFireTrail.active && !FindPlayerPed())previewFireTrail.active=false;
    GLint vp[4];glGetIntegerv(GL_VIEWPORT,vp);
    if(!CaptureFireDepth(vp)){static bool logged=false;if(!logged){Log("Fire volume: scene depth resolve failed");logged=true;}return;}
    GLint active=0,depthBinding=0;glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(GL_TEXTURE1);glGetIntegerv(GL_TEXTURE_BINDING_2D,&depthBinding);
    glBindTexture(GL_TEXTURE_2D,fireDepthTexture);glActiveTexture(GL_TEXTURE0);
    auto previous=rw::gl3::im3dOverrideShader;rw::gl3::im3dOverrideShader=fireTrailShader;fireTrailShader->use();
    glUniform1i(glGetUniformLocation(fireTrailShader->program,"tex0"),0);glUniform1i(glGetUniformLocation(fireTrailShader->program,"sceneDepth"),1);
    glUniform4f(glGetUniformLocation(fireTrailShader->program,"viewport"),vp[0],vp[1],vp[2],vp[3]);
    const RwRenderState states[]={rwRENDERSTATETEXTURERASTER,rwRENDERSTATEZTESTENABLE,rwRENDERSTATEZWRITEENABLE,rwRENDERSTATEVERTEXALPHAENABLE,rwRENDERSTATESRCBLEND,rwRENDERSTATEDESTBLEND,rwRENDERSTATECULLMODE,rwRENDERSTATEFOGENABLE};
    void *saved[8]={};for(int i=0;i<8;i++)RwRenderStateGet(states[i],&saved[i]);
    const auto alphaFunc=rw::GetRenderState(rw::ALPHATESTFUNC);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,RwTextureGetRaster(fireTrailTexture));RwRenderStateSet(rwRENDERSTATEZTESTENABLE,(void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,(void*)FALSE);RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,(void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,(void*)rwBLENDONE);RwRenderStateSet(rwRENDERSTATEDESTBLEND,(void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATECULLMODE,(void*)rwCULLMODECULLNONE);RwRenderStateSet(rwRENDERSTATEFOGENABLE,(void*)FALSE);
    DrawFireTrailSet(departureTrail,0);DrawFireTrailSet(previewFireTrail,1);
    for(int i=0;i<8;i++)RwRenderStateSet(states[i],saved[i]);rw::SetRenderState(rw::ALPHATESTFUNC,alphaFunc);
    rw::gl3::im3dOverrideShader=previous;glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,depthBinding);
    // Restoring RW's stage-zero raster above leaves its active-stage cache at
    // zero. Match that cache after touching the private depth unit directly.
    glActiveTexture(GL_TEXTURE0);
}
