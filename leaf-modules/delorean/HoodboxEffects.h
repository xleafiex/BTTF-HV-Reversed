#pragma once
// Separate from the roof-emitter arcs: these are small hot particles shed
// from the hood hardware, with inherited velocity and ballistic falloff.
struct HoodSpark { CVector position,velocity; float age,life,width; bool blue; };
static std::vector<HoodSpark> hoodSparks;
static float hoodEmission=0;
static float HoodRandom(){return float(rand())/RAND_MAX;}
static void UpdateHoodboxEffects(CAutomobile *car){
    if(variant!=3 || !car->bIsVisible || cinematicTravel.active || car->GetStatus()==STATUS_WRECKED){
        hoodSparks.clear();hoodEmission=0;return;
    }
    const float dt=Min(.1f,Max(0.f,CTimer::GetTimeStepInSeconds()));
    for(auto i=hoodSparks.begin();i!=hoodSparks.end();){
        i->age+=dt;
        if(i->age>=i->life){i=hoodSparks.erase(i);continue;}
        i->velocity*=expf(-1.8f*dt);
        i->velocity.z-=5.5f*dt;i->position+=i->velocity*dt;++i;
    }
    const uint32 now=CTimer::GetTimeInMilliseconds();
    const float speed=car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND;
    const bool arrivalBurst=coldStarted && now-coldStarted<1000;
    if(!circuits || (!arrivalBurst && (now<cooldown || speed<40.5f))){hoodEmission=0;return;}
    const float charge=arrivalBurst?1.f:Bound((speed-40.5f)/7.6f,0,1);
    hoodEmission+=dt;
    while(hoodEmission>=.04f){
        hoodEmission-=.04f;
        const int count=2+int(charge*4);
        for(int n=0;n<count && hoodSparks.size()<128;++n){
            // Coils.txt's hoodbox footprint, without delayed-CLEO velocity
            // prediction. The native renderer submits these immediately.
            const CVector local(-.2f+HoodRandom()*.4f,1.425f+HoodRandom()*.4f,.35f);
            HoodSpark s;
            s.position=car->GetPosition()+car->GetRight()*local.x+car->GetForward()*local.y+car->GetUp()*local.z;
            s.velocity=car->GetMoveSpeed()*GAME_SPEED_TO_METERS_PER_SECOND+
                car->GetRight()*((HoodRandom()-.5f)*3.f)+car->GetForward()*((HoodRandom()-.5f)*2.f)+car->GetUp()*(.8f+HoodRandom()*2.2f);
            s.age=0;s.life=.10f+HoodRandom()*.28f;s.width=.004f+HoodRandom()*.005f;s.blue=(n==0);
            hoodSparks.push_back(s);
        }
    }
}
static void DrawHoodboxEffects(){
    CAutomobile *car=Car();
    if(!enabled || !car || variant!=3 || !car->bIsVisible || car->GetStatus()==STATUS_WRECKED || cinematicTravel.active || FrontEndMenuManager.m_bMenuActive || hoodSparks.empty())return;
    const RwRenderState states[]={rwRENDERSTATETEXTURERASTER,rwRENDERSTATEZTESTENABLE,rwRENDERSTATEZWRITEENABLE,rwRENDERSTATEVERTEXALPHAENABLE,rwRENDERSTATESRCBLEND,rwRENDERSTATEDESTBLEND,rwRENDERSTATECULLMODE,rwRENDERSTATEFOGENABLE};
    void *saved[8]={};for(int i=0;i<8;++i)RwRenderStateGet(states[i],&saved[i]);
    const auto alphaFunc=rw::GetRenderState(rw::ALPHATESTFUNC);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
    auto shader=rw::gl3::im3dOverrideShader;rw::gl3::im3dOverrideShader=nil;
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,nil);RwRenderStateSet(rwRENDERSTATEZTESTENABLE,(void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,(void*)FALSE);RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,(void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,(void*)rwBLENDSRCALPHA);RwRenderStateSet(rwRENDERSTATEDESTBLEND,(void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATECULLMODE,(void*)rwCULLMODECULLNONE);RwRenderStateSet(rwRENDERSTATEFOGENABLE,(void*)FALSE);
    std::vector<RwIm3DVertex> vertices;vertices.reserve(hoodSparks.size()*12);
    for(int layer=0;layer<2;++layer)for(const auto &s:hoodSparks){
        const float life=1.f-s.age/s.life;
        CVector trail=s.velocity*.012f;
        if(trail.Magnitude()>.22f){trail.Normalise();trail*=.22f;}
        if(trail.MagnitudeSqr()<.000001f)trail=car->GetUp()*.015f;
        const CVector p=s.position,q=p-trail;
        CVector side=CrossProduct(p-q,TheCamera.GetPosition()-p);
        if(side.MagnitudeSqr()<1.e-10f)side=TheCamera.GetRight();else side.Normalise();
        side*=s.width*(layer?1.f:3.f);
        const CVector corners[]={q-side,q+side,p+side,p-side};
        const int order[]={0,1,2,0,2,3};
        for(int index:order){
            RwIm3DVertex v;RwIm3DVertexSetPos(&v,corners[index].x,corners[index].y,corners[index].z);
            RwIm3DVertexSetRGBA(&v,s.blue?(layer?215:75):255,s.blue?180:uint8(90+150*life),s.blue?255:uint8(layer?60+150*life:25),uint8((index<2?0:layer?245:65)*life));
            vertices.push_back(v);
        }
    }
    if(!vertices.empty() && RwIm3DTransform(vertices.data(),vertices.size(),nil,rwIM3D_VERTEXXYZ|rwIM3D_VERTEXRGBA)){
        RwImVertexIndex indices[1536];
        for(size_t i=0;i<vertices.size();++i)indices[i]=RwImVertexIndex(i);
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST,indices,vertices.size());RwIm3DEnd();
    }
    for(int i=0;i<8;++i)RwRenderStateSet(states[i],saved[i]);rw::gl3::im3dOverrideShader=shader;
    rw::SetRenderState(rw::ALPHATESTFUNC,alphaFunc);
}
