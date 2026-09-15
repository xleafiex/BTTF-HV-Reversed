#pragma once
// Shared by the leaf and the isolated material preview.
inline rw::Texture *LoadIceSurfaceTexture(const std::string &path){
    FILE *file=fopen(path.c_str(),"rb");if(!file)return nullptr;
    rw::uint32 size[2];
    if(fread(size,4,2,file)!=2 || !size[0] || !size[1] || size[0]>4096 || size[1]>4096){fclose(file);return nullptr;}
    rw::Image *im=rw::Image::create(size[0],size[1],32);if(!im){fclose(file);return nullptr;}im->allocate();
    bool valid=im->pixels!=nullptr;
    for(unsigned y=0;valid && y<size[1];++y)valid=fread(im->pixels+y*im->stride,4,size[0],file)==size[0];
    fclose(file);if(!valid){im->destroy();return nullptr;}
    rw::Raster *r=rw::Raster::create(size[0],size[1],32,rw::Raster::TEXTURE|rw::Raster::C8888|rw::Raster::MIPMAP|rw::Raster::AUTOMIPMAP);
    if(r)r->setFromImage(im);im->destroy();if(!r)return nullptr;
    rw::Texture *t=rw::Texture::create(r);if(!t){r->destroy();return nullptr;}
    t->setFilter(rw::Texture::LINEARMIPLINEAR);t->setAddressU(rw::Texture::WRAP);t->setAddressV(rw::Texture::WRAP);
    return t;
}
