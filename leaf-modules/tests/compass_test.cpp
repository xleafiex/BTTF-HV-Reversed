#include "../delorean/DonorCabin.h"
#include <cassert>
#include <cstdio>
using namespace DonorSystems;
int main(){for(int fps:{15,30,60,144}){const double dt=1.0/fps;
        // Exercise the radians-to-degrees boundary used by the real vehicle.
        CompassState vehicleCompass;
        for(int i=0;i<fps*3;i++)vehicleCompass.UpdateVehicle(dt,1.570796327f,0,0,1,0,0,0);
        assert(std::fabs(vehicleCompass.yaw+90)<.01f);
        for(int i=0;i<fps*3;i++)vehicleCompass.UpdateVehicle(dt,-1.570796327f,.5f,0,.8660254f,0,0,0);
        assert(std::fabs(vehicleCompass.yaw-90)<.01f);
        assert(std::fabs(vehicleCompass.pitch+30)<2.0f);
        // Braking deflects the floating ball, then it settles at rest.
        for(int i=0;i<fps;i++)vehicleCompass.UpdateVehicle(dt,0,0,0,1,6,0,0);
        assert(vehicleCompass.pitch>5);
        for(int i=0;i<fps*4;i++)vehicleCompass.UpdateVehicle(dt,0,0,0,1,0,0,0);
        assert(std::fabs(vehicleCompass.pitch)<.1f);
}
puts("Compass heading, gravity tilt, braking response and settling passed at 15/30/60/144 fps");}
