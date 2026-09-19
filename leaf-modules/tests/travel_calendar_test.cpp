#include "../delorean/TravelCalendar.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace DonorSystems;
    assert(NextDate(19851031)==19851101);
    assert(NextDate(19851231)==19860101);
    assert(NextDate(19850430)==19850501);
    assert(NextDate(19000228)==19000301);
    assert(NextDate(20000228)==20000229);
    assert(NextDate(20000229)==20000301);
    assert(NextDate(20240228)==20240229);
    assert(NextDate(99991231)==99991231);
    TravelCalendar calendar;
    assert(calendar.Update(19851026,23)==19851026);
    int date=calendar.Update(19851026,0);
    assert(date==19851027);
    assert(calendar.Update(date,0)==date);
    calendar.Rebase(1); // travel backwards in time: no extra midnight
    assert(calendar.Update(19551105,1)==19551105);
    calendar.Rebase(23);
    assert(calendar.Update(19551105,0)==19551106);
    puts("PASS: midnight, month/year rollover, century leap years, destination rebase and year limit");
}
