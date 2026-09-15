#include "../delorean/DonorAudio.h"
#include <cassert>
#include <cstdio>
int main(int argc,char **argv){
    assert(argc==2);
    ALCdevice *hostDevice=alcOpenDevice(nullptr);assert(hostDevice);
    ALCcontext *hostContext=alcCreateContext(hostDevice,nullptr);assert(hostContext);
    alcMakeContextCurrent(hostContext);
    assert(DonorAudio::Init(argv[1]));assert(alcGetCurrentContext()==hostContext);
    for(const char*clip:{"delorean/timetravel.wav","delorean/cold.wav","delorean/plut_gauge.wav"}){
        assert(DonorAudio::Preload(clip));
        assert(DonorAudio::Play(clip));DonorAudio::Stop(clip);
    }
    for(int i=0;i<100;i++){
        char voice[32];std::snprintf(voice,sizeof(voice),"recycle-%d",i);
        assert(DonorAudio::Play("delorean/plut_gauge.wav",false,voice));DonorAudio::Stop(voice);
    }
    assert(DonorAudio::Play("delorean/landspeeder_loop_lower_pitch.wav",true));
    assert(DonorAudio::Play("delorean/fusion_open.wav"));
    for(int digit=0;digit<10;digit++){
        char tone[8];std::snprintf(tone,sizeof(tone),"%d.wav",digit);
        assert(DonorAudio::Play(tone,false,tone));
    }
    assert(DonorAudio::Play("delorean/door.wav",false,"door-left"));
    assert(DonorAudio::Play("delorean/door.wav",false,"door-right"));
    for(const char *clip:{"delorean/sparks.wav","delorean/plate.wav","delorean/plate_fall.wav",
        "delorean/landspeeder_accelerate_2_lower_pitch.wav",
        "delorean/landspeeder_decelerate_2_lower_pitch.wav","delorean/wheel_thrust.wav",
        "instant_timetravel.wav"})assert(DonorAudio::Play(clip,false,clip));
    assert(DonorAudio::voices["door-left"].source!=DonorAudio::voices["door-right"].source);
    DonorAudio::Stop("door-left");assert(DonorAudio::IsPlaying("door-right"));
    assert(DonorAudio::IsPlaying("delorean/landspeeder_loop_lower_pitch.wav"));
    assert(DonorAudio::IsPlaying("delorean/fusion_open.wav"));
    assert(alcGetCurrentContext()==hostContext);
    const float pos[]={1,2,3},front[]={0,1,0},up[]={0,0,1};
    DonorAudio::Update(pos,front,up,pos,true);
    const char *loop="delorean/landspeeder_loop_lower_pitch.wav";
    DonorAudio::Configure(loop,2,-1,.5f,20,.25f);
    const float right[]={0,1,0},forward[]={-1,0,0};
    DonorAudio::Update(pos,front,up,pos,true,1,right,forward,up);
    {DonorAudio::ScopedContext ctx;float xyz[3],gain,range;
     const ALuint source=DonorAudio::voices[loop].source;
     alGetSourcefv(source,AL_POSITION,xyz);alGetSourcef(source,AL_GAIN,&gain);alGetSourcef(source,AL_REFERENCE_DISTANCE,&range);
     assert(xyz[0]==2 && xyz[1]==4 && xyz[2]==3.5f && gain==.25f && range==20);}
    DonorAudio::ConfigureWorld(loop,30,40,50,50);
    const float movedCar[]={100,200,300};
    DonorAudio::Update(pos,front,up,movedCar,true,1,right,forward,up);
    {DonorAudio::ScopedContext ctx;float xyz[3];alGetSourcefv(DonorAudio::voices[loop].source,AL_POSITION,xyz);assert(xyz[0]==30 && xyz[1]==40 && xyz[2]==50);}
    // Reconfiguring an existing voice as attached restores vehicle following.
    DonorAudio::Configure(loop,2,-1,.5f,20);
    {DonorAudio::ScopedContext ctx;float xyz[3];alGetSourcefv(DonorAudio::voices[loop].source,AL_POSITION,xyz);assert(xyz[0]==101 && xyz[1]==202 && xyz[2]==300.5f);}
    assert(alcGetCurrentContext()==hostContext);
    {DonorAudio::ScopedContext ctx;ALint state;
     alGetSourcei(DonorAudio::voices["delorean/fusion_open.wav"].source,AL_SOURCE_STATE,&state);assert(state==AL_PAUSED);}
    DonorAudio::Stop("delorean/fusion_open.wav");
    DonorAudio::Update(pos,front,up,pos,false);
    assert(!DonorAudio::IsPlaying("delorean/fusion_open.wav"));
    assert(DonorAudio::IsPlaying("delorean/landspeeder_loop_lower_pitch.wav"));
    assert(!DonorAudio::Play("missing.wav"));
    DonorAudio::StopAll();assert(!DonorAudio::IsPlaying("delorean/landspeeder_loop_lower_pitch.wav"));
    DonorAudio::Shutdown();assert(alcGetCurrentContext()==hostContext);
    alcMakeContextCurrent(nullptr);alcDestroyContext(hostContext);alcCloseDevice(hostDevice);
    puts("PASS: concurrent voices, named stop, pause/resume, missing file, host context restoration, cleanup (null audio driver)");
}
