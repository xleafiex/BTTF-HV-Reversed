#pragma once
rw::gl3::ObjPipeline *cabinDetailPipeline=nil;
std::map<rw::Atomic*,rw::ObjPipeline*> cabinDetailOriginals;
void CabinDetailDraw(rw::Atomic *atomic,rw::gl3::InstanceDataHeader *header){
    const char *name=GetFrameNodeName(atomic->getFrame());
    const float layer=name && strstr(name,"needle")?3.0f:
        (name && (strstr(name,"digit") || strstr(name,"colon") || strstr(name,"led"))?2.0f:1.0f);
    // Thin instrument overlays need a stable separation from their housings.
    // Keep depth testing so other cabin geometry still occludes them normally.
    const GLboolean offsetEnabled=glIsEnabled(GL_POLYGON_OFFSET_FILL);
    GLfloat oldFactor,oldUnits;glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&oldFactor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&oldUnits);
    glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(-.25f,-layer);
    rw::gl3::defaultRenderCB(atomic,header);
    glPolygonOffset(oldFactor,oldUnits);if(!offsetEnabled)glDisable(GL_POLYGON_OFFSET_FILL);
}
RpAtomic *AttachCabinDetail(RpAtomic *atomic,void*){
    const char *name=GetFrameNodeName(RpAtomicGetFrame(atomic));
    if(!name || FrostIsGlass(name) || strstr(name,"_fr") || strstr(name,"wormhole"))return atomic;
    const bool detail=strstr(name,"needle") || strstr(name,"digit") || strstr(name,"colon") ||
        strstr(name,"sidled") || strstr(name,"tcd") || strstr(name,"gaugeslights") ||
        !strncmp(name,"dt",2) || !strncmp(name,"pt",2) || !strncmp(name,"ltd",3);
    if(!detail)return atomic;
    if(!cabinDetailPipeline){
        cabinDetailPipeline=rw::gl3::ObjPipeline::create();
        cabinDetailPipeline->instanceCB=rw::gl3::defaultInstanceCB;
        cabinDetailPipeline->uninstanceCB=rw::gl3::defaultUninstanceCB;
        cabinDetailPipeline->renderCB=CabinDetailDraw;
    }
    cabinDetailOriginals[atomic]=atomic->pipeline;atomic->pipeline=cabinDetailPipeline;return atomic;
}
void ShutdownCabinDetail(){
    if(Car())RpClumpForAllAtomics((RpClump*)Car()->m_rwObject,[](RpAtomic*a,void*)->RpAtomic*{
        auto old=cabinDetailOriginals.find(a);if(old!=cabinDetailOriginals.end())a->pipeline=old->second;return a;
    },nil);
    cabinDetailOriginals.clear();if(cabinDetailPipeline){cabinDetailPipeline->destroy();cabinDetailPipeline=nil;}
}
