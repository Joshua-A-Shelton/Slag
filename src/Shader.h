#ifndef SLAGLIB_SHADER_H
#define SLAGLIB_SHADER_H
#include "GraphicsCard.h"
#include "FrameBuffer.h"
namespace slag
{
    class Shader
    {
    public:
        Shader()=delete;
        Shader(const Shader&)=delete;
        Shader& operator=(const Shader&)=delete;
        Shader(GraphicsCard* card, uint32_t* vertexCode, size_t vertexCodeSize, const char* vertexEntryPointName, uint32_t* fragmentCode, size_t fragmentCodeSize, const char* fragmentEntryPointName, std::shared_ptr<FrameBuffer> framebuffer);
        ~Shader();
        ///Call if the Shader is in the same scope as slag::destroy, otherwise RAII will take care of it
        void destroy();
    private:
        void* _vkPipelineLayout;
        void* _vkPipeline;
        GraphicsCard* _card;
        bool _hasCleanedUp = false;

        friend class Renderer;
    };
    
}

#endif //SLAGLIB_SHADER_H