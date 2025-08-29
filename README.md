# Slag
*Slag is under active development. Many features, even core ones, are not implemented, and the API is not yet stable. Use at your own risk!*
## Simplified Lightweight Accelerated Graphics
Graphics are hard. Slag is designed to make it a little more manageable by thinly abstracting over the low level APIs, and providing sane defaults for them. The idea is to provide a level of abstraction somewhere between that of earlier APIs like openGL, and more modern ones like Vulkan. Getting a triangle on screen in modern APIs like Vulkan notoriously take ~1000 lines of code, all without providing good abstractions to then make a real application on top of it. Here's what it looks like in Slag

```c++
#include <slag/SlagLib.h>
//#include <SDL/ GLFW/ Other Window Handling>

int main()
{
    if(slag::initialize(slag::SlagInitInfo{.graphicsBackend = slag::GraphicsBackend::VULKAN_GRAPHICS_BACKEND}) == slag::SlagInitializationResult::SLAG_INITIALIZATION_SUCCESS)
    {
        auto window = createWindow(); //whatever is required to create a window in whatever library you choose

        //give data for the swapchain's backing window, in this case a WIN32 window. This will be slightly different depending on platform
        slag::PlatformData platformData{};
        platformData.platform = slag::Platform::WIN_32;
        platformData.details.win32.hwnd = window.hwnd;//provide window handle
        platformData.details.win32.hinstance = window.hinstance;//provide application hinstance

        auto swapChain = slag::SwapChain::newSwapChain(platformData,window.width(),window.height(),slag::SwapChain::PresentMode::IMMEDIATE,1,slag::Pixels::Format::B8G8R8A8_UNORM,slag::SwapChain::AlphaCompositing::IGNORE_ALPHA);
        auto queue = slag::slagGraphicsCard()->graphicsQueue();
        auto commandBuffer = slag::CommandBuffer::newCommandBuffer(slag::GPUQueue::QueueType::GRAPHICS);
        auto finished = slag::Semaphore::newSemaphore(1);
        auto descriptorPool = slag::DescriptorPool::newDescriptorPool();

        slag::ShaderCode vertexShader(slag::ShaderStageFlags::VERTEX,slag::ShaderCode::CodeLanguage::SPIRV,"vertex.spv");
        slag::ShaderCode fragmentShader(slag::ShaderStageFlags::FRAGMENT,slag::ShaderCode::CodeLanguage::SPIRV,"fragment.spv");
        slag::ShaderCode* code[]{&vertexShader,&fragmentShader};
        slag::ShaderProperties properties{};
        slag::FrameBufferDescription frameBufferDescription{};
        frameBufferDescription.colorTargets[0] = slag::Pixels::Format::B8G8R8A8_UNORM;
        frameBufferDescription.depthTarget = slag::Pixels::Format::D32_FLOAT;
        slag::VertexDescription vertexDescription(1);
        vertexDescription.add(slag::VertexAttribute{slag::GraphicsType::VECTOR3,0},0);

        slag::ShaderPipeline* shader = slag::ShaderPipeline::newShaderPipeline(code,2,properties,vertexDescription,frameBufferDescription);

        float triangleVertices[9]={ -1.f, -1.f, 0.0f,  0.f,1.f, 0.0f,  1.f, -1.f, 0.0f};//vertices of triangle
        size_t vertsBufferSizeInBytes = sizeof(float)*9;//the size in bytes triangleVertices takes
        auto vertices = slag::Buffer::newBuffer(triangleVertices,vertsBufferSizeInBytes,slag::Buffer::Accessibility::GPU,slag::Buffer::UsageFlags::VERTEX_BUFFER);

        while(window.shouldStayOpen())
        {
            if(auto frame = swapChain.next())
            {
                finished->waitForValue(1);//make sure the command buffer has finished rendering the last frame
                //recreate a new finished semaphore
                delete(finished);
                finished = slag::Semaphore::newSemaphore(0);

                commandBuffer->begin();
                //provide frame data it needs to draw
                commandBuffer->bindDescriptorPool(descriptorPool);
                commandBuffer->setViewPort(0,0,frame->backBuffer()->width(),frame->backBuffer()->height(),1,0);
                commandBuffer->setScissors({{0,0},{frame->backBuffer()->width(),frame->backBuffer()->height()}});

                //start drawing the frame's image buffer
                slag::Attachment attachment{.texture = frame->backBuffer(),.autoClear = true, .clearValue = slag::ClearColor{.floats = {1,1,1,1} }};
                commandBuffer->beginRendering(&attachment,1, nullptr,slag::Rectangle{{0,0},{frame->backBuffer()->width(),frame->backBuffer()->height()}});
                //use our shader
                commandBuffer->bindGraphicsShaderPipeline(shader);
                //bind vertexBuffer
                uint64_t offsets = 0; //how far into vertex buffer to start
                uint64_t vertexStride = sizeof(float) *3;//size of each vertex, in this example representing x, y, and z position as floats
                commandBuffer->bindVertexBuffers(0,&vertices,&offsets,&vertexStride,1);
                //draw
                commandBuffer->draw(3, 1, 0, 0);
                commandBuffer->endRendering();

                commandBuffer->end();

                slag::SemaphoreValue signal
                {
                    .semaphore = finished,
                    .value = 1
                };

                slag::QueueSubmissionBatch submissionBatch;
                submissionBatch.commandBuffers = &commandBuffer;
                submissionBatch.commandBufferCount = 1;
                 submissionBatch.signalSemaphores = &signal;
                submissionBatch.signalSemaphoreCount = 1;
                queue->submit(&submissionBatch,1,frame);
            }
        }

        finished->waitForValue(1);//make sure the command buffer is done before deleting it
        //cleanup
        delete finished;
        delete commandBuffer;
        delete shader;
        delete vertices;
        delete descriptorPool;
        delete swapChain;

        destroyWindow(window); //whatever is required to destroy a window in whatever library you choose
    }
    slag::cleanup();
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
        GIT_TAG #RELEASE_TAG_GOES_HERE
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