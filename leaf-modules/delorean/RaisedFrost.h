#pragma once
#include "IceGeometry.h"
#include "SnowLayer.h"
// Frost overlays already match the donor panels. Keep their smooth topology;
// large low-resolution displacements created the visible faceted camouflage.
RpAtomic *BuildRaisedFrost(RpAtomic *atomic,void *){
    const char *name=GetFrameNodeName(RpAtomicGetFrame(atomic));
    bool frost=false;for(const auto &n:FROSTED_COMPONENTS)if(name && n==name)frost=true;
    if(!frost)return atomic;
    PrepareIceGeometry(atomic);
    if(!strstr(name,"window") && !strstr(name,"windscreen"))AddSnowLayer(atomic,strncmp(name,"door_",5)==0);
    return atomic;
}
