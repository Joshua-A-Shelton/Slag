#ifndef SLAG_ICOMMANDBUFFER_H
#define SLAG_ICOMMANDBUFFER_H
#include "Attachment.h"
#include "GPUQueue.h"
#include "GPUBarriers.h"
#include "Clear.h"
#include "Dimensions.h"
#include "Sampler.h"
#include "Texture.h"

namespace slag
{
    ///Provides the set of universal command buffer commands
    class ICommandBuffer
    {
    public:
        virtual ~ICommandBuffer()=default;
        ///The type of commands this command buffer can execute
        virtual GPUQueue::QueueType commandType()=0;
        ///Start recording commands
        virtual void begin()=0;
        ///End recording commands
        virtual void end()=0;

#ifdef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Insert GPU execution barriers that guarantee certain operations finish before continuing execution of the graphics queue
         * @param textureBarriers Barriers that ensure an operation on a texture finishes before the next operation on it can continue
         * @param textureBarrierCount Number of texture barriers
         * @param bufferBarriers Barriers that ensure an operation on a buffer finishes before the next operation on it can continue
         * @param bufferBarrierCount Number of buffer barriers
         * @param memoryBarriers Barriers that ensure operations in the queue finishes before additional operations can continue
         * @param memoryBarrierCount Number of memory barriers
         */
        virtual void insertBarriers(TextureBarrierDiscreet* textureBarriers, size_t textureBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)=0;
        ///Insert GPU execution barrier that ensures an operation on a texture finishes before the next operation on it can continue
        virtual void insertBarrier(const TextureBarrierDiscreet& barrier)=0;
#else
        /**
         * Insert GPU execution barriers that guarantee certain operations finish before continuing execution of the graphics queue
         * @param textureBarriers Barriers that ensure an operation on a texture finishes before the next operation on it can continue
         * @param textureBarrierCount Number of texture barriers
         * @param bufferBarriers Barriers that ensure an operation on a buffer finishes before the next operation on it can continue
         * @param bufferBarrierCount Number of buffer barriers
         * @param memoryBarriers Barriers that ensure operations in the queue finishes before additional operations can continue
         * @param memoryBarrierCount Number of memory barriers
         */
        virtual void insertBarriers(TextureBarrier* textureBarriers, size_t textureBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GlobalBarrier* memoryBarriers, size_t memoryBarrierCount)=0;
        ///Insert GPU execution barrier that ensures an operation on a texture finishes before the next operation on it can continue
        virtual void insertBarrier(const TextureBarrier& barrier)=0;
#endif
        ///Insert GPU execution barrier that ensures an operation on a buffer finishes before the next operation on it can continue
        virtual void insertBarrier(const BufferBarrier& barrier)=0;
        ///Insert GPU execution barrier that ensures an operations in the queue finishes before additional operations can continue
        virtual void insertBarrier(const GlobalBarrier& barrier)=0;

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Clear a color texture
         * @param texture The texture to clear
         * @param color The color to clear it to
         */
        virtual void clearTexture(Texture* texture, ClearColor color)=0;

        /**
         * clear a depth/stencil texture
         * @param texture The texture to clear
         * @param depthStencil The value to clear it to
         */
        virtual void clearTexture(Texture* texture, ClearDepthStencilValue depthStencil)=0;
#else
            /**
         * Clear a color texture
         * @param texture The texture to clear
         * @param textureLayout the layout of the texture being cleared
         * @param color The color to clear it to
         */
            virtual void clearTexture(Texture* texture,TextureLayouts::Layout textureLayout, ClearColor color)=0;

            /**
             * clear a depth/stencil texture
             * @param texture The texture to clear
             * @param textureLayout the layout of the texture being cleared
             * @param depthStencil The value to clear it to
             */
            virtual void clearTexture(Texture* texture,TextureLayouts::Layout textureLayout ClearDepthStencilValue depthStencil)=0;
#endif

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Propagate an image in a mip chain to a different level in the chain
         * @param texture Texture to update
         * @param sourceMip Mip level that acts as a source that is applied to the destination
         * @param destinationMip Mip level to be applied to
         * @param layer Texture layer to update
         */
        virtual void updateMip(Texture* texture, uint32_t sourceMip, uint32_t destinationMip, uint32_t layer)=0;
#else
        /**
        * Propagate an image in a mip chain to a different level in the chain
        * @param texture
        * @param sourceMip
        * @param sourceLayout
        * @param destinationMip
        * @param destinationLayout
        * @param layer
        */
        virtual void updateMip(Texture* texture, uint32_t sourceMip,TextureLayouts::Layout sourceLayout, uint32_t destinationMip, TextureLayouts::Layout destinationLayout, uint32_t layer)=0;
#endif

        /**
         * Copy contents of one buffer to another
         * @param source Buffer to copy from
         * @param sourceOffset Offset in the buffer to start copying from
         * @param length Length of data in bytes to copy to the new buffer
         * @param destination Buffer to copy to
         * @param destinationOffset Offset in the buffer to start copying to
         */
        virtual void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, uint64_t length, Buffer* destination, uint64_t destinationOffset)=0;
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Copy texel data of a texture to a buffer
         * @param source Texture to copy from
         * @param subresources Parts of the texture to copy
         * @param subresourceCount Number of items in subresources array
         * @param destination Buffer to copy to
         */
        virtual void copyTextureToBuffer(Texture* source, TextureSubresource* subresources, size_t subresourceCount, Buffer* destination)=0;
#else
         /**
         * Copy texel data of a texture to a buffer
         * @param source Texture to copy from
         * @param textureLayout layout of the source texture
         * @param subresources Parts of the texture to copy
         * @param subresourceCount Number of items in subresources array
         * @param destination Buffer to copy to
         */
         virtual void copyTextureToBuffer(Texture* source, TextureLayouts::Layout textureLayout, TextureSubresource* subresources, size_t subresourceCount, Buffer* destination)=0;
#endif

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Draw the contents of one texture into another
         * @param source source The texture to copy from
         * @param sourceLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to copy from
         * @param sourceMip sourceMip The mipmap level of the source texture to copy from
         * @param sourceArea The area of the texture to copy (sized to chosen mip level)
         * @param destination The texture to draw to
         * @param destinationLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to draw to
         * @param destinationMip The mipmap level of the source texture to draw to
         * @param destinationArea The area of the texture to draw to (sized to chosen mip level)
         * @param filter The filter that handles how drawing resizes is handled
         */
        virtual void blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea, Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Rectangle destinationArea, Sampler::Filter filter)=0;
#else
        /**
        * Draw the contents of one texture into another
        * @param source The texture to copy from
        * @param sourceLayout The layout of the source texture at the time of execution (TRANSFER_SOURCE/GENERAL)
        * @param sourceLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to copy from
        * @param sourceMip The mipmap level of the source texture to copy from
        * @param sourceArea The area of the texture to copy (sized to chosen mip level)
        * @param destination The texture to draw to
        * @param destinationLayout The layout of the destination texture at the time of execution (TRANSFER_DESTINATION/GENERAL)
        * @param destinationLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to draw to
        * @param destinationMip The mipmap level of the source texture to draw to
        * @param destinationArea The area of the texture to draw to (sized to chosen mip level)
        * @param filter The filter that handles how drawing resizes is handled
        */
        virtual void blit(Texture* source,TextureLayouts::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip,Rectangle sourceArea, Texture* destination, TextureLayouts::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea,Sampler::Filter filter)=0;
#endif

            /**
            * Sets the drawing area relative to the window, final image will be drawn scaled to the drawing area
            * @param x Pixels away from the left of window to draw
            * @param y Pixels away from the bottom of the window to draw
            * @param width width in pixels of drawing area
            * @param height height in pixels of drawing area
            * @param minDepth minimum depth of drawn pixels
            * @param maxDepth maximum depth of drawn pixels
            */
            virtual void setViewPort(float x, float y, float width, float height, float minDepth,float maxDepth)=0;

            /**
             * Sets the drawing area relative to the window, final image will be clipped to the drawing area
             * @param rectangle
             */
            virtual void setScissors(Rectangle rectangle)=0;
            /**
             *
             * @param r Red constant
             * @param g Green constant
             * @param b Blue constant
             * @param a Alpha constant
             */
            virtual void setBlendConstants(float r, float g, float b, float a)=0;
            /**
             * Set the value to compare against in stencil test
             * @param reference
             */
            virtual void setStencilReference(uint32_t reference)=0;
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Start a renderpass by providing render targets
         * @param colorAttachments Color attachments that shaders will render to in this pass
         * @param colorAttachmentCount Number of Color Attachments
         * @param depthAttachment Depth attachment that shaders will use as depth target in render pass (or nullptr if no depth attachment is needed)
         * @param bounds Area that is affected in render pass
         */
        virtual void beginRendering(Attachment* colorAttachments, size_t colorAttachmentCount,Attachment* depthAttachment,Rectangle bounds)=0;
#else
            /**
             * Start a renderpass by providing render targets
             * @param colorAttachments Color attachments that shaders will render to in this pass
             * @param colorAttachmentCount Number of Color Attachments
             * @param depthAttachment Depth attachment that shaders will use as depth target in render pass (or nullptr if no depth attachment is needed)
             * @param bounds Area that is affected in render pass
             */
            virtual void beginRendering(AttachmentDiscreet* colorAttachments, size_t colorAttachmentCount,AttachmentDiscreet* depthAttachment,Rectangle bounds)=0;
#endif

            virtual void endRendering()=0;
    };
} // slag

#endif //SLAG_ICOMMANDBUFFER_H
