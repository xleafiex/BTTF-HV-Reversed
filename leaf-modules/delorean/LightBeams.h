#pragma once

// Nested, feathered cone surfaces: no opaque end cap or rectangular sprite.
// The caller owns render states; normal scene depth hides the volume behind objects.
static void DrawLightVolume(const CVector &origin, CVector direction, CVector right,
    float length, float radius, uint8 red, uint8 green, uint8 blue, float strength)
{
    direction.Normalise();
    right-=direction*DotProduct(right,direction);
    right.Normalise();
    CVector up=CrossProduct(direction,right);
    const int sides=24, rings=10, layers=5;
    RwIm3DVertex vertices[(rings+1)*(sides+1)];
    RwImVertexIndex indices[rings*sides*6];
    int n=0;
    for(int r=0;r<rings;r++) for(int s=0;s<sides;s++) {
        int a=r*(sides+1)+s,b=a+sides+1;
        indices[n++]=a;indices[n++]=b;indices[n++]=a+1;
        indices[n++]=a+1;indices[n++]=b;indices[n++]=b+1;
    }
    for(int layer=layers;layer>0;--layer) {
        float radial=float(layer)/layers;
        for(int r=0;r<=rings;r++) {
            float t=float(r)/rings;
            float fade=sinf(PI*t)*(1.f-t);
            // Keep a broad base at the lamp and taper gently toward the end.
            // A very small radius reads as a line at normal Vice City camera distance.
            float width=(.08f+radius*t)*radial;
            for(int s=0;s<=sides;s++) {
                float angle=2.f*PI*s/sides;
                CVector p=origin+direction*(length*t)+(right*cosf(angle)+up*sinf(angle))*width;
                auto &v=vertices[r*(sides+1)+s];
                RwIm3DVertexSetPos(&v,p.x,p.y,p.z);
                // Fade the outside shell to avoid visible polygon facets.
                float shell=powf(1.f-radial,1.6f);
                RwIm3DVertexSetRGBA(&v,red,green,blue,(uint8)(strength*fade*shell));
            }
        }
        if(RwIm3DTransform(vertices,(rings+1)*(sides+1),nil,rwIM3D_VERTEXXYZ|rwIM3D_VERTEXRGBA)) {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST,indices,n);
            RwIm3DEnd();
        }
    }
}
