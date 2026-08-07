#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../utilities/GeneralUtilities.h"
#include "slag/exceptions/ResourceCreationError.h"
using namespace slag;

TEST(Texture, Create)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto tex1d = std::unique_ptr<Texture>(card->newTexture1D(256,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::NONE,3));
    auto tex2d = std::unique_ptr<Texture>(card->newTexture2D(100,250,PixelFormat::R32_FLOAT,TextureUsageFlags::COLOR_TARGET | TextureUsageFlags::READ_WRITE,2,SampleCount::ONE));
    auto tex2dDepth = std::unique_ptr<Texture>(card->newTexture2D(1920,1080,PixelFormat::D32_FLOAT_S8X24_UINT,TextureUsageFlags::DEPTH_STENCIL_TARGET,1,SampleCount::EIGHT));
    auto tex2dDepthMultiMip = std::unique_ptr<Texture>(card->newTexture2D(1920,1080,PixelFormat::D32_FLOAT_S8X24_UINT,TextureUsageFlags::DEPTH_STENCIL_TARGET,3,SampleCount::ONE));
    auto tex2dArray = std::unique_ptr<Texture>(card->newTexture2D(50,50,PixelFormat::BC7_UNORM_SRGB,TextureUsageFlags::SAMPLED,2,SampleCount::ONE,5));
    auto tex3d = std::unique_ptr<Texture>(card->newTexture3D(25,25,25,PixelFormat::R32G32B32A32_FLOAT,TextureUsageFlags::SAMPLED | TextureUsageFlags::READ_WRITE,3));
    auto cube = std::unique_ptr<Texture>(card->newTextureCube(500,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED,3,2));

    GTEST_ASSERT_EQ(tex1d->width(),256);
    GTEST_ASSERT_EQ(tex1d->height(),1);
    GTEST_ASSERT_EQ(tex1d->depth(),1);
    GTEST_ASSERT_EQ(tex1d->layers(),1);
    GTEST_ASSERT_EQ(tex1d->format(),PixelFormat::R8G8B8A8_UNORM);
    GTEST_ASSERT_EQ(tex1d->usage(),TextureUsageFlags::NONE);
    GTEST_ASSERT_EQ(tex1d->mipLevels(),3);
    GTEST_ASSERT_EQ(tex1d->sampleCount(),SampleCount::ONE);
    GTEST_ASSERT_EQ(tex1d->type(), TextureType::ONE_DIMENSIONAL);
    GTEST_ASSERT_EQ(tex1d->graphicsCard(),card);

    GTEST_ASSERT_EQ(tex2d->width(),100);
    GTEST_ASSERT_EQ(tex2d->height(),250);
    GTEST_ASSERT_EQ(tex2d->depth(),1);
    GTEST_ASSERT_EQ(tex2d->layers(),1);
    GTEST_ASSERT_EQ(tex2d->format(),PixelFormat::R32_FLOAT);
    GTEST_ASSERT_EQ(tex2d->usage(),TextureUsageFlags::COLOR_TARGET | TextureUsageFlags::READ_WRITE);
    GTEST_ASSERT_EQ(tex2d->mipLevels(),2);
    GTEST_ASSERT_EQ(tex2d->sampleCount(),SampleCount::ONE);
    GTEST_ASSERT_EQ(tex2d->type(), TextureType::TWO_DIMENSIONAL);
    GTEST_ASSERT_EQ(tex2d->graphicsCard(),card);

    GTEST_ASSERT_EQ(tex2dDepth->width(),1920);
    GTEST_ASSERT_EQ(tex2dDepth->height(),1080);
    GTEST_ASSERT_EQ(tex2dDepth->depth(),1);
    GTEST_ASSERT_EQ(tex2dDepth->layers(),1);
    GTEST_ASSERT_EQ(tex2dDepth->format(),PixelFormat::D32_FLOAT_S8X24_UINT);
    GTEST_ASSERT_EQ(tex2dDepth->usage(),TextureUsageFlags::DEPTH_STENCIL_TARGET);
    GTEST_ASSERT_EQ(tex2dDepth->mipLevels(),1);
    GTEST_ASSERT_EQ(tex2dDepth->sampleCount(),SampleCount::EIGHT);
    GTEST_ASSERT_EQ(tex2dDepth->type(), TextureType::TWO_DIMENSIONAL);
    GTEST_ASSERT_EQ(tex2dDepth->graphicsCard(),card);

    GTEST_ASSERT_EQ(tex2dDepthMultiMip->width(),1920);
    GTEST_ASSERT_EQ(tex2dDepthMultiMip->height(),1080);
    GTEST_ASSERT_EQ(tex2dDepthMultiMip->depth(),1);
    GTEST_ASSERT_EQ(tex2dDepthMultiMip->layers(),1);
    GTEST_ASSERT_EQ(tex2dDepthMultiMip->format(),PixelFormat::D32_FLOAT_S8X24_UINT);
    GTEST_ASSERT_EQ(tex2dDepthMultiMip->usage(),TextureUsageFlags::DEPTH_STENCIL_TARGET);
    GTEST_ASSERT_EQ(tex2dDepthMultiMip->mipLevels(),3);
    GTEST_ASSERT_EQ(tex2dDepthMultiMip->sampleCount(),SampleCount::ONE);
    GTEST_ASSERT_EQ(tex2dDepthMultiMip->type(), TextureType::TWO_DIMENSIONAL);
    GTEST_ASSERT_EQ(tex2dDepthMultiMip->graphicsCard(),card);

    GTEST_ASSERT_EQ(tex2dArray->width(),50);
    GTEST_ASSERT_EQ(tex2dArray->height(),50);
    GTEST_ASSERT_EQ(tex2dArray->depth(),1);
    GTEST_ASSERT_EQ(tex2dArray->layers(),5);
    GTEST_ASSERT_EQ(tex2dArray->format(),PixelFormat::BC7_UNORM_SRGB);
    GTEST_ASSERT_EQ(tex2dArray->usage(),TextureUsageFlags::SAMPLED);
    GTEST_ASSERT_EQ(tex2dArray->mipLevels(),2);
    GTEST_ASSERT_EQ(tex2dArray->sampleCount(),SampleCount::ONE);
    GTEST_ASSERT_EQ(tex2dArray->type(), TextureType::TWO_DIMENSIONAL);
    GTEST_ASSERT_EQ(tex2dArray->graphicsCard(),card);

    GTEST_ASSERT_EQ(tex3d->width(),25);
    GTEST_ASSERT_EQ(tex3d->height(),25);
    GTEST_ASSERT_EQ(tex3d->depth(),25);
    GTEST_ASSERT_EQ(tex3d->layers(),1);
    GTEST_ASSERT_EQ(tex3d->format(),PixelFormat::R32G32B32A32_FLOAT);
    GTEST_ASSERT_EQ(tex3d->usage(),TextureUsageFlags::SAMPLED | TextureUsageFlags::READ_WRITE);
    GTEST_ASSERT_EQ(tex3d->mipLevels(),3);
    GTEST_ASSERT_EQ(tex3d->sampleCount(),SampleCount::ONE);
    GTEST_ASSERT_EQ(tex3d->type(), TextureType::THREE_DIMENSIONAL);
    GTEST_ASSERT_EQ(tex3d->graphicsCard(),card);

    GTEST_ASSERT_EQ(cube->width(),500);
    GTEST_ASSERT_EQ(cube->height(),500);
    GTEST_ASSERT_EQ(cube->depth(),1);
    GTEST_ASSERT_EQ(cube->layers(),12);
    GTEST_ASSERT_EQ(cube->format(),PixelFormat::R8G8B8A8_UNORM);
    GTEST_ASSERT_EQ(cube->usage(),TextureUsageFlags::SAMPLED);
    GTEST_ASSERT_EQ(cube->mipLevels(),3);
    GTEST_ASSERT_EQ(cube->sampleCount(),SampleCount::ONE);
    GTEST_ASSERT_EQ(cube->type(), TextureType::CUBE_MAP);
    GTEST_ASSERT_EQ(cube->graphicsCard(),card);
}

#if SLAG_DEBUG
TEST(Texture, InvalidWidth)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture1D(0,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED)),"Texture must have a width of at least 1");
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture2D(0,50,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED)),"Texture must have a width of at least 1");
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture3D(0,50,50,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED)),"Texture must have a width of at least 1");
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTextureCube(0,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED)),"Texture must have a dimension of at least 1");
}

TEST(Texture, InvalidHeight)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture2D(50,0,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED)),"Texture must have a height of at least 1");
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture3D(50,0,50,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED)),"Texture must have a height of at least 1");
}

TEST(Texture, InvalidDepth)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture3D(50,50,0,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED)),"Texture must have a depth of at least 1");
}

TEST(Texture, InvalidLayers)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture2D(50,50,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED,1,SampleCount::ONE,0)),"Texture must have a layer count of at least 1");
}

TEST(Texture, InvalidMips)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture1D(50,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED,0)),"Texture must have a mip level count of at least 1");
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture2D(50,50,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED,0)),"Texture must have a mip level count of at least 1");
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture3D(50,50,50,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED,0)),"Texture must have a mip level count of at least 1");
    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTextureCube(50,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED,0)),"Texture must have a mip level count of at least 1");
}

TEST(Texture, InvalidColorAndDepthUsage)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);

    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture2D(25,25,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::COLOR_TARGET | TextureUsageFlags::DEPTH_STENCIL_TARGET)),"Texture cannot be both a color target and a depth/stencil target");
}

TEST(Texture, InvalidUnDefinedFormat)
{
    auto card = Slag::backend()->graphicsCard(0);
    EXPECT_THROW(auto tex = std::unique_ptr<Texture>(card->newTexture2D(25,25,PixelFormat::UNDEFINED,TextureUsageFlags::NONE)),ResourceCreationError);
}

TEST(Texture, InvalidColorUsageFormat)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);
    auto texelFormats = sequentialEnumRange(PixelFormat::UNDEFINED, PixelFormat::B4G4R4A4_UNORM);
    bool performedTest = false;
    for (auto format : texelFormats)
    {
        PixelFormatProperties properties = card->formatProperties(format);
        if (properties.tiling == TextureTiling::UNSUPPORTED && format != PixelFormat::UNDEFINED)
        {
            EXPECT_THROW(auto tex = std::unique_ptr<Texture>(card->newTexture2D(25,25,format,TextureUsageFlags::NONE)),ResourceCreationError);
            performedTest = true;
        }
    }
    if (!performedTest)
    {
        GTEST_SKIP();
    }
}

TEST(Texture,MipSampleCounts)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);

    EXPECT_DEATH(auto tex = std::unique_ptr<Texture>(card->newTexture2D(100,100,PixelFormat::R8G8B8A8_UNORM,TextureUsageFlags::SAMPLED,2,SampleCount::TWO)),"Texture cannot have both multiple mip levels and have a sample count greater than one");
}
#endif