#include "Utilities.h"
#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <stb_image.h>
#include <functional>
template<typename T>
using unique_ptr_custom = std::unique_ptr<T,std::function<void(T*)>>;
namespace slag
{
    std::unique_ptr<Texture> utilities::loadTextureFromFile(const std::filesystem::path& path)
    {
        int width, height, channels;
        stbi_image_free(nullptr);
        unique_ptr_custom<stbi_uc> data(stbi_load(path.string().c_str(), &width, &height,&channels,4),[](stbi_uc* uc){stbi_image_free(uc);});
        return std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UNORM,Texture::Type::TEXTURE_2D,Texture::UsageFlags::SAMPLED_IMAGE,width,height,1,1,Texture::SampleCount::ONE,data.get(),1,1));
    }

    std::vector<unsigned char> utilities::loadTexelsFromFile(const std::filesystem::path& path)
    {
        int width, height, channels;
        stbi_image_free(nullptr);
        unique_ptr_custom<stbi_uc> data(stbi_load(path.string().c_str(), &width, &height,&channels,4),[](stbi_uc* uc){stbi_image_free(uc);});
        std::vector<unsigned char> result(width*height*4);
        memcpy(result.data(),data.get(),width*height*4);
        return result;
    }
} // slag
