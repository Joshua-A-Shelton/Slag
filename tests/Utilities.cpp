#include "Utilities.h"
#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <stb_image.h>
namespace slag
{
    std::unique_ptr<Texture> utilities::loadTextureFromFile(const std::filesystem::path& path)
    {
        int width, height, channels;
        std::unique_ptr<unsigned char> data(stbi_load(path.c_str(), &width, &height,&channels,4),[=](unsigned char* data){stbi_image_free(data);});
        return std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,width,height,1,1,Texture::SampleCount::ONE,data.get(),1,1));
    }
} // slag
