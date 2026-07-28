# Slag
*Slag is under active development. The API may not be stable, and there are still some features missing, as well as a few known bugs. Use at your own risk!*
## Simplified Lightweight Accelerated Graphics
Graphics are hard. Getting a triangle to show up on screen in an API like vulkan is notoriously around 1000 lines of code, and most of those lines don't provide a good framework for building an actual application on top of. Additionally, as these APIs have evolved over the years, they've made radical changes to how they're expected to be used, often having multiple versions of functions, or expecting extensions to be enabled and passed around. These API changes do often result in fewer lines of code, but it makes picking what version of the call with what extensions significantly more complicated. SLAG is an attempt to simplify the process of using these APIs, as well as providing an abstraction that is usable for multiple different APIs at the flip of a single switch. Here's what a "Hello, World" application looks like in SLAG

```c++
#include <slag/Slag.h>
//#include <SDL/ GLFW/ Other Window Handling>
using namespace slag;

void main()
{
    if (Slag::initialize({.backend = BackendAPI::VULKAN}) != SlagInitializationResult::SUCCESS)
    {
        auto window = windowingLibraryCreateWindow();//Whatever is required to create a window in your windowing library
        auto graphicsCard = Slag::backend()->graphicsCard(0);//just get the first available graphics card
        
        PlatformData platformData{};
        platformData.plarform = Platform::WIN_32;//Windowing platform, in this case windows, but X11 and wayland are also available
        platformData.details.win32.hwnd = window.hwnd;//Because we're using win32, we pass the window HWND
        platformData.details.win32.hinstance = window.hinstance;//similarly, pass the window HINSTANCE
        
        SwapChainParameters swapChainParams{};//There's additional details here about how the swapchain works, but the defaults work fine
        
        auto swapchain = graphicsCard->newSwapchain(platformData, 500, 500, swapChainParams);//Create a swapchain with 500x500 pixel back buffers,
        
        float vertices[] = {-0.5f,-0.5f,0, 0,.5f,0, .5f,-0.5f,0,};//The vertices for a triangle
        float colors[] = {1,0,0, 0,1,0, 0,0,1};//The colors for the triangle vertices
        
        auto vertexBuffer = graphicsCard->newBuffer(sizeof(vertices),BufferCPUAccess::WRITE_ONLY);//create a buffer to hold vertex data
        auto colorBuffer = graphicsCard->newBuffer(sizeof(colors),BufferCPUAccess::WRITE_ONLY);//create a buffer to hold color data
        
        memcpy(vertexBuffer->data(),vertices,sizeof(vertices));//copy the vertex data into vertex buffer
        memcpy(colorBuffer->data(),colors,sizeof(colors));//copy the color data into the color buffer
        //describe the vertex data layout
        std::vector<VertexBinding> vertexBindings = 
        {
            //first binding contains the vertex attribute POSITION
            VertexBinding(0,sizeof(float)*3,InputRate::PER_VERTEX, std::vector<VertexAttribute>{VertexAttribute("POSITION", PixelFormat::R32G32B32_FLOAT,0)}),
            //second binding contains the vertex attribute COLOR
            VertexBinding(1,sizeof(float)*3,InputRate::PER_VERTEX, std::vector<VertexAttribute>{VertexAttribute("COLOR", R32G32B32_FLOAT,0)})
        };
        
        VertexDescription vertexDescripton(vertexBindings);//create a description of the vertex data layout from the bindings
        PipelineState pipelineState{};//Provides additional details about how the pipeline will execute, but the defaults work fine
        FramebufferDescription framebufferDescription{};//Describes the framebuffer that will be used for rendering
        framebufferDescription.colorFormats[0] = swapchain->parameters().imageFormat;//The format of the image we render to (the swapchain image here)
        
        char* vertexSource = readFile("path/to/vertex/shader.vert.spirv");//anything really to get vertex shader source code, read from file, embedded, etc.
        uint32_t vertexSourceSize = strlen(vertexSource);//anything to get the size of the source code,
        char* fragmentSource = readFile("path/to/fragment/shader.frag.spirv");//ditto for fragment shader source code
        uint32_t fragmentSourceSize = strlen(fragmentSource);//ditto for the size of the fragment shader source code
        
        //These objects are pretty simple wrappers for the details of the shader code, they don't own memory, just provide data about the code
        ShaderCode vertexCode{.language = ShaderLanguage::SPIRV,.code = vertexSource,.codeSize = vertexSourceSize};
        ShaderCode fragmentCode{.language = ShaderLanguage::SPIRV,.code = fragmentSource,.codeSize = fragmentSourceSize};
        
        auto shaderPipeline = graphicsCard->newShaderPipeline(vertexDescription,vertexCode,fragmentCode,pipelineState,framebufferDescription);
        //You are free to delete vertexSource and fragmentSource after this point,
        
        Semaphore* renderFinished = nullptr;//The semaphore that signals the render is finished. We'll assign it in the render loop each frame
        CommandBuffer* commandBuffer = graphicsCard->newCommandBuffer(QueueType::GRAPHICS);//create a new command buffer that can render graphics
        
        while (window.isOpen()) //whatever windowing library you're using
        {
            if (auto frame = swapchain->next())//get the next frame from the swapchain
            {
                if (renderFinished)
                {
                    delete renderFinished;//delete the semaphore if it exists, we'll assign it and use it again in the loop,
                }
                renderFinished = graphicsCard->newSemaphore();//create a new semaphore that indicates the render is finished
                auto backBuffer = frame->renderBuffer();//get the backbuffer we're rendering to
                
                commandBuffer->begin();//start recording commands to send to the GPU
                
                //Create a texture barrier that will make the swapchain image available to render to
                TextureBarrier toColorTarget
                {
                    .texture = backBuffer, //The texture we need to make sure is in the correct state
                    .baseLayer = 0, //The first array index of the texture we're making sure is in the correct state
                    .layerCount = 1, //The number of array indices we need to make sure is in the correct state
                    .baseMipLevel = 0, //The first mipmap level we need to make sure is in the correct state
                    .mipCount = 1, //The number of mipmap levels we need to make sure is in the correct state
                    .syncBefore = SyncStages::NONE, //Which stages of execution need to finish before we change the texture's state
                    .syncAfter = SyncStages::ALL_GRAPHICS, //Which stages of execution need to wait until after the texture's state has been changed
                    .flush = MemoryCaches::NONE, //Which graphics cache needs to be written back to main memory
                    .invalidate = MemoryCaches::NONE, //Which graphics caches need to acquire the changed memory
                    .layoutBefore = TextureLayout::UNKNOWN, //The layout of the texture before we change it's state (or UNKNOWN if we don't care)
                    .layoutAfter = TextureLayout::COLOR_TARGET //The layout of the texture after we change it's state
                };
                
                commandBuffer->insertBarriers(&toColorTarget,1);//insert the barrier into the command buffer
                
                Attachment colorAttachment{.texture = backBuffer, .autoClear = true, .clearValue = {.color = ClearColor{.2f,.2f,.8f,1.0f}}};//create the attachment we'll render to
                commandBuffer->beginRendering(&colorAttachment,1,nullptr,slag::Rectangle{0,0,backBuffer->width(),backBuffer->height()});//begin rendering
                
                Buffer* buffers[] = {vertexBuffer,colorBuffer};//Create an array of the buffers that contain vertex data
                uint64_t offsets[] = {0,0};//Offsets into the buffers to begin pulling data from, in this case start at the beginning of each buffer
                uint64_t strides[] = {sizeof(float)*3,sizeof(float)*3};//Strides between each vertex attribute
                commandBuffer->bindVertexBuffers(0,buffers,offsets,strides,2);//bind the vertex buffers as the data to use when drawing
                commandBuffer->bindShaderPipeline(shaderPipeline);//set the shader pipeline as the exeution to follow when drawing
                commandBuffer->draw(3,1,0,0);//draw 3 vertices,
                commandBuffer->endRendering();//stop rendering
                
                //Create another barrier that will make the swapchain image available to present to the screen
                TextureBarrier toPresent
                {
                    .texture = backBuffer,
                    .baseLayer = 0,
                    .layerCount = 1,
                    .baseMipLevel = 0,
                    .mipCount = 1,
                    .syncBefore = SyncStages::ALL_GRAPHICS, //Finish all graphics commands before we change the texture's state
                    .syncAfter = SyncStages::ALL, //Make all other commands wait until this texture's state has been changed (which is nothing, but a value must be provided)
                    .flush = MemoryCaches::COLOR_TARGET, //Write the color target cache back to main memory
                    .invalidate = MemoryCaches::NONE, //We're done with the texture, we don't need to invalidate any caches
                    .layoutBefore = TextureLayout::COLOR_TARGET, //The texture has been in the color target layout while we drew to it
                    .layoutAfter = TextureLayout::PRESENT //Change the texture layout to one that can be presented to the screen
                };
                commandBuffer->insertBarriers(&toPresent,1);//insert the barrier into the command buffer
                
                commandBuffer->end();//end recording commands to send to the GPU
                
                //Now we actually submit the recorded commands to the GPU
                
                SemaphoreValue signal{.semaphore=renderFinished, .value=1};//just an object that associates a semaphore with a future value
                
                SubmissionBatch batch
                {
                    .waitSemaphores = nullptr, //We don't need to wait for any semaphores to be at a value before we start the work in this batch, if we did, they'd go here
                    .waitSemaphoreCount = 0,
                    .commandBuffers = &commandBuffer, //An array of command buffers to submit to the GPU, in this case just the one we just recorded
                    .commandBufferCount = 1,
                    .signalSemaphores = &signal, //An array of semaphore to set values to to signify the work has been completed, in this case just the semaphore we just created
                    .signalSemaphoreCount = 1
                };
                
                _graphicsCard->submit(&batch,1);//submit the batch to the GPU
                renderFinished->waitForValue(1);//wait for the semaphore to be set to 1, which indicates we're finished rendering
                swapchain->present();//Queue the frame to be presented to the screen, it'll show automatically based on swapchain parameters
            }
        }
        
        renderFinished->waitForValue(1);//wait gpu to finish with it's current resources before we exit
        delete renderFinished;
        delete commandBuffer;
        delete shaderPipeline;
        delete vertexBuffer;
        delete colorBuffer;
        delete swapchain;
        windowingLibraryDestroyWindow(window); //whatever is required to destroy a window in whatever library you choose
        Slag::cleanup();
           
    }
}
```
Not nothing, but ~150 lines of commented (minus windowing and loading shaders) code is a lot more manageable than the touted 1000 lines for Vulkan for a simple "Hello, World", with the added bonus that switching between APIs is as simple as changing a single line of code.

Simply change 
```c++
Slag::initialize({.backend = BackendAPI::VULKAN})
```
to
```c++
Slag::initialize({.backend = BackendAPI::DX12})
```
and you're done!

## Examples
The "Hello, World" example given here isn't comprehensive and is certainly a little naive in its implementation. For more complete examples, check out the [example projects](examples) to see how to use additional features of the library.

## Documentation and Walkthroughs
Coming soon!

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
#include <slag/Slag.h>
```