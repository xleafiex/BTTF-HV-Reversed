#include "../delorean/ArrivalSequence.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using DonorSystems::ArrivalSequence;
int main(){
 ArrivalSequence s;assert(s.Take(99999)==-1);
 s.Begin(19851026,19851027);assert(s.Take(0)==0);assert(s.Take(199)==-1);assert(s.Take(200)==1);assert(s.Take(499)==-1);assert(s.Take(500)==2);assert(s.Take(600)==-1);
 assert(!strcmp(s.Clip(0),"delorean/reentry_short.wav") && s.Clip(1)==nullptr && s.Clip(2)==nullptr);
 s.Begin(19851026,20151021);assert(s.offsets[1]>690 && s.offsets[1]<710);assert(s.offsets[2]>1400 && s.offsets[2]<1450);assert(!s.longJump && !s.shortJump);assert(s.Clip(2)==nullptr);
 ArrivalSequence reverse;reverse.Begin(20151021,19851026);assert(reverse.offsets[1]==s.offsets[1]);
 s.Begin(19851026,18851026);assert(s.longJump && s.Clip(2)!=nullptr);
 // Slow frames drain each due event exactly once.
 assert(s.Take(10000)==0 && s.Take(10000)==1 && s.Take(10000)==2 && s.Take(10000)==-1);
 puts("Donor arrival timing, sound selection, reverse travel and missed-frame delivery passed");
}
