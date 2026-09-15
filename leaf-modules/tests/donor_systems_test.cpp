#include "../delorean/DonorSystems.h"
#include <cassert>
#include <cstdio>
using DonorSystems::Dashboard;
static Dashboard Run(int fps) {
    Dashboard d;
    for(int i=0;i<fps*30;++i)d.Update(1.0/fps,true,true,1,.6f,0,true);
    return d;
}
static void FusionEvents(int fps) {
    int previous=-1,trash=0,close=0,unlock=0,complete=0;
    for(int frame=0;frame<=fps*10;++frame){
        auto pose=DonorSystems::Fusion(double(frame)/fps);
        for(int event=previous+1;event<=pose.index;++event){
            trash+=event==3;close+=event==4;unlock+=event==7;complete+=event==8;
        }
        previous=pose.index;
        assert(pose.progress>=0 && pose.progress<=1);
    }
    assert(trash==1 && close==1 && unlock==1 && complete==1);
}
int main(){
    const Dashboard baseline=Run(30);
    for(int fps:{15,30,60,120,144}){
        auto d=Run(fps);
        assert(std::fabs(d.temperature-baseline.temperature)<.001f);
        assert(std::fabs(d.oil-baseline.oil)<.001f);
        assert(d.volts==40 && d.petrol==-30 && d.ignition==-30);
        assert(!d.seatbelt && !d.batteryLight && !d.fuelLight && d.lowBeams);
        FusionEvents(fps);
    }
    Dashboard d;d.Update(.25,true,true,1,0,0,false);
    assert(d.volts<20 && d.seatbelt && d.ignition==-60);
    for(int i=0;i<300;++i)d.Update(1./30,false,true,1,0,0,false);
    assert(d.volts==15 && d.ignition==0 && !d.oilLight && !d.batteryLight);
    d.Update(3,false,false,1,0,0,false);assert(d.volts==-20 && d.ignition==60);
    Dashboard forward,reverse;
    forward.Update(.3,true,true,1,1,0,false);reverse.Update(.3,true,true,0,0,1,false);
    assert(forward.oil==reverse.oil);
    double closed=0;for(int i=0;i<6;++i)closed+=DonorSystems::FusionDurations()[i];
    assert(DonorSystems::Fusion(closed+.49).index==6);
    assert(DonorSystems::Fusion(closed+.51).index==7);
    assert(DonorSystems::Fusion(closed+1.01).index==8);
    double reactorClosed=0;for(int i=0;i<16;++i)reactorClosed+=DonorSystems::ReactorDurations()[i];
    assert(DonorSystems::Reactor(reactorClosed+.49).index==16);
    assert(DonorSystems::Reactor(reactorClosed+.51).index==17);
    assert(DonorSystems::Reactor(reactorClosed+1.01).index==18);
    puts("PASS: gauges, ignition states, reverse throttle, lamps, 15-144 fps timing, one-shot cues, both refuel completion delays");
}
