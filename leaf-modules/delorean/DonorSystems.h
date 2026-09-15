#pragma once
#include <algorithm>
#include <cmath>

// Script motion is stepped at the donor's 30 fps baseline, independently of
// reVC's 50-units/second timestep. Times below are seconds of game time.
namespace DonorSystems {
inline float Approach(float v, float target, float down, float up) {
    return v > target ? std::max(target, v-down) : std::min(target, v+up);
}
struct Dashboard {
    float temperature=75, oil=0, temp=0, volts=-20, petrol=0, ignition=0;
    double pending=0, running=0;
    bool engine=false, oilLight=false, batteryLight=false, seatbelt=false;
    bool fuelLight=false, lowBeams=false;
    void Update(double seconds, bool on, bool occupied, int gear, float gas, float brake, bool lights) {
        if(on && !engine) running=0;
        engine=on;
        pending += seconds;
        while(pending+1e-9 >= 1.0/30.0) {
            pending -= 1.0/30.0;
            if(on) {
                running += 1.0/30.0;
                temperature=std::min(160.0f,temperature+.05f);
                volts=Approach(volts,running<.5?5:40,1,1);
                ignition=Approach(ignition,running<.5?-60:-30,15,15);
                temp=Approach(temp,std::max(-10.0f,.35f*temperature-45),1,1);
                const float throttle=std::max(0.0f,std::min(1.0f,gear==0?brake:gas));
                const float heat=temperature>140?2*temperature-340:(temperature>100 || running>1?-60:0);
                oil=Approach(oil,std::min(-20-40*throttle,heat),1+2*throttle,1);
                petrol=Approach(petrol,-30,1,1);
                oilLight=oil>-10; batteryLight=volts<20;
                seatbelt=running<5; fuelLight=petrol>-5; lowBeams=lights;
            } else {
                temperature=std::max(75.0f,temperature-.01f);
                volts=Approach(volts,occupied?15:-20,1,1);
                ignition=Approach(ignition,occupied?0:60,15,15);
                oilLight=batteryLight=seatbelt=lowBeams=false;
            }
        }
    }
};

// One evaluation returns the stage even if the renderer skipped several frames.
struct Stage { int index; float progress; };
inline Stage Locate(double elapsed, const double *durations, int count) {
    int i=0;
    while(i<count && elapsed+1e-8>=durations[i]) elapsed-=durations[i++];
    return {i,i<count ? float(std::max(0.0,std::min(1.0,elapsed/durations[i]))) : 1.0f};
}
inline const double *FusionDurations() {
    // wait10 = one frame; wait50 = two frames at the donor's 30 fps baseline.
    static const double d[]={10/30.,16/30.,22/30.,82/30.,7/30.,10/30.,.5,.5}; return d;
}
inline Stage Fusion(double seconds) {return Locate(seconds,FusionDurations(),8);}
inline const double *ReactorDurations() {
    static const double d[]={19/30.,6/30.,.5,.2,.2,22/30.,91/30.,3/30.,.1,21/30.,.1,.1,.1,6/30.,.4,7/30.,.5,.5}; return d;
}
inline Stage Reactor(double seconds) {return Locate(seconds,ReactorDurations(),18);}
}
