#include "../delorean/DonorCabin.h"
#include <cassert>
#include <cstdio>
using namespace DonorSystems;
int main(){
    assert(DisplayDigit(20151021,1)==1 && DisplayDigit(20151021,2)==2 && DisplayDigit(20151021,3)==0 && DisplayDigit(20151021,8)==2);
    assert(DigitalSpeed(0)==0 && DigitalSpeed(5)==9 && DigitalSpeed(10)==18 && DigitalSpeed(48.1f)==88 && DigitalSpeed(100)==88);
    assert(DigitalSpeed(47.9f)==87 && DigitalSpeed(48.0f)==88);
    DoorSounds doors;
    assert(!doors.Opened(0,true));assert(doors.Opened(0,false));assert(!doors.Opened(0,false));
    assert(doors.Opened(1,false));assert(!doors.Opened(0,true));assert(doors.Opened(0,false));
    for(int fps:{15,30,60,144}){
        const double dt=1.0/fps;
        CompassState compass;
        for(int i=0;i<fps*2;i++)compass.Update(dt,90,30,-20);
        assert(std::fabs(compass.yaw-90)<.01f && std::fabs(compass.pitch-30)<2.0f && std::fabs(compass.roll+20)<2.0f);
        compass.Update(dt,-170,30,-20); assert(compass.yaw>89 && compass.yaw<=96);
        ConsoleClock clock;clock.Reset(23,59);
        for(int i=0;i<fps*60;i++)clock.Update(dt);
        assert(clock.hour==0 && clock.minute==0 && clock.DisplayHour()==12);
        assert(clock.Colon());
        clock.Reset(13,5);assert(clock.DisplayHour()==1 && clock.MinuteHand()==30 && clock.HourHand()==392.5f);
        clock.Update(.5);assert(!clock.Colon());
        clock.Update(.5);assert(clock.Colon());
        clock.Update(125);assert(clock.hour==13 && clock.minute==7);
        // A destination clock jump must not alter the independently running
        // onboard clock; LAST remains the captured value while this advances.
        const int departed=2359;clock.Reset(departed/100,departed%100);
        for(int i=0;i<fps*120;i++)clock.Update(dt);
        assert(departed==2359 && clock.hour==0 && clock.minute==1);
        EmergencyLight emergency;
        emergency.Update(dt,true);
        for(int i=0;i<fps;i++)emergency.Update(dt,false);
        assert(emergency.on && emergency.handle==30);
        emergency.Update(dt,true);
        for(int i=0;i<fps;i++)emergency.Update(dt,false);
        assert(!emergency.on && emergency.handle==0);
        UnderbodyLights lights;
        for(int i=0;i<fps;i++)lights.Update(dt,true,true);
        assert(lights.alpha==255 && lights.chaser==5);
        for(int i=0;i<fps;i++)lights.Update(dt,true,false);
        assert(lights.alpha==0 && lights.chaser==0);
        for(int i=0;i<fps;i++)lights.Update(dt,false,true);
        assert(lights.alpha==0 && lights.chaser==0);
        UnderbodyLights phases;
        for(int n=1;n<=5;n++){phases.Update(.25,true,true);assert(phases.chaser==n%5+1);}
        Shifter shifter;
        for(int i=0;i<fps;i++)shifter.Update(dt,0,1,false,0);
        assert(shifter.gear==-1 && std::fabs(shifter.rpmNeedle-18.285f)<.001f);
        for(int gear=0;gear<=5;gear++){
            for(int i=0;i<fps*3;i++)shifter.Update(dt,10,gear,false,-1);
            assert(shifter.gear==gear);
            const float targets[]={100.0837f,108.3687f,69.9663f,49.4131f,40.028f,32.28235f};
            assert(std::fabs(shifter.rpmNeedle-targets[gear])<.002f);
        }
        for(int i=0;i<fps*3;i++)shifter.Update(dt,48.1f,5,true,100);
        assert(shifter.gear==-1 && std::fabs(shifter.rpmNeedle-103.7311f)<.002f);
        for(int i=0;i<fps*3;i++)shifter.Update(dt,1000,5,true,100);
        assert(shifter.rpmNeedle==360);
        Wipers w;w.Update(dt,true,true,true,false);assert(w.stalk==-5);
        for(int i=0;i<fps;i++)w.Update(dt,true,true,false,false);
        assert(w.angle<0 && w.mode==2);
        for(int i=0;i<fps*3;i++)w.Update(dt,false,true,false,false);
        assert(w.angle==0 && w.mode==0 && w.direction==0);
        w.Update(dt,true,true,false,false);
        w.Update(dt,true,true,true,false);assert(w.stalk==-5);
        w.Update(dt,true,true,false,false);
        w.Update(dt,true,true,true,false);assert(w.stalk==-10);
        Wipers single;single.Update(dt,true,true,false,true);
        for(int i=0;i<fps*3;i++)single.Update(dt,true,true,false,false);
        assert(single.angle==0 && single.direction==0);
        Wipers intermittent;
        for(int i=0;i<fps*2;i++)intermittent.Update(dt,true,true,false,true);
        assert(intermittent.mode==1 && intermittent.angle==0);
        for(int i=0;i<fps*5;i++)intermittent.Update(dt,true,true,false,false);
        assert(intermittent.angle==0);
        intermittent.Update(dt,true,true,false,true);
        for(int i=0;i<fps*10;i++)intermittent.Update(dt,true,true,false,false);
        assert(intermittent.mode==0 && intermittent.angle==0 && intermittent.direction==0);
        w.Update(dt,true,true,false,false);
        w.Update(dt,true,true,false,true);assert(w.stalk==-5);
        w.Update(dt,true,true,false,false);
        w.Update(dt,true,true,false,true);assert(w.stalk==0);
        for(int i=0;i<fps*3;i++)w.Update(dt,true,true,false,false);
        assert(w.mode==0 && w.angle==0 && w.direction==0);
        Wipers cycle;
        for(int repeat=0;repeat<2;repeat++){
            cycle.Update(dt,true,true,true,false);assert(cycle.stalk==-5);
            for(int i=0;i<fps;i++)cycle.Update(dt,true,true,true,false);
            assert(cycle.stalk==-5); // Held key must not cycle repeatedly.
            cycle.Update(dt,true,true,false,false);
            cycle.Update(dt,true,true,true,false);assert(cycle.stalk==-10);
            cycle.Update(dt,true,true,false,false);
            cycle.Update(dt,true,true,true,false);assert(cycle.stalk==0);
            for(int i=0;i<fps*3;i++)cycle.Update(dt,true,true,false,false);
            assert(cycle.angle==0 && cycle.direction==0 && cycle.mode==0);
        }
        Pedals p;
        for(int i=0;i<fps;i++)p.Update(dt,true,1,false,1,0,true);
        assert(p.gas==20 && p.brake==0 && p.handbrake==20);
        for(int i=0;i<fps;i++)p.Update(dt,true,1,false,0,0,false);
        assert(p.gas==0 && p.handbrake==0);
        for(int i=0;i<fps;i++)p.Update(dt,true,0,false,1,0,false);
        assert(p.gas==0 && p.brake==20);
        Windows windows;windows.leftDown=true;
        for(int i=0;i<fps*3;i++)windows.Update(dt);
        assert(windows.left==-.15f && windows.right==0);
        windows.leftDown=false;
        for(int i=0;i<fps*3;i++)windows.Update(dt);
        assert(windows.left==0);
        Signals signals;
        for(int i=0;i<fps/5;i++)signals.Update(dt,true,true,false,false);
        assert(signals.mode==1);
        bool blink=false;
        for(int i=0;i<fps;i++){signals.Update(dt,true,false,false,false);blink|=signals.blink;}
        assert(blink && signals.lever==-5);
        for(int i=0;i<fps/5;i++)signals.Update(dt,true,false,false,true);
        assert(signals.mode==3);
        Signals chords;
        for(int option=1;option<=3;option++){
            const bool ls=option==1,rs=option==2;
            chords.UpdateControls(dt,true,true,ls,rs);assert(chords.mode==option);
            for(int i=0;i<fps*2;i++)chords.UpdateControls(dt,true,true,ls,rs);
            assert(chords.mode==option); // Holding must not auto-toggle.
            chords.UpdateControls(dt,true,false,ls,rs);
            chords.UpdateControls(dt,true,true,ls,rs);assert(chords.mode==0);
            chords.UpdateControls(dt,true,false,false,false);
        }
        chords.UpdateControls(dt,true,true,true,false);assert(chords.mode==1);
        chords.UpdateControls(dt,true,true,false,true);assert(chords.mode==1);
        chords.UpdateControls(dt,true,false,false,true);
        chords.UpdateControls(dt,true,true,false,true);assert(chords.mode==2);
        chords.UpdateControls(dt,true,false,false,false);
        chords.UpdateControls(dt,true,true,false,false);assert(chords.mode==3);
        ReactorGauges gauges;gauges.Update(dt,false,true,true);
        int sounds=0;
        for(int i=0;i<fps*4;i++){
            gauges.Update(dt,true,true,true);sounds+=gauges.startupSound;
            if(!gauges.lights)assert(gauges.power==0 && gauges.Needle()==10);
            if(gauges.startupSound)assert(gauges.lights && gauges.power==0);
        }
        assert(sounds==1 && gauges.power==23 && gauges.Needle()==45);
        for(int i=0;i<fps*2;i++)gauges.Update(dt,false,true,true);
        assert(gauges.power==0 && gauges.Needle()==10);
        EngineSounds e;e.Update(dt,true,true,0,0,1,false,true,true);assert(e.start);
        e.Update(dt,true,true,255,0,1,false,true,true);assert(e.accelerate);
        e.Update(dt,true,true,255,0,1,false,false,true);assert(e.stopAcceleration);
        e.Update(dt,true,true,255,0,1,false,true,true);assert(e.accelerate);
        e.Update(dt,true,true,0,0,1,false,true,true);assert(e.decelerate);
        bool stoppedAudio=false;
        for(int i=0;i<fps;i++){e.Update(dt,true,true,0,0,1,false,true,true);stoppedAudio|=e.stopAcceleration;}
        assert(stoppedAudio);
        e.Update(dt,true,true,255,0,1,false,true,true);
        e.Update(dt,true,true,0,0,1,false,true,true);
        e.Update(.05,true,true,255,0,1,false,true,true);
        assert(e.accelerate && !e.stopAcceleration);
        for(int i=0;i<fps*2;i++)e.Update(dt,true,true,0,0,1,true,false,false);
        assert(e.hoverGain==.5f);
        e.Update(dt,false,true,0,0,1,true,false,false);assert(e.stop);
        for(int i=0;i<fps*2;i++)e.Update(dt,false,true,0,0,1,true,false,false);
        assert(e.hoverGain==0);
    }
    puts("PASS: cabin controls, reverse/released pedals, reactor gauges, engine cues and fade at 15/30/60/144 fps");
}
