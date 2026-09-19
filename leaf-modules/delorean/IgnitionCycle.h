#pragma once

namespace DonorSystems {
// The donor waits 100 ms twice between failed turnovers. Keep that cadence
// independent of rendering, and stop the current attempt on throttle release.
struct IgnitionCycle {
    double elapsed=0;
    unsigned attempts=0;
    bool active=false;
    void Cancel() { elapsed=0; active=false; }
    bool Update(double seconds,bool requested,bool lowPower,unsigned roll) {
        if(!requested) { Cancel(); return false; }
        if(!active) { active=true; elapsed=0; }
        elapsed+=seconds;
        if(elapsed+1e-9<.2) return false;
        elapsed-=.2;
        ++attempts;
        // Preserve the port's bounded low-power recovery; the donor's
        // contradictory success branch is not copied literally.
        if(!lowPower || roll%4==0 || attempts>=6) {
            *this=IgnitionCycle(); return true;
        }
        return false;
    }
};
}
