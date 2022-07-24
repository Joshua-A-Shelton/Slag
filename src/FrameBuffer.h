#ifndef SLAGLIB_FRAMEBUFFER_H
#define SLAGLIB_FRAMEBUFFER_H
#include "RenderAttachment.h"
#include <vector>
#include <array>
namespace slag
{
    class SwapChain;
    class FrameBuffer
    {
    public:
        bool hasDepthBuffer();
        size_t attachmentCount();
        size_t colorAttachmentCount();
        FrameBuffer()=delete;
        FrameBuffer(FrameBuffer&)=delete;
        FrameBuffer& operator=(FrameBuffer&)=delete;
        ~FrameBuffer();
        //TODO: see if we absolutely can't directly create a framebuffer
        static std::shared_ptr<FrameBuffer> Create(uint32_t width, uint32_t height, GraphicsCard* card);
        static std::shared_ptr<FrameBuffer> Create(uint32_t width, uint32_t height, GraphicsCard* card, std::array<float,4>);
        static std::shared_ptr<FrameBuffer> Create(std::vector<std::shared_ptr<RenderAttachment>>& attachments, uint32_t width, uint32_t height, GraphicsCard* card);
        static std::shared_ptr<FrameBuffer> Create(std::vector<std::shared_ptr<RenderAttachment>>& attachments, uint32_t width, uint32_t height, GraphicsCard* card, std::vector<std::array<float,4>>& clearColors);
        std::shared_ptr<RenderAttachment> getAttachment(size_t index);
        uint32_t width();
        uint32_t height();
        void setClearColors(float* colors, size_t count);
        ///Call if the Attachment is in the same scope as slag::destroy, otherwise RAII will take care of it
        void destroy();
    private:
        FrameBuffer(std::vector<std::shared_ptr<RenderAttachment>>& attachments, uint32_t width, uint32_t height, GraphicsCard* card, std::vector<std::array<float,4>>& clearColors);
        std::vector<std::shared_ptr<RenderAttachment>> _attachments;
        bool _hasDepthBuffer = false;
        void* _vkFrameBuffer = nullptr;
        void* _vkRenderPass = nullptr;
        GraphicsCard* _card = nullptr;
        uint32_t _width = 0;
        uint32_t _height = 0;
        std::vector<std::array<float,4>> _clearColors;
        bool _hasCleanedUp = false;
        friend class Renderer;
        friend class SwapChain;
        friend class Shader;
    };
}

#endif //SLAGLIB_FRAMEBUFFER_H