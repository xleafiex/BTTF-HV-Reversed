// Optional Bullet-backed whole-body ragdoll prototype for the player and nearby peds.
// It deliberately lives in its own Leaf package so it can be removed without changing gameplay.
#define NOMINMAX
#include <windows.h>
#include <map>
#include <btBulletDynamicsCommon.h>
#include "common.h"
#include "leaf_api.h"
#include "Ped.h"
#include "PlayerPed.h"
#include "PlayerInfo.h"
#include "Pools.h"
#include "Timer.h"
#include "Frontend.h"
#include "Hud.h"
#include "Font.h"

namespace {
LeafHost host; bool active=false,key=false;
btDefaultCollisionConfiguration *config=nil; btCollisionDispatcher *dispatcher=nil;
btBroadphaseInterface *broadphase=nil; btSequentialImpulseConstraintSolver *solver=nil;
btDiscreteDynamicsWorld *world=nil; btCollisionShape *shape=nil;
btCollisionShape *groundShape=nil; btRigidBody *groundBody=nil;
std::map<CPed*,btRigidBody*> bodies;
void Log(const char *s){if(host.log)host.log(s);}
void Help(const char *s){wchar text[128];AsciiToUnicode(s,text);CHud::SetHelpMessage(text,true);}
btRigidBody *Body(CPed *ped){
    btTransform t; t.setIdentity(); CVector p=ped->GetPosition(); t.setOrigin(btVector3(p.x,p.y,p.z));
    btScalar mass=75; btVector3 inertia(0,0,0); shape->calculateLocalInertia(mass,inertia);
    btDefaultMotionState *motion=new btDefaultMotionState(t);
    btRigidBody::btRigidBodyConstructionInfo info(mass,motion,shape,inertia);
    btRigidBody *body=new btRigidBody(info);world->addRigidBody(body);bodies[ped]=body;ped->SetPedState(PED_FALL);return body;
}
bool Init(const LeafHost *h){
    if(!h||h->abiVersion!=LEAF_ABI_VERSION)return false;host=*h;
    config=new btDefaultCollisionConfiguration;dispatcher=new btCollisionDispatcher(config);
    broadphase=new btDbvtBroadphase;solver=new btSequentialImpulseConstraintSolver;
    world=new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,config);world->setGravity(btVector3(0,0,-19.62f));
    shape=new btCapsuleShape(.32f,.95f);active=true;Log("Bullet ragdoll module active: F6 toggles player and nearby peds.");return true;
}
void Activate(){
    CPed *player=FindPlayerPed();if(!player)return; CVector origin=player->GetPosition();
    // Keep the prototype bodies near the current map surface while the game
    // continues to own the real world collision.
    groundShape=new btStaticPlaneShape(btVector3(0,0,1),0);
    btTransform groundTransform; groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(origin.x,origin.y,origin.z-1.0f));
    btDefaultMotionState *groundMotion=new btDefaultMotionState(groundTransform);
    btRigidBody::btRigidBodyConstructionInfo groundInfo(0,groundMotion,groundShape,btVector3(0,0,0));
    groundBody=new btRigidBody(groundInfo); world->addRigidBody(groundBody);
    for(int i=0;i<CPools::GetPedPool()->GetSize();++i){
        CPed *ped=CPools::GetPedPool()->GetSlot(i);if(!ped||bodies.count(ped))continue;
        CVector d=ped->GetPosition()-origin;if(ped==player||d.MagnitudeSqr()<30*30)Body(ped);
    }
    Help("Bullet ragdoll active. F6 resets the ragdoll.");Log("Bullet ragdoll bodies created for player and nearby peds.");
}
void Reset(){
    for(auto &it:bodies){CPed *ped=it.first;btRigidBody *body=it.second;world->removeRigidBody(body);delete body->getMotionState();delete body;if(ped&&!ped->Dead())ped->SetPedState(PED_IDLE);}
    bodies.clear();Help("Bullet ragdoll reset.");Log("Bullet ragdoll reset.");
    if(groundBody){world->removeRigidBody(groundBody);delete groundBody->getMotionState();delete groundBody;groundBody=nil;}
    delete groundShape; groundShape=nil;
}
void Update(){
    if(!active||FrontEndMenuManager.m_bMenuActive||CTimer::GetIsPaused())return;
    bool down=(GetAsyncKeyState(VK_F6)&0x8000)!=0;if(down&&!key){if(bodies.empty())Activate();else Reset();}key=down;
    if(bodies.empty())return;world->stepSimulation(fminf(CTimer::GetTimeStep(),3.0f)/30.0f,2,1.0f/60.0f);
    for(auto &it:bodies){btTransform t;it.second->getMotionState()->getWorldTransform(t);btVector3 p=t.getOrigin();it.first->SetPosition((float)p.x(),(float)p.y(),(float)p.z());it.first->SetMoveState(PEDMOVE_STILL);}
}
void Shutdown(){Reset();delete shape;delete world;delete solver;delete broadphase;delete dispatcher;delete config;shape=nil;world=nil;solver=nil;broadphase=nil;dispatcher=nil;config=nil;active=false;}
const LeafMod mod={LEAF_ABI_VERSION,Init,Update,nil,Shutdown};
}
extern "C" __declspec(dllexport) const LeafMod *LeafGetMod(){return &mod;}
