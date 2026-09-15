// Noise volume ported from the user's VCMiamiSky WeatherRenderer.h.
#pragma once
namespace LeafCloud {
    static float Hash(int x,int y,int z,int period) {
        uint32_t n=(x&(period-1))*73856093u^(y&(period-1))*19349663u^(z&(period-1))*83492791u;
        n=(n^(n>>13))*1274126177u;return (n&65535)/65535.f;
    }
    static float Noise(float x,float y,float z,int period){
        int ix=int(std::floor(x)),iy=int(std::floor(y)),iz=int(std::floor(z));
        float a=x-ix,b=y-iy,c=z-iz;a=a*a*(3-2*a);b=b*b*(3-2*b);c=c*c*(3-2*c);
        float result=0;
        for(int k=0;k<2;++k)for(int j=0;j<2;++j)for(int i=0;i<2;++i)
            result+=Hash(ix+i,iy+j,iz+k,period)*(i?a:1-a)*(j?b:1-b)*(k?c:1-c);
        return result;
    }
    static float Cellular(float x,float y,float z){
        int ix=int(std::floor(x)),iy=int(std::floor(y)),iz=int(std::floor(z));float nearest=3;
        for(int k=-1;k<=1;++k)for(int j=-1;j<=1;++j)for(int i=-1;i<=1;++i){
            int a=ix+i,b=iy+j,c=iz+k;
            float dx=a+Hash(a,b,c,8)-x,dy=b+Hash(a+31,b+17,c+7,8)-y,dz=c+Hash(a+13,b+27,c+43,8)-z;
            nearest=std::min(nearest,dx*dx+dy*dy+dz*dz);
        }
        return fminf(1.f,fmaxf(0.f,1-std::sqrt(nearest)));
    }
}

