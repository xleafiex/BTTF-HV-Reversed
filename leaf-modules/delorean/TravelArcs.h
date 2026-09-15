#pragma once

// Short, moving world-space ribbons. No velocity-based particle stretching:
// every point is evaluated relative to the current car transform.
struct TravelArc { uint32 started; CVector points[6]; float seed, duration, stop, width, trail; };
static std::vector<TravelArc> travelArcs;
static uint32 nextTravelArc=0;

static CVector ArcLocal(const CVector &world,CAutomobile *car){
    const CVector d=world-car->GetPosition();
    return CVector(DotProduct(d,car->GetRight()),DotProduct(d,car->GetForward()),DotProduct(d,car->GetUp()));
}
static CVector ArcWorld(const CVector &p,CAutomobile *car){
    return car->GetPosition()+car->GetRight()*p.x+car->GetForward()*p.y+car->GetUp()*p.z;
}
static CVector ArcEmitter(CAutomobile *car){
    struct Query { bool found; CVector world; } q={false,CVector(0,0,0)};
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,[](RpAtomic *a,void *data)->RpAtomic*{
        auto &q=*static_cast<Query*>(data);const char *n=GetFrameNodeName(RpAtomicGetFrame(a));
        if(n && !strcmp(n,"fluxemitteron") && a->geometry){
            auto center=a->geometry->morphTargets[0].calculateBoundingSphere().center;
            RwV3d world;RwV3dTransformPoints(&world,&center,1,RwFrameGetLTM(RpAtomicGetFrame(a)));
            q.world=world;q.found=true;
        }return a;
    },&q);
    return q.found?ArcLocal(q.world,car):CVector(0,-.68f,.765f);
}
static void UpdateTravelArcs(CAutomobile *car,bool active,uint32 now){
    if(!active || cinematicTravel.active){travelArcs.clear();nextTravelArc=now;return;}
    for(auto i=travelArcs.begin();i!=travelArcs.end();)if(now-i->started>=i->duration*i->stop+160)i=travelArcs.erase(i);else ++i;
    if(now<nextTravelArc)return;
    const float mph=car->GetMoveSpeed().Magnitude()*GAME_SPEED_TO_METERS_PER_SECOND*(88.0f/48.1f);
    const float charge=Bound((mph-65.f)/23.f,0,1);
    nextTravelArc=now+uint32(75.f-35.f*charge)+rand()%55;
    const int burst=1+(rand()%100<int(35+45*charge)?1:0)+(charge>.8f && rand()%3==0?1:0);
    const CVector emitter=ArcEmitter(car);
    for(int emitted=0;emitted<burst && travelArcs.size()<32;emitted++){
    TravelArc a;a.started=now;a.seed=float(rand()%1000);
    a.points[0]=emitter;
    a.duration=200.0f+rand()%501;
    a.stop=rand()%3==0?.25f+float(rand()%46)/100.0f:1.0f;
    a.width=.7f+float(rand()%101)/100.0f;
    a.trail=.22f+float(rand()%34)/100.f;
    const float side=(rand()&1)?1.0f:-1.0f;
    const bool rearward=(rand()%3)!=0;
    a.points[1]=a.points[0]+CVector(side*.3f,rearward?-.5f:.6f,.18f);
    a.points[2]=CVector(side*1.05f,rearward?-1.9f:.15f,.55f);
    a.points[3]=CVector(side*1.17f,1.05f,.25f);
    a.points[4]=CVector(side*.65f,2.35f,.15f);
    a.points[5]=CVector(0,3.3f,.3f);
    const int route=rand()%4;
    if(route==0){
        // Miss the dome, turn around the front corner, then peel backward
        // along the opposite side instead of converging at the nose.
        a.points[1]=a.points[0]+CVector(side*.45f,.7f,.25f);
        a.points[2]=CVector(side*1.3f,2.8f,.6f);
        a.points[3]=CVector(side*1.55f,1.0f,.85f);
        a.points[4]=CVector(side*1.4f,-1.8f,.45f);
        a.points[5]=CVector(side*.85f,-3.4f,.2f);
    }else if(route==1){
        // Dip beside the bonnet and climb into the upper dome.
        a.points[1]=a.points[0]+CVector(side*.6f,-.4f,.25f);
        a.points[2]=CVector(side*1.25f,.3f,.1f);
        a.points[3]=CVector(side*.9f,1.9f,-.15f);
        a.points[4]=CVector(side*.4f,2.9f,.65f);
        a.points[5]=CVector(0,3.1f,1.55f);
    }else if(route==2){
        a.points[1]=a.points[0]+CVector(side*.25f,-.5f,.65f);
        a.points[2]=CVector(side*.9f,-1.6f,1.65f);
        a.points[3]=CVector(side*1.15f,.4f,1.9f);
        a.points[4]=CVector(side*.5f,2.2f,1.65f);
        a.points[5]=CVector(0,3.1f,1.1f);
    }
    // Break up repeated silhouettes while retaining clearance around the car.
    const float spread=.85f+float(rand()%36)/100.f;
    for(int i=1;i<6;i++){
        a.points[i].x*=spread;
        a.points[i].y+=float(rand()%41-20)/100.f;
        a.points[i].z+=float(rand()%41-20)/100.f;
    }
    // Secondary discharges from donor coil positions; the roof emitter remains
    // the dominant source and all routes still follow the current car transform.
    if(emitted>0 && rand()%3==0){
        a.points[0]=CVector(side*.95f,-.9f,.35f);
        a.points[1]=CVector(side*1.2f,-.5f,.65f);
        a.width*=.7f;
    }
    // Roughly one third fire from the emitter's forward face directly into
    // the dome, with only small electrical kinks rather than a rearward loop.
    if(rand()%3==0){
        a.points[0]=emitter+CVector(0,.12f,0);
        const CVector target(float(rand()%41-20)/100.f,3.3f,.45f+float(rand()%56)/100.f);
        for(int i=1;i<6;i++)a.points[i]=a.points[0]+(target-a.points[0])*(float(i)/5.f);
        a.duration=180.f+rand()%181;
        a.trail=.35f+float(rand()%21)/100.f;
    }
    travelArcs.push_back(a);
    }
}
static CVector SampleTravelArc(const TravelArc &a,float t){
    float s=Bound(t,0,1)*5;int i=Min(4,int(s));float u=s-i;
    const CVector &p0=a.points[Max(0,i-1)],&p1=a.points[i],&p2=a.points[i+1],&p3=a.points[Min(5,i+2)];
    CVector p=(p1*2+(p2-p0)*u+(p0*2-p1*5+p2*4-p3)*(u*u)+(-p0+p1*3-p2*3+p3)*(u*u*u))*.5f;
    // Interpolated irregular knots give angular electrical kinks, rather than
    // a smooth sine-wave cable. The seed is stable throughout the discharge.
    const float knot=t*24.f;const float cell=floorf(knot),fraction=knot-cell;
    const float jitter=.085f*sinf(t*PI);
    p.x+=(sinf(cell*127.1f+a.seed)*(1-fraction)+sinf((cell+1)*127.1f+a.seed)*fraction)*jitter;
    p.z+=(sinf(cell*311.7f+a.seed)*(1-fraction)+sinf((cell+1)*311.7f+a.seed)*fraction)*jitter;
    return p;
}
static void DrawTravelArcs(){
    CAutomobile *car=Car();if(!enabled || !car || car->GetStatus()==STATUS_WRECKED || cinematicTravel.active || travelArcs.empty())return;
    const RwRenderState states[]={rwRENDERSTATETEXTURERASTER,rwRENDERSTATEZTESTENABLE,rwRENDERSTATEZWRITEENABLE,rwRENDERSTATEVERTEXALPHAENABLE,rwRENDERSTATESRCBLEND,rwRENDERSTATEDESTBLEND,rwRENDERSTATECULLMODE,rwRENDERSTATEFOGENABLE};
    void *saved[8]={};for(int i=0;i<8;i++)RwRenderStateGet(states[i],&saved[i]);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,nil);RwRenderStateSet(rwRENDERSTATEZTESTENABLE,(void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,(void*)FALSE);RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,(void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,(void*)rwBLENDSRCALPHA);RwRenderStateSet(rwRENDERSTATEDESTBLEND,(void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATECULLMODE,(void*)rwCULLMODECULLNONE);RwRenderStateSet(rwRENDERSTATEFOGENABLE,(void*)FALSE);
    const uint32 now=CTimer::GetTimeInMilliseconds();
    static std::vector<RwIm3DVertex> batches[2];
    for(auto &batch:batches)batch.clear();
    for(const auto &a:travelArcs){
        const float age=float(now-a.started),end=a.duration*a.stop;
        const float fade=Bound((end+160-age)/160,0,1);
        float head=Bound(age/a.duration,0,a.stop),tail=Max(0.0f,age/a.duration-a.trail);
        if(tail>=1 || head<=tail)continue;
        for(int layer=0;layer<2;layer++){
            auto &vertices=batches[layer];
            for(int n=0;n<48;n++){
                float t0=tail+(head-tail)*n/48,t1=tail+(head-tail)*(n+1)/48;
                CVector p=ArcWorld(SampleTravelArc(a,t0),car),q=ArcWorld(SampleTravelArc(a,t1),car);
                CVector side=CrossProduct(q-p,TheCamera.GetPosition()-(p+q)*.5f);
                if(side.MagnitudeSqr()<1.e-10f)continue;side.Normalise();
                const float progress0=float(n)/48,progress1=float(n+1)/48;
                const float taper0=sqrtf(progress0)*Min(1.f,(1-progress0)*18.f);
                const float taper1=sqrtf(progress1)*Min(1.f,(1-progress1)*18.f);
                const float width=(layer==0?.10f:.026f)*a.width;
                const float pulse=.8f+.2f*sinf(age*.045f+a.seed);
                const float offsets[]={-1.f,-.45f,0.f,.45f,1.f};
                const float feather[]={0.f,.55f,1.f,.55f,0.f};
                for(int band=0;band<4;band++){
                    const CVector corners[]={p+side*(width*taper0*offsets[band]),p+side*(width*taper0*offsets[band+1]),q+side*(width*taper1*offsets[band+1]),q+side*(width*taper1*offsets[band])};
                    const float opacity[]={feather[band]*taper0,feather[band+1]*taper0,feather[band+1]*taper1,feather[band]*taper1};
                    const int order[]={0,1,2,0,2,3};
                    for(int j:order){
                        RwIm3DVertex v;RwIm3DVertexSetPos(&v,corners[j].x,corners[j].y,corners[j].z);
                        RwIm3DVertexSetRGBA(&v,variant==3?255:layer?220:45,layer?240:128,variant==3?(layer?190:64):255,uint8((layer?255:110)*fade*pulse*opacity[j]));
                        vertices.push_back(v);
                    }
                }
            }
        }
    }
    // All glows, then all white-hot cores. Keep each submission below librw's
    // fixed 10,000 vertex/index buffers and retain CPU capacity between frames.
    static std::vector<RwImVertexIndex> indices;
    for(auto &vertices:batches){
        if(vertices.empty())continue;
        const size_t batchLimit=9996;
        const size_t required=Min(vertices.size(),batchLimit);
        if(indices.size()<required){
            const size_t old=indices.size();indices.resize(required);
            for(size_t i=old;i<indices.size();i++)indices[i]=RwImVertexIndex(i);
        }
        for(size_t offset=0;offset<vertices.size();offset+=batchLimit){
            const size_t count=Min(batchLimit,vertices.size()-offset);
            if(RwIm3DTransform(vertices.data()+offset,count,nil,rwIM3D_VERTEXXYZ|rwIM3D_VERTEXRGBA)){
                RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST,indices.data(),count);RwIm3DEnd();
            }
        }
    }
    for(int i=0;i<8;i++)RwRenderStateSet(states[i],saved[i]);
}
