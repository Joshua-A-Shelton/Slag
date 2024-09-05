#include "gtest/gtest.h"
#include "slag/SlagLib.h"
using namespace slag;

TEST(Texture, Creation)
{
    auto texture = Texture::newTexture("resources/Crucible.png",Pixels::R8G8B8A8_UINT,1,Texture::Usage::SAMPLED_IMAGE,Texture::Layout::SHADER_RESOURCE,false);
    delete texture;
}

TEST(Texture, Mipmaps)
{
    auto texture = Texture::newTexture("resources/Crucible.png",Pixels::R8G8B8A8_UINT,4,Texture::Usage::SAMPLED_IMAGE,Texture::Layout::SHADER_RESOURCE,true);
    delete texture;
}