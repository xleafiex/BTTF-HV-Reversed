#include "../delorean/IgnitionCycle.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
int main(){
    for(int fps : {15,30,60,144}){
        DonorSystems::IgnitionCycle cycle;
        int frame=0;
        while(!cycle.Update(1.0/fps,true,true,1)) {++frame;assert(frame<fps*2);}
        assert(std::abs((frame+1.0)/fps-1.2)<=1.0/fps+1e-8);
        assert(!cycle.active && cycle.attempts==0);
        assert(!cycle.Update(.1,true,true,1));
        assert(!cycle.Update(.1,false,true,1));
        assert(!cycle.active);
        assert(!cycle.Update(.1,true,false,1));
        assert(cycle.Update(.1,true,false,1));
        assert(cycle.Update(.2,true,true,0));
    }
    std::puts("PASS: ignition cadence at 15/30/60/144 fps, bounded recovery, throttle release, normal and lucky starts");
}
