#pragma once
// A separate raised shell is appended to the ice atomic so visibility, door
// transforms and the donor thaw timer remain shared with the underlying ice.
inline float SnowNoise(rw::V3d p){
    int x=int(floorf(p.x)),y=int(floorf(p.y)),z=int(floorf(p.z));
    float f[3]={p.x-x,p.y-y,p.z-z};for(float &v:f)v=v*v*(3-2*v);
    float total=0;
    for(int i=0;i<2;i++)for(int j=0;j<2;j++)for(int k=0;k<2;k++){
        rw::uint32 h=rw::uint32(x+i)*374761393u+rw::uint32(y+j)*668265263u+rw::uint32(z+k)*2246822519u;
        h=(h^(h>>13))*1274126177u;h^=h>>16;
        total+=(h&65535)/65535.f*(i?f[0]:1-f[0])*(j?f[1]:1-f[1])*(k?f[2]:1-f[2]);
    }
    return total;
}
inline float SnowDeposit(rw::V3d p){
    float f=SnowNoise(rw::scale(p,3.2f))*.88f+SnowNoise(rw::scale(p,13.f))*.12f;
    f=std::max(0.f,std::min(1.f,(f-.54f)/.23f));return f*f*(3-2*f);
}
inline void AddSnowLayer(rw::Atomic *atomic,bool doorPanel=false){
    using namespace rw;
    Geometry *src=atomic->geometry;auto &m=src->morphTargets[0];
    Matrix carInverse;Matrix::invert(&carInverse,atomic->clump->getFrame()->getLTM());
    struct Point {V3d p,n;float amount;};
    std::vector<Point> points;
    std::vector<int> materials;
    // Only retain triangles touching a deposit. Bound both spacing and total
    // vertex count; this is prepared once when the car is spawned.
    float spacing=.035f;
    for(int attempt=0;attempt<6;attempt++){
        points.clear();materials.clear();
        bool full=false;
        for(int face=0;face<src->numTriangles && !full;face++){
            const Triangle &tri=src->triangles[face];
            float longest=0;
            for(int j=0;j<3;j++)longest=std::max(longest,length(sub(m.vertices[tri.v[j]],m.vertices[tri.v[(j+1)%3]])));
            int d=std::max(1,std::min(48,int(ceilf(longest/spacing))));
            auto point=[&](int x,int y){
                float b=float(x)/d,c=float(y)/d,a=1-b-c;
                Point v;v.p=add(add(scale(m.vertices[tri.v[0]],a),scale(m.vertices[tri.v[1]],b)),scale(m.vertices[tri.v[2]],c));
                v.n=normalize(add(add(scale(m.normals[tri.v[0]],a),scale(m.normals[tri.v[1]],b)),scale(m.normals[tri.v[2]],c)));
                v.amount=SnowDeposit(v.p);
                if(doorPanel){
                    V3d world,local;V3d::transformPoints(&world,&v.p,1,atomic->getFrame()->getLTM());
                    V3d::transformPoints(&local,&world,1,&carInverse);
                    // The donor door overlay also spans its upper window surround.
                    v.amount*=std::max(0.f,std::min(1.f,(.23f-local.z)/.04f));
                }
                return v;
            };
            auto emit=[&](Point a,Point b,Point c){
                if(std::max(a.amount,std::max(b.amount,c.amount))<.025f)return;
                if(src->numVertices+points.size()+3>60000){full=true;return;}
                points.push_back(a);points.push_back(b);points.push_back(c);materials.push_back(tri.matId);
            };
            for(int x=0;x<d && !full;x++)for(int y=0;y<d-x && !full;y++){
                emit(point(x,y),point(x+1,y),point(x,y+1));
                if(x+y<d-1)emit(point(x+1,y),point(x+1,y+1),point(x,y+1));
            }
        }
        if(!full)break;
        spacing*=1.5f;
        if(attempt==5)return; // Preserve the ice if this export exceeds the budget.
    }
    if(points.empty())return;
    Geometry *g=Geometry::create(src->numVertices+points.size(),src->numTriangles+materials.size(),src->flags);
    if(!g)return;
    auto &out=g->morphTargets[0];
    memcpy(out.vertices,m.vertices,src->numVertices*sizeof(V3d));memcpy(out.normals,m.normals,src->numVertices*sizeof(V3d));
    memcpy(g->texCoords[0],src->texCoords[0],src->numVertices*sizeof(TexCoords));memcpy(g->triangles,src->triangles,src->numTriangles*sizeof(Triangle));
    for(int i=0;i<src->matList.numMaterials;i++)g->matList.appendMaterial(src->matList.materials[i]);
    for(unsigned i=0;i<points.size();i++){
        Point v=points[i];const float e=.002f;
        V3d grad={(SnowDeposit(add(v.p,V3d{e,0,0}))-SnowDeposit(sub(v.p,V3d{e,0,0})))/(2*e),
                  (SnowDeposit(add(v.p,V3d{0,e,0}))-SnowDeposit(sub(v.p,V3d{0,e,0})))/(2*e),
                  (SnowDeposit(add(v.p,V3d{0,0,e}))-SnowDeposit(sub(v.p,V3d{0,0,e})))/(2*e)};
        grad=sub(grad,scale(v.n,dot(grad,v.n)));
        unsigned index=src->numVertices+i;
        out.vertices[index]=add(v.p,scale(v.n,.0008f+.040f*v.amount));
        out.normals[index]=normalize(sub(v.n,scale(grad,.040f)));
        g->texCoords[0][index]={-1024.f,v.amount};
    }
    for(unsigned i=0;i<materials.size();i++){
        Triangle &t=g->triangles[src->numTriangles+i];
        for(int j=0;j<3;j++)t.v[j]=src->numVertices+i*3+j;t.matId=materials[i];
    }
    g->calculateBoundingSphere();g->unlock();atomic->setGeometry(g,0);g->destroy();
}
