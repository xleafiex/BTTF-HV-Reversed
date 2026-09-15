#include "../delorean/DonorLegPose.h"
#include <cassert>
#include <cstdio>
using namespace DonorLegPose;
int main(){
    const V hip(0,0,1),knee(0,.3f,.7f),ankle(0,.6f,.4f);
    const float upper=Length(knee-hip),lower=Length(ankle-knee);
    for(V target:{V(.1f,.65f,.5f),V(-.2f,.5f,.3f),V(0,5,-5),V(0,0,.99f)}){
        V nextKnee,nextAnkle;
        assert(Solve(hip,knee,ankle,target,V(0,0,1),nextKnee,nextAnkle));
        assert(std::fabs(Length(nextKnee-hip)-upper)<.0001f);
        assert(std::fabs(Length(nextAnkle-nextKnee)-lower)<.0001f);
        V transformed=Align(knee-hip,knee-hip,nextKnee-hip);
        assert(Length(transformed-(nextKnee-hip))<.0001f);
        assert(std::isfinite(nextAnkle.z));
        if(Length(target-hip)<upper+lower-.001f)assert(Length(nextAnkle-target)<.0001f);
    }
    V a,b;
    assert(!Solve(hip,hip,ankle,ankle,V(0,0,1),a,b));
    assert(Length(Align(V(1,0,0),V(1,0,0),V(-1,0,0))-V(-1,0,0))<.0001f);
    puts("PASS: seated leg solver preserves segment lengths, reaches/clamps targets, handles degenerate and opposite directions");
}
