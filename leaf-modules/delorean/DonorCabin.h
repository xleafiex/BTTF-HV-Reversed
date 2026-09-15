#pragma once
#include "DonorSystems.h"

namespace DonorSystems {
inline int DisplayDigit(int value,int place){
    for(int p=1;p<place;p++)value/=10;
    return value%10;
}
struct CompassState {
    double pending=0;
    float yaw=0,pitch=0,roll=0, pitchVelocity=0,rollVelocity=0;
    // Vehicle headings are radians; the donor compass animation uses degrees.
    void UpdateVehicle(double seconds,float headingRadians,float forwardZ,float rightZ,float upZ,float longitudinalForce,float lateralForce,float speed){
        const float degrees=57.295779513f;
        const float targetPitch=-std::atan2(forwardZ,upZ)*degrees;
        const float targetRoll=std::atan2(rightZ,std::sqrt(forwardZ*forwardZ+upZ*upZ))*degrees;
        Update(seconds,-headingRadians*degrees,targetPitch,targetRoll,longitudinalForce,lateralForce,speed);
    }
    void Update(double seconds,float targetYaw,float targetPitch,float targetRoll,float longitudinalForce=0,float lateralForce=0,float speed=0){
        pending+=seconds;
        while(pending+1e-9>=1./30){pending-=1./30;
            float dy=targetYaw-yaw;while(dy>180)dy-=360;while(dy<-180)dy+=360;
            yaw+=std::max(-3.0f,std::min(3.0f,dy));
            // A buoy/ball response: tilt follows with spring and damping,
            // so a sudden vehicle bank does not snap the compass face.
            const float pitchForce=(targetPitch+longitudinalForce*1.8f+std::min(8.0f,speed*.04f)-pitch)*.16f-pitchVelocity*.22f;
            const float rollForce=(targetRoll+lateralForce*1.8f-roll)*.16f-rollVelocity*.22f;
            pitchVelocity=std::max(-3.0f,std::min(3.0f,pitchVelocity+pitchForce));
            rollVelocity=std::max(-3.0f,std::min(3.0f,rollVelocity+rollForce));
            pitch+=pitchVelocity;roll+=rollVelocity;
        }
    }
};
inline int DigitalSpeed(float metersPerSecond){return int(std::max(0.0f,std::min(88.0f,metersPerSecond*1.835f)));}
struct DoorSounds {
    bool closed[2]={true,true};
    bool Opened(int side,bool nowClosed){bool opened=closed[side] && !nowClosed;closed[side]=nowClosed;return opened;}
};
struct ConsoleClock {
    int hour=0,minute=0;
    double elapsed=0,blinkTime=0;
    void Reset(int h,int m){hour=h%24;minute=m%60;elapsed=blinkTime=0;}
    void Update(double seconds){
        elapsed+=seconds;blinkTime+=seconds;
        while(elapsed+1e-9>=60){elapsed-=60;if(++minute==60){minute=0;hour=(hour+1)%24;}}
        while(blinkTime+1e-9>=1)blinkTime-=1;
    }
    int DisplayHour()const{return hour%12?hour%12:12;}
    bool Colon()const{return blinkTime<.5-1e-9;}
    float HourHand()const{return hour*30.0f+minute*.5f;}
    float MinuteHand()const{return minute*6.0f;}
};
struct EmergencyLight {
    bool on=false;
    double pending=0;
    float handle=0;
    void Update(double seconds,bool toggle){
        if(toggle)on=!on;
        pending+=seconds;
        while(pending+1e-9>=1./30){pending-=1./30;handle=Approach(handle,on?30:0,5,5);}
    }
};
struct UnderbodyLights {
    double pending=0, phase=0;
    float alpha=0;
    int chaser=0;
    void Update(double seconds,bool fitted,bool folded){
        phase+=seconds;
        const bool on=fitted && folded;
        if(on && phase>=1.25)phase=std::fmod(phase,1.25);
        chaser=on?std::min(5,int((phase+1e-9)/.25)+1):0;
        pending+=seconds;
        while(pending+1e-9>=1./30){pending-=1./30;alpha=Approach(alpha,on?255:0,15,15);}
    }
};
struct Shifter {
    double pending=0;
    int gear=-1;
    float rpmNeedle=0;
    void Update(double seconds,float speed,int currentGear,bool folded,float rearRightWheelSpeed){
        // Donor shifter.cpp selects neutral below 1 m/s and in hover mode.
        gear=speed>1 && !folded?std::max(0,std::min(5,currentGear)):-1;
        const float ratios[][2]={{7.602f,18.285f},{7.602f,10.0f},{7.602f,18.285f},
            {4.838f,12.636f},{3.326f,10.0f},{2.534f,10.0f},{1.971f,8.926f}};
        const float *ratio=ratios[gear+1];
        const float target=folded?std::max(18.285f,std::min(360.0f,speed*1.971f+8.926f)):
            std::max(5.0f,std::min(360.0f,11.85f*std::fabs(rearRightWheelSpeed)*ratio[0]+ratio[1]));
        pending+=seconds;
        while(pending+1e-9>=1./30){pending-=1./30;rpmNeedle=Approach(rpmNeedle,target,5,5);}
    }
};
struct Wipers {
    double pending=0, parked=0;
    float stalk=0, lever=0, angle=0;
    int mode=0, direction=0;
    bool held=false, upSound=false, downSound=false;
    void Update(double seconds,bool engine,bool occupied,bool u,bool i) {
        upSound=downSound=false;
        // Turning the engine off always parks the blades.  The donor motor
        // still completes its return stroke, so the visual state cannot stay
        // stuck in an active sweep after the switch is released.
        if(!engine && (angle < -0.01f || direction!=0)){
            stalk=0; mode=0; direction=angle < -0.01f ? -1 : 0;
        }
        if(!u && !i)held=false;
        if(!i && stalk==5)stalk=0;
        if(occupied && !held){
            if(u){
                held=true;
                // User control: one key cycles normal, fast, then parked.
                if(stalk==-10 || mode==1){stalk=0;mode=0;direction=angle<0?-1:0;}
                else stalk=stalk==-5?-10:-5;
            }
            else if(i && stalk<5){
                held=true;
                // Intermittent has a neutral stalk. I explicitly parks it
                // instead of entering another momentary/intermittent sweep.
                if(mode==1 && stalk==0){mode=0;direction=angle<0?-1:0;}
                else stalk+=5;
                if(stalk==0){mode=0;direction=angle<0?-1:0;}
                if(stalk==5){mode=0;parked=0;if(!direction)direction=1;}
            }
        }
        pending+=seconds;
        while(pending+1e-9>=1./30){
            pending-=1./30;parked+=1./30;
            lever=Approach(lever,stalk,1,1);
            if(stalk<0 && !direction)direction=1;
            if(stalk==0 && mode!=1)mode=0;
            else if(stalk==-5)mode=2;
            else if(stalk==-10)mode=3;
            const float speed=stalk==-10?6:3;
            if(direction==1){
                if(mode==1 && parked<8)continue;
                upSound=true;angle-=speed;
                if(angle<-65){angle=-65;direction=-1;}
            }else if(direction==-1){
                downSound=true;angle+=speed;
                if(angle>0){angle=0;parked=0;if(stalk==5)mode=1;direction=mode?1:0;}
            }
        }
    }
};
struct Pedals {
    double pending=0, shiftAge=1;
    float gas=0, brake=0, clutch=0, handbrake=0;
    int previousGear=0;
    void Update(double seconds,bool occupied,int gear,bool flying,float throttle,float braking,bool hand){
        if(occupied && gear!=previousGear){previousGear=gear;shiftAge=0;}
        pending+=seconds;
        while(pending+1e-9>=1./30){
            pending-=1./30;
            handbrake=Approach(handbrake,occupied && !flying && hand?20:0,4,4);
            if(!occupied)continue;
            float targetGas=(gear==0?braking:throttle)*20;
            float targetBrake=(gear==0?throttle:braking)*20;
            const float targetClutch=!flying && shiftAge<.1?20:0;
            if(targetClutch)targetGas=0;
            // Clamp release too: the donor helper sets a negative delta to zero,
            // which otherwise leaves a fully pressed pedal stuck indefinitely.
            gas=Approach(gas,targetGas,5,5);brake=Approach(brake,targetBrake,5,5);
            clutch=Approach(clutch,targetClutch,5,5);shiftAge+=1./30;
        }
    }
};
struct Signals {
    double pending=0, phase=0, hazardAge=1;
    int mode=0;
    float lever=0;
    bool blink=false, onSound=false, offSound=false;
    bool keyHeld=false;
    void UpdateControls(double seconds,bool occupied,bool l,bool leftShift,bool rightShift){
        Update(seconds,occupied,l && leftShift && !rightShift,
            l && rightShift && !leftShift,l && leftShift==rightShift);
    }
    void Update(double seconds,bool occupied,bool left,bool right,bool hazards){
        onSound=offSound=false;
        const bool pressed=left || right || hazards;
        // Sample the press before the animation tick so short taps work at
        // high frame rates. A held L (even with changing Shift) toggles once.
        if(occupied && pressed && !keyHeld){
            const int request=left?1:right?2:3;
            mode=mode==request?0:request;
            if(request==3)hazardAge=0;
        }
        keyHeld=pressed;
        pending+=seconds;
        while(pending+1e-9>=1./30){
            pending-=1./30;hazardAge+=1./30;phase+=1./30;
            if(blink && (!mode || phase>=.8)){blink=false;offSound=true;phase=0;}
            else if(!blink && mode && phase>.4){blink=true;onSound=true;phase=.4;}
            lever=Approach(lever,mode==1?-5:mode==2?5:0,1,1);
        }
    }
    float HazardButton()const{return hazardAge<.1?.01f:mode==3?.005f:0;}
};
struct Windows {
    double pending=0;
    float left=0,right=0,leftSwitch=0,rightSwitch=0;
    bool leftDown=false,rightDown=false;
    void Update(double seconds){
        pending+=seconds;
        while(pending+1e-9>=1./30){
            pending-=1./30;
            const float targetLeft=leftDown?-.15f:0,targetRight=rightDown?-.15f:0;
            leftSwitch=left>targetLeft?-30:left<targetLeft?30:0;
            rightSwitch=right>targetRight?-30:right<targetRight?30:0;
            left=Approach(left,targetLeft,.002f,.002f);right=Approach(right,targetRight,.002f,.002f);
        }
    }
};
struct ReactorGauges {
    double pending=0, switched=5;
    float geiger=10,power=0;
    bool wasOn=true, lights=false, startupSound=false;
    void Update(double seconds,bool on,bool fueled,bool plutonium){
        startupSound=false;
        if(wasOn!=on){switched=on?0:5;wasOn=on;}
        const double before=switched;switched+=seconds;
        if(before<2 && switched>=2 && on)startupSound=true;
        lights=on && switched>=2;
        // Finish lamp warm-up and expose one lit frame before needle motion.
        const bool ready=fueled && lights && !startupSound && switched>=2.0+1.0/30.0;
        pending+=seconds;
        while(pending+1e-9>=1./30){
            pending-=1./30;
            power=Approach(power,ready?23:0,1,1);
        // The physical plutonium needle follows the energized time-circuit
        // state.  A fueled reactor alone is not enough while the circuits
        // are switched off.
        geiger=Approach(geiger,ready && on && plutonium?45:0,1,1);
        }
    }
    float Needle()const{return std::max(10.0f,geiger);}
};
struct EngineSounds {
    bool running=false, accelerating=false;
    bool start=false, stop=false, accelerate=false, decelerate=false, stopAcceleration=false;
    double delayedStop=-1, pending=0;
    float hoverGain=0;
    void Update(double seconds,bool on,bool occupied,int gas,int brake,int gear,bool folded,bool ground,bool grip){
        start=on && !running;stop=!on && running;running=on;
        accelerate=decelerate=stopAcceleration=false;
        if(delayedStop>=0){delayedStop-=seconds;if(delayedStop<=0){stopAcceleration=true;delayedStop=-1;}}
        const bool requesting=occupied && gas>=150 && brake==0 && gear>0 && on;
        if(folded || (requesting && (!ground || !grip))){
            if(accelerating)stopAcceleration=true;
            accelerating=false;
        }else if(requesting){
            if(!accelerating){accelerate=true;delayedStop=-1;stopAcceleration=false;}
            accelerating=true;
        }else if(accelerating){decelerate=true;delayedStop=.05;accelerating=false;}
        pending+=seconds;
        while(pending+1e-9>=1./30){pending-=1./30;hoverGain=Approach(hoverGain,on && folded?.5f:0,.01f,.01f);}
    }
};
}
