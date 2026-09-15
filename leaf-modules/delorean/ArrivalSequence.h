#pragma once
#include <cmath>
namespace DonorSystems {
// HV Include/Explosion.txt: re-entry spacing is based on the date delta.
struct ArrivalSequence {
    unsigned offsets[3]={0,200,500};
    int next=3;
    bool shortJump=true,longJump=false;
    void Begin(int fromDate,int toDate){
        const double years=std::fabs(double(toDate)-fromDate)/10000.0;
        shortJump=years<1.0;longJump=years>50.0;next=0;
        offsets[0]=0;
        if(shortJump){offsets[1]=200;offsets[2]=500;}
        else {const double delay=400.0+88.0*std::log(years);offsets[1]=unsigned(delay);offsets[2]=offsets[1]+unsigned(delay*1.05);}
    }
    int Take(unsigned elapsed){return next<3 && elapsed>=offsets[next]?next++:-1;}
    const char *Clip(int step)const{
        if(shortJump)return step==0?"delorean/reentry_short.wav":nullptr;
        if(longJump)return "delorean/reentry_long.wav";
        return step<2?"delorean/reentry_single.wav":nullptr;
    }
};
}
