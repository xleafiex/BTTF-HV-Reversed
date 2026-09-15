#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace DonorAudio {
struct Voice { ALuint source=0; bool resume=false; bool fixed=false; float offset[3]={0,-2,0}; };
static ALCdevice *device=nullptr;
static ALCcontext *context=nullptr;
static std::string root;
static float position[3]={};
static float carRight[3]={1,0,0},carForward[3]={0,1,0},carUp[3]={0,0,1};
static std::map<std::string,ALuint> buffers;
static std::map<std::string,Voice> voices;
struct ScopedContext {
    ALCcontext *previous;
    ScopedContext():previous(alcGetCurrentContext()){alcMakeContextCurrent(context);}
    ~ScopedContext(){alcMakeContextCurrent(previous);}
};
inline bool Init(const std::string &path) {
    root=path;
    device=alcOpenDevice(nullptr);
    if(!device) return false;
    context=alcCreateContext(device,nullptr);
    if(!context){alcCloseDevice(device);device=nullptr;return false;}
    ScopedContext use;
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    alDopplerFactor(0); // Donor attached clips retain their authored pitch.
    return true;
}
inline uint32_t U32(const unsigned char *p){return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24;}
inline void Place(Voice &v){
    float world[3];for(int i=0;i<3;++i)world[i]=v.fixed?v.offset[i]:position[i]+carRight[i]*v.offset[0]+carForward[i]*v.offset[1]+carUp[i]*v.offset[2];
    alSourcefv(v.source,AL_POSITION,world);
}
inline unsigned U16(const unsigned char *p){return unsigned(p[0]) | unsigned(p[1])<<8;}
inline ALuint Load(const char *name) {
    auto found=buffers.find(name); if(found!=buffers.end())return found->second;
    std::ifstream file(root+"/"+name,std::ios::binary|std::ios::ate);
    if(!file)return 0;
    const auto size=file.tellg(); if(size<12 || size>64*1024*1024)return 0;
    std::vector<unsigned char> bytes(static_cast<size_t>(size));
    file.seekg(0);if(!file.read(reinterpret_cast<char*>(bytes.data()),size))return 0;
    if(memcmp(bytes.data(),"RIFF",4)||memcmp(bytes.data()+8,"WAVE",4))return 0;
    unsigned channels=0,bits=0,rate=0,format=0;size_t data=0,length=0;
    for(size_t p=12;p+8<=bytes.size();) {
        const size_t n=U32(bytes.data()+p+4); if(n>bytes.size()-p-8)return 0;
        if(!memcmp(bytes.data()+p,"fmt ",4) && n>=16){
            format=U16(bytes.data()+p+8);channels=U16(bytes.data()+p+10);
            rate=U32(bytes.data()+p+12);bits=U16(bytes.data()+p+22);
        }else if(!memcmp(bytes.data()+p,"data",4)){data=p+8;length=n;}
        p+=8+n+(n&1);
    }
    if(format!=1 || (channels!=1 && channels!=2) || (bits!=8 && bits!=16) || !rate || !data)return 0;
    const unsigned bytesPerSample=bits/8;
    if(length%(channels*bytesPerSample))return 0;
    // OpenAL spatializes mono clips. Preserve the donor stereo mix by averaging
    // its channels before attaching it to the moving reactor/car.
    std::vector<int16_t> pcm(length/(channels*bytesPerSample));
    for(size_t i=0;i<pcm.size();++i){
        int sum=0;for(unsigned c=0;c<channels;++c){
            const size_t sample=i*channels+c;
            sum+=bits==8?(int(bytes[data+sample])-128)<<8:int16_t(U16(bytes.data()+data+2*sample));
        }
        pcm[i]=int16_t(sum/int(channels));
    }
    alGetError();ALuint buffer=0;alGenBuffers(1,&buffer);
    alBufferData(buffer,AL_FORMAT_MONO16,pcm.data(),ALsizei(pcm.size()*2),ALsizei(rate));
    if(alGetError()!=AL_NO_ERROR){if(buffer)alDeleteBuffers(1,&buffer);return 0;}
    buffers[name]=buffer;return buffer;
}
inline bool Play(const char *name,bool loop=false,const char *voiceName=nullptr) {
    if(!context)return false;
    ScopedContext use;
    const ALuint buffer=Load(name);if(!buffer)return false;
    const char *key=voiceName?voiceName:name;
    auto found=voices.find(key);
    if(found==voices.end() && voices.size()>=64){
        // Named one-shots used to exhaust the pool permanently after driving
        // long enough. Reclaim completed voices, preserving active loops.
        for(auto it=voices.begin();it!=voices.end();){
            ALint state;alGetSourcei(it->second.source,AL_SOURCE_STATE,&state);
            if(state!=AL_PLAYING && state!=AL_PAUSED){
                alDeleteSources(1,&it->second.source);it=voices.erase(it);
            }else ++it;
        }
        if(voices.size()>=64)return false;
    }
    Voice &voice=voices[key];
    if(!voice.source){alGetError();alGenSources(1,&voice.source);if(alGetError()!=AL_NO_ERROR){voices.erase(key);return false;}}
    ALint state;alGetSourcei(voice.source,AL_SOURCE_STATE,&state);
    if(loop && (state==AL_PLAYING || state==AL_PAUSED))return true;
    alSourceStop(voice.source);voice.resume=false;
    alSourcei(voice.source,AL_BUFFER,buffer);alSourcei(voice.source,AL_LOOPING,loop?AL_TRUE:AL_FALSE);
    alSourcef(voice.source,AL_REFERENCE_DISTANCE,5.0f);
    alSourcef(voice.source,AL_MAX_DISTANCE,100.0f);
    Place(voice);
    alSourcePlay(voice.source);return alGetError()==AL_NO_ERROR;
}
inline bool Preload(const char *name){
    if(!context)return false;
    ScopedContext use;return Load(name)!=0;
}
inline bool IsPlaying(const char *name) {
    if(!context || !voices.count(name))return false;
    ScopedContext use;ALint state;alGetSourcei(voices[name].source,AL_SOURCE_STATE,&state);
    return state==AL_PLAYING || state==AL_PAUSED;
}
inline void Stop(const char *name){
    if(!context || !voices.count(name))return;
    ScopedContext use;Voice &v=voices[name];alSourceStop(v.source);v.resume=false;
}
inline void StopAll(){if(context){ScopedContext use;for(auto &v:voices){alSourceStop(v.second.source);v.second.resume=false;}}}
inline void Configure(const char *name,float x,float y,float z,float range,float gain=1,float pitch=1){
    if(!context || !voices.count(name))return;
    ScopedContext use;Voice &v=voices[name];v.fixed=false;v.offset[0]=x;v.offset[1]=y;v.offset[2]=z;
    alSourcef(v.source,AL_REFERENCE_DISTANCE,range);alSourcef(v.source,AL_GAIN,gain);
    alSourcef(v.source,AL_PITCH,pitch);Place(v);
}
inline void ConfigureWorld(const char *name,float x,float y,float z,float range){
    Configure(name,x,y,z,range);
    if(!context || !voices.count(name))return;
    ScopedContext use;Voice &v=voices[name];v.fixed=true;Place(v);
}
inline void Update(const float *listener,const float *front,const float *up,const float *car,bool paused,float gain=1.0f,
                   const float *right=nullptr,const float *forward=nullptr,const float *vehicleUp=nullptr){
    if(!context)return;
    ScopedContext use;const float orientation[]={front[0],front[1],front[2],up[0],up[1],up[2]};
    std::copy(car,car+3,position); alListenerf(AL_GAIN,gain);
    if(right)std::copy(right,right+3,carRight);
    if(forward)std::copy(forward,forward+3,carForward);
    if(vehicleUp)std::copy(vehicleUp,vehicleUp+3,carUp);
    alListenerfv(AL_POSITION,listener);alListenerfv(AL_ORIENTATION,orientation);
    for(auto &item:voices){
        Voice &v=item.second;Place(v);
        ALint state;alGetSourcei(v.source,AL_SOURCE_STATE,&state);
        if(paused && state==AL_PLAYING){alSourcePause(v.source);v.resume=true;}
        else if(!paused && v.resume){alSourcePlay(v.source);v.resume=false;}
    }
}
inline void Shutdown(){
    if(!context)return;
    {ScopedContext use;for(auto &v:voices)alDeleteSources(1,&v.second.source);
     for(auto &b:buffers)alDeleteBuffers(1,&b.second);voices.clear();buffers.clear();}
    alcDestroyContext(context);alcCloseDevice(device);context=nullptr;device=nullptr;
}
}
