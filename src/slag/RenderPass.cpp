#include "RenderPass.h"
#include <stdexcept>
namespace slag
{
    RenderPass::RenderPass(std::string name, slag::RenderPass::PassType passType)
    {
        throw std::runtime_error("not implemented");
    }

    void RenderPass::addColorAttachment(std::string name, slag::AttachmentInfo &info)
    {
        throw std::runtime_error("not implemented");
    }

    RenderPass RenderPass::merge(RenderPass &with)
    {
        throw std::runtime_error("not implemented");
        //return RenderPass(std::basic_string(), RenderPass::GRAPHICS);
    }
} // slag