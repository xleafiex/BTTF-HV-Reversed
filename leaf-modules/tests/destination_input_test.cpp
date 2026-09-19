#include "../delorean/DestinationInput.h"
#include <cassert>
#include <cstdio>
int main(){
    int date=19851026,time=121;
    assert(DonorSystems::ParseDestination("0930",date,time));
    assert(date==19851026 && time==930);
    assert(DonorSystems::ParseDestination("11051955",date,time));
    assert(date==19551105 && time==930);
    assert(DonorSystems::ParseDestination("102120151629",date,time));
    assert(date==20151021 && time==1629);
    for(const char *bad:{"","123","12345","1234567890123","12a4"}){
        assert(!DonorSystems::ParseDestination(bad,date,time));
        assert(date==20151021 && time==1629);
    }
    puts("PASS: time-only, date-only, full destination and malformed entry preservation");
}
