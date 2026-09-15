// Independent rear-facing cameras for the donor's three mirror materials.
// Captures are staggered to bound cost, and never render mirror surfaces into themselves.
#pragma once
#include "main.h"
#include "Timecycle.h"
namespace DonorMirrors {
struct Mirror { RpAtomic *surface=nil; RpMaterial *material=nil; RwTexture *original=nil,*texture=nil; RwCamera *camera=nil; RwFrame *mount=nil; CVector center=CVector(0,0,0),normal=CVector(0,1,0); };
static Mirror mirrors[7];
static RpClump *discoveredClump=nil;
static int nextMirror=0;
static std::vector<std::pair<RpAtomic*,uint32>> hiddenMirrors;
inline void Clear(){
    discoveredClump=nil;nextMirror=0;
    for(auto &m:mirrors){
        if(m.material && m.original)RpMaterialSetTexture(m.material,m.original);
        if(m.original)RwTextureDestroy(m.original);
        if(m.camera){
            if(m.camera->world)RpWorldRemoveCamera(m.camera->world,m.camera);
            RwRaster *z=RwCameraGetZRaster(m.camera);RwFrame *f=RwCameraGetFrame(m.camera);
            RwCameraSetRaster(m.camera,nil);RwCameraSetZRaster(m.camera,nil);RwCameraSetFrame(m.camera,nil);
            RwCameraDestroy(m.camera);if(z)RwRasterDestroy(z);if(f)RwFrameDestroy(f);
        }
        if(m.texture)RwTextureDestroy(m.texture);
        m=Mirror();
    }
}
inline RpAtomic *Find(RpAtomic *a,void *){
    auto g=RpAtomicGetGeometry(a);if(!g)return a;
    for(int i=0;i<g->matList.numMaterials;i++){
        auto material=g->matList.materials[i];auto texture=RpMaterialGetTexture(material);if(!texture)continue;
        const char *name=RwTextureGetName(texture);
        int index=!_stricmp(name,"mirror")?0:!_stricmp(name,"mirrorl")?1:!_stricmp(name,"mirrorr")?2:-1;
        if(index<0 || mirrors[index].material)continue;
        auto &m=mirrors[index];m.material=material;m.original=texture;texture->refCount++;
        m.mount=RpAtomicGetFrame(a);m.surface=a;
        // A door atomic includes the cabin and trim. Locate only triangles
        // assigned to the mirror material, not the whole door's bounding sphere.
        CVector low(1.e10f,1.e10f,1.e10f),high(-1.e10f,-1.e10f,-1.e10f);int count=0;
        for(int t=0;t<g->numTriangles;t++)if(g->triangles[t].matId==i)for(int v=0;v<3;v++){
            const auto &p=g->morphTargets[0].vertices[g->triangles[t].v[v]];
            low.x=Min(low.x,p.x);low.y=Min(low.y,p.y);low.z=Min(low.z,p.z);
            high.x=Max(high.x,p.x);high.y=Max(high.y,p.y);high.z=Max(high.z,p.z);++count;
        }
        m.center=count?(low+high)*.5f:CVector(g->morphTargets[0].calculateBoundingSphere().center);
        float largest=0;
        for(int t=0;t<g->numTriangles;t++)if(g->triangles[t].matId==i){
            auto &tri=g->triangles[t];auto vertices=g->morphTargets[0].vertices;
            CVector normal=CrossProduct(CVector(vertices[tri.v[1]])-CVector(vertices[tri.v[0]]),CVector(vertices[tri.v[2]])-CVector(vertices[tri.v[0]]));
            if(normal.MagnitudeSqr()>largest){largest=normal.MagnitudeSqr();normal.Normalise();m.normal=normal;}
        }
        m.camera=RwCameraCreate();RwCameraSetFrame(m.camera,RwFrameCreate());
        // beginUpdate takes its lighting world from the camera. An unattached
        // camera makes GL3 enumerate lights through a null world pointer.
        RpWorldAddCamera(Scene.world,m.camera);
        RwRaster *r=RwRasterCreate(256,256,32,rwRASTERTYPECAMERATEXTURE);
        RwCameraSetRaster(m.camera,r);RwCameraSetZRaster(m.camera,RwRasterCreate(256,256,0,rwRASTERTYPEZBUFFER));
        RwCameraSetNearClipPlane(m.camera,.1f);RwCameraSetFarClipPlane(m.camera,200.0f);
        RwV2d view={1.0f,1.0f};RwCameraSetViewWindow(m.camera,&view);
        m.texture=RwTextureCreate(r);RwTextureSetFilterMode(m.texture,rwFILTERLINEAR);
        RwTextureSetAddressing(m.texture,rwTEXTUREADDRESSCLAMP);
    }return a;
}
inline RpAtomic *FindHubcap(RpAtomic *a,void *){
    const char *name=GetFrameNodeName(RpAtomicGetFrame(a));
    if(!name || !strstr(name,"hubcapbttf3"))return a;
    int index=strstr(name,"lf")?3:strstr(name,"lb")?4:strstr(name,"rf")?5:strstr(name,"rb")?6:-1;
    if(index<0 || mirrors[index].camera || !a->geometry)return a;
    auto &m=mirrors[index];m.mount=RpAtomicGetFrame(a);m.surface=a;
    m.center=CVector(a->geometry->morphTargets[0].calculateBoundingSphere().center);
    m.camera=RwCameraCreate();RwCameraSetFrame(m.camera,RwFrameCreate());RpWorldAddCamera(Scene.world,m.camera);
    auto raster=RwRasterCreate(128,128,32,rwRASTERTYPECAMERATEXTURE);
    RwCameraSetRaster(m.camera,raster);RwCameraSetZRaster(m.camera,RwRasterCreate(128,128,0,rwRASTERTYPEZBUFFER));
    RwCameraSetNearClipPlane(m.camera,.05f);RwCameraSetFarClipPlane(m.camera,200.f);
    RwV2d view={1.f,1.f};RwCameraSetViewWindow(m.camera,&view);
    m.texture=RwTextureCreate(raster);RwTextureSetFilterMode(m.texture,rwFILTERLINEAR);RwTextureSetAddressing(m.texture,rwTEXTUREADDRESSCLAMP);
    return a;
}
template<class Pool> void DrawPool(Pool *pool,const CVector &position){
    if(!pool)return;
    for(int i=0;i<pool->GetSize();i++){
        auto entity=pool->GetSlot(i);
        if(!entity || !entity->m_rwObject || !entity->bIsVisible)continue;
        if((entity->GetPosition()-position).MagnitudeSqr()>200.0f*200.0f)continue;
        RwSphere bound;bound.center=entity->GetBoundCentre();bound.radius=entity->GetBoundRadius();
        if(RwCameraFrustumTestSphere(Scene.camera,&bound)==rwSPHEREOUTSIDE)continue;
        // Avoid vehicle callbacks that enqueue into the main view's alpha lists.
        if(RwObjectGetType(entity->m_rwObject)==rpATOMIC)AtomicDefaultRenderCallBack((RpAtomic*)entity->m_rwObject);
        else if(RwObjectGetType(entity->m_rwObject)==rpCLUMP)RpClumpForAllAtomics((RpClump*)entity->m_rwObject,[](RpAtomic *a,void*)->RpAtomic*{
            if(RpAtomicGetFlags(a)&rpATOMICRENDER)AtomicDefaultRenderCallBack(a);return a;
        },nil);
    }
}
// Experimental live reflection coating, limited to the named BTTF3 hubcaps.
// Material textures remain untouched; each wheel has an outward capture.
inline void DrawHubcaps(CAutomobile *car){
    if(!car || !car->m_rwObject || !car->bIsVisible)return;
    const RwRenderState states[]={rwRENDERSTATETEXTURERASTER,rwRENDERSTATEZTESTENABLE,rwRENDERSTATEZWRITEENABLE,rwRENDERSTATEVERTEXALPHAENABLE,rwRENDERSTATESRCBLEND,rwRENDERSTATEDESTBLEND,rwRENDERSTATECULLMODE};
    void *saved[7]={};for(int i=0;i<7;i++)RwRenderStateGet(states[i],&saved[i]);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,(void*)TRUE);RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,(void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,(void*)TRUE);RwRenderStateSet(rwRENDERSTATESRCBLEND,(void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,(void*)rwBLENDINVSRCALPHA);RwRenderStateSet(rwRENDERSTATECULLMODE,(void*)rwCULLMODECULLNONE);
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,[](RpAtomic *a,void *data)->RpAtomic*{
        CAutomobile *car=static_cast<CAutomobile*>(data);const char *name=GetFrameNodeName(RpAtomicGetFrame(a));
        if(!name || !strstr(name,"hubcapbttf3") || !(RpAtomicGetFlags(a)&rpATOMICRENDER))return a;
        auto g=a->geometry;if(!g || !g->numVertices || !g->morphTargets[0].normals)return a;
        const RwMatrix *matrix=RwFrameGetLTM(RpAtomicGetFrame(a));
        RwV3d center;auto localCenter=g->morphTargets[0].calculateBoundingSphere().center;
        RwV3dTransformPoints(&center,&localCenter,1,matrix);
        int index=strstr(name,"lf")?3:strstr(name,"lb")?4:strstr(name,"rf")?5:strstr(name,"rb")?6:-1;
        if(index<0)return a;
        auto &mirror=mirrors[index];if(!mirror.texture || !mirror.camera)return a;
        const RwMatrix *capture=RwFrameGetLTM(RwCameraGetFrame(mirror.camera));
        std::vector<RwIm3DVertex> vertices(g->numVertices);std::vector<RwImVertexIndex> indices(g->numTriangles*3);
        for(int i=0;i<g->numVertices;i++){
            RwV3d world;RwV3dTransformPoints(&world,&g->morphTargets[0].vertices[i],1,matrix);
            auto local=g->morphTargets[0].normals[i];
            CVector normal=CVector(matrix->right)*local.x+CVector(matrix->up)*local.y+CVector(matrix->at)*local.z;normal.Normalise();
            CVector point(world),view=point-TheCamera.GetPosition();view.Normalise();
            CVector reflected=view-normal*(2*DotProduct(view,normal));
            const float u=Max(.01f,Min(.99f,.5f+.48f*DotProduct(reflected,CVector(capture->right))));
            const float v=Max(.01f,Min(.99f,.5f-.48f*DotProduct(reflected,CVector(capture->up))));
            point+=normal*.002f;
            RwIm3DVertexSetPos(&vertices[i],point.x,point.y,point.z);RwIm3DVertexSetRGBA(&vertices[i],255,255,255,200);
            RwIm3DVertexSetU(&vertices[i],u);RwIm3DVertexSetV(&vertices[i],v);
        }
        for(int i=0;i<g->numTriangles;i++)for(int j=0;j<3;j++)indices[i*3+j]=g->triangles[i].v[j];
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER,RwTextureGetRaster(mirror.texture));
        if(RwIm3DTransform(vertices.data(),vertices.size(),nil,rwIM3D_VERTEXXYZ|rwIM3D_VERTEXRGBA|rwIM3D_VERTEXUV)){
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST,indices.data(),indices.size());RwIm3DEnd();
        }
        return a;
    },car);
    for(int i=0;i<7;i++)RwRenderStateSet(states[i],saved[i]);
}
inline void Capture(CAutomobile *car,int mirrorIndex){
    auto &m=mirrors[mirrorIndex];
    RwV3d pos;RwV3dTransformPoints(&pos,&m.center,1,RwFrameGetLTM(m.mount));
    RwMatrix *matrix=RwFrameGetMatrix(RwCameraGetFrame(m.camera));
    CVector position(pos),up=car->GetUp();
    const RwMatrix *mount=RwFrameGetLTM(m.mount);
    CVector normal=CVector(mount->right)*m.normal.x+CVector(mount->up)*m.normal.y+CVector(mount->at)*m.normal.z;normal.Normalise();
    CVector incident=position-TheCamera.GetPosition();incident.Normalise();
    // Reflect the viewing ray about the real mirror face, including door
    // animation. A fixed rear-facing camera is not a mirror when looking sideways.
    CVector forward=incident-normal*(2.0f*DotProduct(incident,normal));forward.Normalise();
    if(mirrorIndex==1 || mirrorIndex==2){
        const float lateral=DotProduct(position-car->GetPosition(),car->GetRight());
        const float side=lateral<0?-1.f:1.f;
        const CVector outward=car->GetRight()*side;
        const auto &bounds=car->GetColModel()->boundingBox;
        const float bodyEdge=side<0?-bounds.min.x:bounds.max.x;
        // The mirror surface can be recessed into the door geometry. Keep the
        // capture outside the body, including when the door is animated.
        position+=outward*Max(.18f,bodyEdge+.25f-side*lateral);
        // Never aim a side mirror through the cabin. Retain the reflected
        // vertical angle, with a rearward and slightly outward viewing ray.
        const float vertical=Max(-.35f,Min(.35f,DotProduct(forward,car->GetUp())));
        const float outwardAngle=Max(.22f,Min(.65f,DotProduct(forward,outward)));
        forward=-car->GetForward()+outward*outwardAngle+car->GetUp()*vertical;
        forward.Normalise();
        RwV2d view={.65f,.65f};RwCameraSetViewWindow(m.camera,&view);
    }
    if(mirrorIndex>=3){
        forward=car->GetRight()*(DotProduct(position-car->GetPosition(),car->GetRight())<0?-1.f:1.f);
        position+=forward*.12f;
    }
    position+=forward*.08f;
    CVector right=CrossProduct(up,forward);right.Normalise();up=CrossProduct(forward,right);up.Normalise();
    matrix->right=-right;matrix->up=up;matrix->at=forward;matrix->pos=position;RwMatrixUpdate(matrix);RwFrameUpdateObjects(RwCameraGetFrame(m.camera));
    hiddenMirrors.clear();
    RpClumpForAllAtomics((RpClump*)car->m_rwObject,[](RpAtomic *a,void*)->RpAtomic*{
        auto g=RpAtomicGetGeometry(a);
        for(int i=0;i<g->matList.numMaterials;i++)for(auto &m:mirrors)if(g->matList.materials[i]==m.material){
            hiddenMirrors.push_back({a,RpAtomicGetFlags(a)});RpAtomicSetFlags(a,0);return a;
        }return a;
    },nil);
    const RwRenderState states[]={rwRENDERSTATEZTESTENABLE,rwRENDERSTATEZWRITEENABLE,rwRENDERSTATECULLMODE,rwRENDERSTATEVERTEXALPHAENABLE,rwRENDERSTATESRCBLEND,rwRENDERSTATEDESTBLEND};
    void *savedStates[6]={};for(int i=0;i<6;i++)RwRenderStateGet(states[i],&savedStates[i]);
    auto savedWorld=rw::engine->currentWorld;
    RwCamera *saved=Scene.camera;Scene.camera=m.camera;
    CVisibilityPlugins::SetRenderWareCamera(m.camera);
    RwRGBA sky={uint8(CTimeCycle::GetSkyBottomRed()),uint8(CTimeCycle::GetSkyBottomGreen()),uint8(CTimeCycle::GetSkyBottomBlue()),255};
    RwCameraClear(m.camera,&sky,rwCAMERACLEARIMAGE|rwCAMERACLEARZ);
    if(RwCameraBeginUpdate(m.camera)){
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE,(void*)TRUE);RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,(void*)TRUE);
        RwRenderStateSet(rwRENDERSTATECULLMODE,(void*)rwCULLMODECULLFRONT);
        DrawPool(CPools::GetBuildingPool(),CVector(pos));DrawPool(CPools::GetTreadablePool(),CVector(pos));
        DrawPool(CPools::GetObjectPool(),CVector(pos));DrawPool(CPools::GetVehiclePool(),CVector(pos));DrawPool(CPools::GetPedPool(),CVector(pos));
        RwCameraEndUpdate(m.camera);
        if(m.material)RpMaterialSetTexture(m.material,m.texture);
    }
    Scene.camera=saved;rw::engine->currentWorld=savedWorld;
    CVisibilityPlugins::SetRenderWareCamera(saved);
    for(int i=0;i<6;i++)RwRenderStateSet(states[i],savedStates[i]);
    for(auto &item:hiddenMirrors)RpAtomicSetFlags(item.first,item.second);
}
inline bool Visible(int index){
    auto &candidate=mirrors[index];
    if(!candidate.camera || !candidate.surface || !(RpAtomicGetFlags(candidate.surface)&rpATOMICRENDER))return false;
    RwSphere surface;RwV3dTransformPoints(&surface.center,&candidate.center,1,RwFrameGetLTM(candidate.mount));surface.radius=.3f;
    return RwCameraFrustumTestSphere(Scene.camera,&surface)!=rwSPHEREOUTSIDE;
}
inline void Update(CAutomobile *car,bool hubcapsVisible){
    if(!car || !car->m_rwObject || !Scene.world || !Scene.camera)return;
    auto clump=(RpClump*)car->m_rwObject;
    if(discoveredClump!=clump){
        RpClumpForAllAtomics(clump,Find,nil);
        RpClumpForAllAtomics(clump,FindHubcap,nil);
        discoveredClump=clump;
    }
    // Door mirrors retain their visible-surface rotation. Wheels no longer
    // wait behind them: both near-side hubcaps capture every rendered frame.
    for(int attempt=0;attempt<3;attempt++){
        const int index=nextMirror;nextMirror=(nextMirror+1)%3;
        if(Visible(index)){Capture(car,index);break;}
    }
    if(hubcapsVisible && !CCamera::bLeafFirstPerson){
        for(int index=3;index<7;index++){
            if(!Visible(index))continue;
            RwV3d center;RwV3dTransformPoints(&center,&mirrors[index].center,1,RwFrameGetLTM(mirrors[index].mount));
            const float side=DotProduct(CVector(center)-car->GetPosition(),car->GetRight())<0?-1.f:1.f;
            if(DotProduct(TheCamera.GetPosition()-CVector(center),car->GetRight()*side)<=0)continue;
            Capture(car,index);
        }
    }
}
}
