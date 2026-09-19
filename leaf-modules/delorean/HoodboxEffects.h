#pragma once
// Separate from the roof-emitter arcs: these are small hot particles shed
// from the hood hardware, with inherited velocity and ballistic falloff.
struct HoodSpark { CVector position,velocity; float age,life,width; bool blue; };
struct HoodEmitter { RwFrame *frame; CVector local; };
static std::vector<HoodEmitter> hoodEmitters;
static std::vector<HoodSpark> hoodSparks;
static float hoodEmission=0;
static float HoodRandom(){return float(rand())/RAND_MAX;}
static void InitialiseHoodboxEmitters(CAutomobile *car){
    hoodEmitters.clear();hoodSparks.clear();hoodEmission=0;
    // Resolve emitting points against the actual upward-facing hoodbox
    // triangles. Store them in the part's frame so opening the bonnet also
    // moves the discharge, rather than leaving it floating over the chassis.
    struct Query { CAutomobile *car; } query={car};
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,[](RpAtomic *a,void *data)->RpAtomic*{
        const char *name=GetFrameNodeName(RpAtomicGetFrame(a));
        if(!name || strcmp(name,"bonnetbttf3") || !a->geometry)return a;
        CAutomobile *car=static_cast<Query*>(data)->car;
        auto *g=a->geometry;RwFrame *frame=RpAtomicGetFrame(a);
        std::vector<CVector> local(g->numVertices);
        for(int i=0;i<g->numVertices;++i){
            RwV3d world;RwV3dTransformPoints(&world,&g->morphTargets[0].vertices[i],1,RwFrameGetLTM(frame));
            local[i]=ArcLocal(world,car);
        }
        for(int row=0;row<3;++row)for(int side=-1;side<=1;side+=2){
            const float x=side*.13f,y=1.46f+row*.15f;
            float best=-1.e10f;CVector point;
            for(int t=0;t<g->numTriangles;++t){
                const auto &tri=g->triangles[t];
                const CVector &p=local[tri.v[0]],&q=local[tri.v[1]],&r=local[tri.v[2]];
                const float det=(q.y-r.y)*(p.x-r.x)+(r.x-q.x)*(p.y-r.y);
                if(fabsf(det)<1.e-7f)continue;
                const float u=((q.y-r.y)*(x-r.x)+(r.x-q.x)*(y-r.y))/det;
                const float v=((r.y-p.y)*(x-r.x)+(p.x-r.x)*(y-r.y))/det;
                const float w=1-u-v;
                if(u<0 || v<0 || w<0)continue;
                const float z=u*p.z+v*q.z+w*r.z;
                if(z<=best)continue;
                best=z;
                const auto &a=g->morphTargets[0].vertices[tri.v[0]],&b=g->morphTargets[0].vertices[tri.v[1]],&c=g->morphTargets[0].vertices[tri.v[2]];
                point=CVector(a.x*u+b.x*v+c.x*w,a.y*u+b.y*v+c.y*w,a.z*u+b.z*v+c.z*w);
            }
            if(best>-1.e9f)hoodEmitters.push_back({frame,point});
        }
        return a;
    },&query);
    char message[96];snprintf(message,sizeof(message),"Hoodbox surface emitters: %u/6",unsigned(hoodEmitters.size()));Log(message);
}
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
    hoodEmission-=dt;
    if(hoodEmission<=0){
        hoodEmission=.025f+HoodRandom()*(.11f-.06f*charge);
        const int count=3+int(charge*9)+rand()%5;
        const unsigned emitter=hoodEmitters.empty()?0:rand()%hoodEmitters.size();
        for(int n=0;n<count && hoodSparks.size()<128;++n){
            // Coils.txt's hoodbox footprint, without delayed-CLEO velocity
            // prediction. The native renderer submits these immediately.
            const CVector local(-.2f+HoodRandom()*.4f,1.425f+HoodRandom()*.4f,.35f);
            HoodSpark s;
            s.position=car->GetPosition()+car->GetRight()*local.x+car->GetForward()*local.y+car->GetUp()*local.z;
            if(!hoodEmitters.empty()){
                RwV3d world;const auto &e=hoodEmitters[emitter];
                RwV3dTransformPoints(&world,&e.local,1,RwFrameGetLTM(e.frame));
                s.position=CVector(world)+car->GetUp()*.012f;
            }
            s.velocity=car->GetMoveSpeed()*GAME_SPEED_TO_METERS_PER_SECOND+
                car->GetRight()*((HoodRandom()-.5f)*5.f)+car->GetForward()*((HoodRandom()-.5f)*3.f)+car->GetUp()*(.5f+HoodRandom()*3.5f);
            s.age=0;s.life=.055f+HoodRandom()*.25f;s.width=.002f+HoodRandom()*.003f;s.blue=(n==0);
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
    static std::vector<RwIm3DVertex> vertices;vertices.clear();vertices.reserve(128*48);
    for(int layer=0;layer<2;++layer)for(const auto &s:hoodSparks){
        const float life=1.f-s.age/s.life;
        CVector trail=s.velocity*.004f;
        if(trail.Magnitude()>.16f){trail.Normalise();trail*=.16f;}
        if(trail.MagnitudeSqr()<.000001f)trail=car->GetUp()*.015f;
        const CVector p=s.position,q=p-trail;
        CVector side=CrossProduct(p-q,TheCamera.GetPosition()-p);
        if(side.MagnitudeSqr()<1.e-10f)side=TheCamera.GetRight();else side.Normalise();
        side*=s.width*(layer?1.f:3.f);
        const float offsets[]={-1.f,-.4f,0.f,.4f,1.f},feather[]={0,.65f,1,.65f,0};
        for(int band=0;band<4;++band){
        const CVector corners[]={q+side*offsets[band],q+side*offsets[band+1],p+side*offsets[band+1],p+side*offsets[band]};
        const float opacity[]={0,0,feather[band+1],feather[band]};
        const int order[]={0,1,2,0,2,3};
        for(int index:order){
            RwIm3DVertex v;RwIm3DVertexSetPos(&v,corners[index].x,corners[index].y,corners[index].z);
            RwIm3DVertexSetRGBA(&v,s.blue?(layer?215:75):255,s.blue?180:uint8(90+150*life),s.blue?255:uint8(layer?60+150*life:25),uint8((layer?245:65)*life*life*opacity[index]));
            vertices.push_back(v);
        }
        }
    }
    if(!vertices.empty() && RwIm3DTransform(vertices.data(),vertices.size(),nil,rwIM3D_VERTEXXYZ|rwIM3D_VERTEXRGBA)){
        RwImVertexIndex indices[6144];
        for(size_t i=0;i<vertices.size();++i)indices[i]=RwImVertexIndex(i);
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST,indices,vertices.size());RwIm3DEnd();
    }
    for(int i=0;i<8;++i)RwRenderStateSet(states[i],saved[i]);rw::gl3::im3dOverrideShader=shader;
    rw::SetRenderState(rw::ALPHATESTFUNC,alphaFunc);
}
