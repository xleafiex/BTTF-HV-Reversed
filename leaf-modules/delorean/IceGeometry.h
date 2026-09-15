#pragma once
// Build a private overlay with surface normals derived from the panel mesh.
// The donor effect's authored normals are radial and cannot be used for
// triplanar mapping: they stretched the texture across the bonnet and doors.
inline void PrepareIceGeometry(rw::Atomic *atomic){
    using namespace rw;
    Geometry *src=atomic->geometry;
    if(!src || !src->numVertices || !src->numTriangles || !src->texCoords[0])return;
    Geometry *g=Geometry::create(src->numVertices,src->numTriangles,Geometry::POSITIONS|Geometry::NORMALS|Geometry::TEXTURED|Geometry::LIGHT|Geometry::MODULATE);
    if(!g)return;
    auto &m=g->morphTargets[0];
    memcpy(m.vertices,src->morphTargets[0].vertices,src->numVertices*sizeof(V3d));
    memcpy(g->texCoords[0],src->texCoords[0],src->numVertices*sizeof(TexCoords));
    memcpy(g->triangles,src->triangles,src->numTriangles*sizeof(Triangle));
    for(int i=0;i<src->matList.numMaterials;i++)g->matList.appendMaterial(src->matList.materials[i]);
    for(int i=0;i<g->numVertices;i++)m.normals[i]={0,0,0};
    for(int i=0;i<g->numTriangles;i++){
        auto &t=g->triangles[i];
        V3d n=cross(sub(m.vertices[t.v[1]],m.vertices[t.v[0]]),sub(m.vertices[t.v[2]],m.vertices[t.v[0]]));
        for(int j=0;j<3;j++)m.normals[t.v[j]]=add(m.normals[t.v[j]],n);
    }
    for(int i=0;i<g->numVertices;i++){
        float l=length(m.normals[i]);
        m.normals[i]=l>1.e-8f?scale(m.normals[i],1.0f/l):V3d{0,0,1};
        // Separate the coating from the coplanar stainless surface to avoid depth stippling.
        m.vertices[i]=add(m.vertices[i],scale(m.normals[i],.0015f));
    }
    g->calculateBoundingSphere();g->unlock();atomic->setGeometry(g,0);g->destroy();
}
