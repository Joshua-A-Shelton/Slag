#include "gtest/gtest.h"
#include "slag/SlagLib.h"
using namespace slag;

TEST(Texture, Creation)
{
    auto texture = Texture::newTexture("resources/Crucible.png",Pixels::R8G8B8A8_UINT,1,Texture::Usage::SAMPLED_IMAGE,Texture::Layout::SHADER_RESOURCE,false);
    delete texture;
    int i=0;
}

TEST(Texture, Mipmaps)
{
    auto texture = Texture::newTexture("resources/Crucible.png",Pixels::R8G8B8A8_UINT,4,Texture::Usage::SAMPLED_IMAGE,Texture::Layout::SHADER_RESOURCE,true);
    delete texture;

    assert(false && "no generating texture on graphics buffer implemented");
    assert(false && "no generating texture on compute buffer implemented");
    //assert throw error
    assert(false && "no generating texture on transfer buffer implemented");
}