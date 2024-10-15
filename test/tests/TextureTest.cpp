#include "gtest/gtest.h"
#include "slag/SlagLib.h"
using namespace slag;

TEST(Texture, LoadFromFile)
{
    auto texture = Texture::newTexture("resources/Crucible.png",5,TextureUsageFlags::SAMPLED_IMAGE,false,Texture::Layout::SHADER_RESOURCE);
    GTEST_ASSERT_GE(texture->sampleCount() ,1);
    GTEST_ASSERT_GE(texture->mipLevels() ,5);
    GTEST_ASSERT_GE(texture->type() ,Texture::Type::TEXTURE_2D);
    GTEST_ASSERT_GE(texture->layers() ,1);
    GTEST_ASSERT_GE(texture->width() ,1920);
    GTEST_ASSERT_GE(texture->height() ,1080);

    delete texture;
}

TEST(Texture, MultiSampled)
{
    auto texture = Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,500,700,16,TextureUsageFlags::RENDER_TARGET_ATTACHMENT,Texture::Layout::RENDER_TARGET);
    GTEST_ASSERT_GE(texture->sampleCount() ,16);
    GTEST_ASSERT_GE(texture->mipLevels() ,1);
    GTEST_ASSERT_GE(texture->type() ,Texture::Type::TEXTURE_2D);
    GTEST_ASSERT_GE(texture->layers() ,1);
    GTEST_ASSERT_GE(texture->width() ,500);
    GTEST_ASSERT_GE(texture->height() ,700);
    delete texture;
}