#ifndef CRUCIBLEEDITOR_RENDERPASS_H
#define CRUCIBLEEDITOR_RENDERPASS_H

#include <string>
#include <unordered_map>
namespace slag
{
    struct AttachmentInfo
    {
        float sizeX = 1.0f;
        float sizeY = 1.0f;
        Pixels::PixelFormat format = Pixels::PixelFormat::UNDEFINED;
        unsigned int samples = 1;
        unsigned int levels = 1;
        unsigned int layers = 1;
        bool persistent = true;
    };
    struct BufferInfo
    {
        size_t size = 0;
        Buffer::Usage usage;
        bool persistent = true;
    };

    class RenderPass
    {
    public:
        enum PassType
        {
            GRAPHICS,
            COMPUTE,
            TRANSFER
        };
        RenderPass(std::string name, PassType passType);
        void addColorAttachment(std::string name, AttachmentInfo& info);

    private:
        std::string _name;
        PassType _passType;
        std::unordered_map<std::string ,Attachment> _inputAttachments;
        std::unordered_map<std::string ,BufferInfo> _inputBufferInfo;

        RenderPass merge(RenderPass& with);

    };

} // slag

#endif //CRUCIBLEEDITOR_RENDERPASS_H
