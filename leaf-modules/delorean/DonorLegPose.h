#pragma once
#include <cmath>
#include <algorithm>
namespace DonorLegPose {
struct V {
    float x,y,z;
    V(float x=0,float y=0,float z=0):x(x),y(y),z(z){}
    V operator+(V b)const{return V(x+b.x,y+b.y,z+b.z);}
    V operator-(V b)const{return V(x-b.x,y-b.y,z-b.z);}
    V operator*(float s)const{return V(x*s,y*s,z*s);}
};
inline float Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V Cross(V a,V b){return V(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);}
inline float Length(V a){return std::sqrt(Dot(a,a));}
inline V Unit(V a){float n=Length(a);return n>1e-6f?a*(1/n):V();}
inline V Align(V v,V from,V to){
    V a=Unit(from),b=Unit(to);
    if(Length(a)<.5f || Length(b)<.5f)return v;
    float c=std::max(-1.0f,std::min(1.0f,Dot(a,b)));
    if(c<-.9999f){V axis=Unit(Cross(a,std::fabs(a.z)<.9f?V(0,0,1):V(1,0,0)));return axis*(2*Dot(axis,v))-v;}
    V k=Cross(a,b);
    return v+Cross(k,v)+Cross(k,Cross(k,v))*(1/(1+c));
}
inline bool Solve(V hip,V knee,V ankle,V target,V bendHint,V &newKnee,V &newAnkle){
    const float upper=Length(knee-hip),lower=Length(ankle-knee),distance=Length(target-hip);
    if(upper<.01f || lower<.01f || distance<.001f)return false;
    const V axis=Unit(target-hip);
    const float reach=std::max(std::fabs(upper-lower)+.001f,std::min(upper+lower-.001f,distance));
    const float along=(upper*upper-lower*lower+reach*reach)/(2*reach);
    const float height=std::sqrt(std::max(0.0f,upper*upper-along*along));
    V bend=(knee-hip)-axis*Dot(knee-hip,axis);
    if(Length(bend)<.001f)bend=bendHint-axis*Dot(bendHint,axis);
    if(Length(bend)<.001f)bend=Cross(axis,std::fabs(axis.z)<.9f?V(0,0,1):V(1,0,0));
    newKnee=hip+axis*along+Unit(bend)*height;
    newAnkle=hip+axis*reach;
    return true;
}
}
