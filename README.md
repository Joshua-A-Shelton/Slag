# Slag
*Slag is under active development. Many features, even core ones, are not implemented, and the API is not yet stable. Use at your own risk!*
## Simplified Lightweight Accelerated Graphics
Graphics are hard. Slag is designed to make it a little more manageable by thinly abstracting over the low level APIs, and providing sane defaults for them. The idea is to provide a level of abstraction somewhere between that of earlier APIs like openGL, and more modern ones like Vulkan. Getting a triangle on screen in modern APIs like Vulkan notoriously take ~1000 lines of code, all without providing good abstractions to then make a real application on top of it. Here's what it looks like in Slag

```c++
#include <slag/SlagLib.h>
//#include <SDL/ GLFW/ Other Window Handling>
int main()
{
    slag::SlagInitDetails details{.backend=slag::BackEnd::Vulkan};
    if(slag::SlagLib::initialize(details))
    {
        auto window = createWindow(); //whatever is required to create a window in whatever library you choose
        
        slag::PlatformData platformData{};
        platformData.nativeWindowHandle = window.handle; //HWND in Windows, X11 window in *nix
        platformData.nativeDisplayType = window.display; //hinstance in Windows, X11 display in *nix
        
        auto graphicsQueue = slag::SlagLib::graphicsCard()->graphicsQueue();
        auto swapchain = slag::Swapchain::newSwapchain(pd, 500, 500, 2, Swapchain::PresentMode::MAILBOX, Pixels::Format::B8G8R8A8_UNORM,nullptr);
        auto commandBuffer = slag::CommandBuffer::newCommandBuffer(slag::GpuQueue::GRAPHICS);
        auto descriptorPool = slag::DescriptorPool::newDescriptorPool();
        
        ShaderModule modules[2] = {ShaderModule(ShaderStageFlags::VERTEX,"VertexShader.spv"),ShaderModule(ShaderStageFlags::FRAGMENT,"FragmentShader.spv")};
        ShaderProperties props;
        
        FrameBufferDescription description;
        description.addColorTarget(Pixels::B8G8R8A8_UNORM);
        auto shader = Shader::newShader(modules,2, nullptr,0,props, nullptr,description);
        
        float triangleVertices[9]={ -1.f, -1.f, 0.0f,  0.f,1.f, 0.0f,  1.f, -1.f, 0.0f};//vertices of triangle
        size_t vertsBufferSizeInBytes = sizeof(float)*9;//the size in bytes triangleVertices takes
        auto vertices = Buffer::newBuffer(triangleVertices,vertsBufferSizeInBytes,Buffer::GPU,Buffer::VERTEX_BUFFER)
        
        while(window.shouldStayOpen())
        {
            if(auto frame = swapchain.next())
            {
                commandBuffer->waitUntilFinished();//make sure the command buffer has finished rendering the last frame
                
                commandBuffer->begin();
                //provide frame data it needs to draw
                commandBuffer->bindDescriptorPool(descriptorPool);
                commandBuffer->setViewPort(0,0,frame->backBuffer()->width(),frame->backBuffer()->height(),1,0);
                commandBuffer->setScissors({{0,0},{frame->backBuffer()->width(),frame->backBuffer()->height()}});
                //make the frame's image buffer targetable by graphics shaders
                commandBuffer->insertBarrier({
                    .texture = frame->backBuffer(),
                    .oldLayout = slag::Texture::UNDEFINED,
                    .newLayout = slag::Texture::RENDER_TARGET,
                    .accessBefore = slag::BarrierAccessFlags::NONE,
                    .accessAfter = slag::BarrierAccessFlags::SHADER_WRITE,
                    .syncBefore = slag::PipelineStageFlags::NONE,
                    .syncAfter = slag::PipelineStageFlags::ALL_GRAPHICS
                });
                //start drawing the frame's image buffer
                slag::Attachment attachment(frame->backBuffer(),slag::Texture::RENDER_TARGET, true,{.floats={1,0,0,1}});
                commandBuffer->beginRendering(&attachment,1, nullptr,slag::Rectangle{{0,0},{frame->backBuffer()->width(),frame->backBuffer()->height()}});
                //use our shader
                commandBuffer->bindGraphicsShader(shader);
                //bind vertexBuffer
                size_t offsets = 0; //how far into vertex buffer to start
                size_t vertexStride = sizeof(float) *3;//size of each vertex, in this example representing x, y, and z position as floats
                commandBuffer->bindVertexBuffers(0,&vertices,&offsets,&size,&stride,1);
                //draw
                commandBuffer->draw(3, 1, 0, 0);
                commandBuffer->endRendering();
                 //make the frame's image buffer suitable for showing to the screen
                 commandBuffer->insertBarrier({
                    .texture = frame->backBuffer(),
                    .oldLayout = slag::Texture::RENDER_TARGET,
                    .newLayout = slag::Texture::PRESENT,
                    .accessBefore = slag::BarrierAccessFlags::SHADER_WRITE,
                    .accessAfter = slag::BarrierAccessFlags::COLOR_ATTACHMENT_READ,
                    .syncBefore = slag::PipelineStageFlags::ALL_GRAPHICS,
                    .syncAfter = slag::PipelineStageFlags::ALL_COMMANDS
                });
                commandBuffer->end();
                
                graphicsQueue->submit(&commandBuffer,1, nullptr,0, nullptr,0,frame);
            }
        }
        
        commandBuffer->waitUntilFinished();//make sure the command buffer is done before deleting it
        //cleanup
        delete commandBuffer;
        delete shader;
        delete vertices;
        delete descriptorPool;
        delete swapchain;
        
        destroyWindow(window); //whatever is required to destroy a window in whatever library you choose
    }
    slag::SlagLib::cleanup();
}
```
Not nothing, but less than 100 lines of commented (minus windowing) code seems a lot more manageable than the crazy setup for Vulkan!

## Including in your project

Slag is built with cmake. The easiest way to include Slag in your project is as follows (requires cmake >= 3.24)

```cmake
#get slag library
include(FetchContent)
fetchcontent_declare(
        Slag
        GIT_REPOSITORY https://github.com/Joshua-A-Shelton/Slag.git
        GIT_TAG origin/master
        GIT_SHALLOW TRUE
        OVERRIDE_FIND_PACKAGE
)
fetchcontent_makeavailable(Slag)

find_package(Slag REQUIRED)

#link to slag library
target_link_libraries(YourProject Slag)
```
Start using Slag by including the library header
```c++
#include <slag/SlagLib.h>
```