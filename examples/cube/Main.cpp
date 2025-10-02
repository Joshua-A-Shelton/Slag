#define SDL_MAIN_HANDLED
#include <iostream>
#include <SDL2/SDL.h>
#include <slag/Slag.h>
#include <glm/glm.hpp>

#include "SDL_syswm.h"
#include "glm/ext/matrix_clip_space.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "glm/ext/matrix_transform.hpp"

class DefaultResources: public slag::FrameResources
{
public:
    slag::CommandBuffer* commandBuffer=nullptr;
    slag::Semaphore* commandsFinished=nullptr;
    slag::DescriptorPool* descriptorPool=nullptr;
    slag::Buffer* globalsBuffer = nullptr;
    slag::Buffer* instanceBuffer = nullptr;

    DefaultResources():slag::FrameResources()
    {
        commandBuffer = slag::CommandBuffer::newCommandBuffer(slag::GPUQueue::QueueType::GRAPHICS);
        commandsFinished = slag::Semaphore::newSemaphore();
        descriptorPool = slag::DescriptorPool::newDescriptorPool();
        globalsBuffer = slag::Buffer::newBuffer(sizeof(glm::mat4)*3,slag::Buffer::Accessibility::CPU_AND_GPU,slag::Buffer::UsageFlags::UNIFORM_BUFFER);
        instanceBuffer = slag::Buffer::newBuffer(sizeof(glm::mat4),slag::Buffer::Accessibility::CPU_AND_GPU,slag::Buffer::UsageFlags::UNIFORM_BUFFER);
    }
    ~DefaultResources()override
    {
        delete commandBuffer;
        delete commandsFinished;
        delete descriptorPool;
        delete globalsBuffer;
        delete instanceBuffer;
    }
};


slag::FrameResources* createResource(uint8_t frameIndex, slag::SwapChain* swapChain)
{
    return new DefaultResources();
}
slag::Texture* depthTexture = nullptr;
void swapChainRebuilt(slag::SwapChain* swapChain)
{
    delete depthTexture;
    depthTexture = slag::Texture::newTexture(slag::Pixels::Format::D32_FLOAT,slag::Texture::Type::TEXTURE_2D,slag::Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,swapChain->backBufferWidth(),swapChain->backBufferHeight(),1,1,1);
}

void backEndMessage(const std::string& message, slag::SlagDebugLevel level, int32_t messageID)
{
    std::cout << message << std::endl;
}

int main()
{
    slag::initialize(slag::SlagInitInfo{.graphicsBackend = slag::GraphicsBackend::VULKAN_GRAPHICS_BACKEND, .slagDebugHandler = backEndMessage});

    std::vector<float> cverts =
    {
        -0.5f, 0.5f, -0.5f,  // A 0
        0.5f, 0.5f, -0.5f,   // B 1
        0.5f,  -0.5f, -0.5f,   // C 2
        -0.5f,  -0.5f, -0.5f,  // D 3
        -0.5f, 0.5f,  0.5f,  // E 4
        0.5f, 0.5f,  0.5f,   // F 5
        0.5f,  -0.5f,  0.5f,   // G 6
        -0.5f,  -0.5f,  0.5f,   // H 7

        -0.5f,  -0.5f, -0.5f,  // D 8
        -0.5f, 0.5f, -0.5f,  // A 9
        -0.5f, 0.5f,  0.5f,  // E 10
        -0.5f,  -0.5f,  0.5f,   // H 11
        0.5f, 0.5f, -0.5f,   // B 12
        0.5f,  -0.5f, -0.5f,   // C 13
        0.5f,  -0.5f,  0.5f,   // G 14
        0.5f, 0.5f,  0.5f,   // F 15

        -0.5f, 0.5f, -0.5f,   // A 16
        0.5f, 0.5f, -0.5f,    // B 17
        0.5f, 0.5f,  0.5f,    // F 18
        -0.5f, 0.5f,  0.5f,   // E 19
        0.5f,  -0.5f, -0.5f,   // C 20
        -0.5f,  -0.5f, -0.5f, // D 21
        -0.5f,  -0.5f,  0.5f,  // H 22
        0.5f,  -0.5f,  0.5f,   // G 23
    };
    std::vector<float> cuvs =
    {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
    };
    std::vector<uint16_t> cindexes =
    {
        0, 3, 2,
         2, 1, 0,
         4, 5, 6,
         6, 7 ,4,
         // left and right
         11, 8, 9,
         9, 10, 11,
         12, 13, 14,
         14, 15, 12,
         // bottom and top
         16, 17, 18,
         18, 19, 16,
         20, 21, 22,
         22, 23, 20
    };

    slag::Buffer* cubeVerts = slag::Buffer::newBuffer(cverts.data(),cverts.size()*sizeof(float),slag::Buffer::Accessibility::GPU,slag::Buffer::UsageFlags::VERTEX_BUFFER);
    slag::Buffer* cubeUVs = slag::Buffer::newBuffer(cuvs.data(),cuvs.size()*sizeof(float),slag::Buffer::Accessibility::GPU,slag::Buffer::UsageFlags::VERTEX_BUFFER);
    slag::Buffer* cubeIndices = slag::Buffer::newBuffer(cindexes.data(),cindexes.size()*sizeof(uint16_t),slag::Buffer::Accessibility::GPU,slag::Buffer::UsageFlags::INDEX_BUFFER);



    slag::VertexDescription vertexPosUVDescription(2);
    vertexPosUVDescription.add(slag::GraphicsType::VECTOR3,0,0).add(slag::GraphicsType::VECTOR2,0,1);

    slag::ShaderCode vertexShader(slag::ShaderStageFlags::VERTEX,slag::ShaderCode::CodeLanguage::SPIRV,"resources/shaders/TexturedDepth.vertex.spv");
    slag::ShaderCode fragmentShader(slag::ShaderStageFlags::FRAGMENT,slag::ShaderCode::CodeLanguage::SPIRV,"resources/shaders/TexturedDepth.fragment.spv");
    slag::ShaderCode* code[]{&vertexShader,&fragmentShader};
    slag::ShaderProperties properties{};
    slag::FrameBufferDescription frameBufferDescription{};
    frameBufferDescription.colorTargets[0] = slag::Pixels::Format::B8G8R8A8_UNORM;
    frameBufferDescription.depthTarget = slag::Pixels::Format::D32_FLOAT;

    slag::ShaderPipeline* texturedDepthPipeline = slag::ShaderPipeline::newShaderPipeline(code,2,properties,vertexPosUVDescription,frameBufferDescription);
    auto GlobalsIndex = texturedDepthPipeline->descriptorGroup(0)->indexOf("Globals");
    auto InstanceIndex = texturedDepthPipeline->descriptorGroup(1)->indexOf("Instance");
    auto InstanceTextureIndex = texturedDepthPipeline->descriptorGroup(1)->indexOf("Instance.sampledTexture");
    auto InstanceSamplerIndex = texturedDepthPipeline->descriptorGroup(1)->indexOf("Instance.sampler");
    int width, height, channels;
    auto pixels = stbi_load("resources/textures/gradient.jpg", &width, &height,&channels,4);
    slag::TextureBufferMapping mapping
    {
        .bufferOffset = 0,
        .textureSubresource =
     {
            .aspectFlags = slag::Pixels::AspectFlags::COLOR,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .textureOffset = {0,0,0},
        .textureExtent = {static_cast<uint32_t>(width),static_cast<uint32_t>(height),1}
    };
    auto texture = slag::Texture::newTexture(slag::Pixels::Format::R8G8B8A8_UNORM,slag::Texture::Type::TEXTURE_2D,slag::Texture::UsageFlags::SAMPLED_IMAGE,width,height,1,1,1, slag::Texture::SampleCount::ONE,pixels,width*height*4,&mapping,1);
    stbi_image_free(pixels);
    slag::Sampler* defaultSampler = slag::Sampler::newSampler(slag::SamplerParameters{});


    int WINDOW_WIDTH = 300;
    int WINDOW_HEIGHT = 300;

    auto window = SDL_CreateWindow("Slag Cube",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,WINDOW_WIDTH,WINDOW_HEIGHT,SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    slag::PlatformData pd{};

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
#ifdef _WIN32
    pd.platform = slag::Platform::WIN_32;
    pd.details.win32.hwnd = wmInfo.info.win.window;
    pd.details.win32.hinstance = wmInfo.info.win.hinstance;
#elif __linux
    pd.platform = slag::Platform::X11;
    pd.details.x11.window = wmInfo.info.x11.window;
    pd.details.x11.display = wmInfo.info.x11.display;
#endif

    auto swapChain = slag::SwapChain::newSwapChain(pd,WINDOW_WIDTH,WINDOW_HEIGHT,slag::SwapChain::PresentMode::QUEUE,2,slag::Pixels::Format::B8G8R8A8_UNORM,slag::SwapChain::AlphaCompositing::IGNORE_ALPHA,createResource,swapChainRebuilt);
    auto queue = slag::slagGraphicsCard()->graphicsQueue();

    slag::Buffer* vertexBuffers[]={cubeVerts,cubeUVs};
    uint64_t vertexOffsets[]={0,0};
    uint64_t vertexStrides[]{sizeof(glm::vec3),sizeof(glm::vec2)};

    glm::mat4 instanceMatrix(1);

    bool keepOpen = true;
    uint32_t last_tick_time = 0;
    float delta = 0;
    while(keepOpen)
    {

        uint32_t tick_time = SDL_GetTicks();
        delta = ((float)tick_time - (float)last_tick_time)/1000;
        last_tick_time = tick_time;

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_WINDOWEVENT:
                    if(event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        keepOpen = false;
                    }
                    else if(event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        int w,h;
                        SDL_GetWindowSize(window,&w,&h);
                        swapChain->backBufferSize(w,h);
                    }
                    break;
                default:
                    break;
            }
        }

        if (auto frame = swapChain->next())
        {
            auto resources = frame->frameResources<DefaultResources>();
            auto commandBuffer = resources->commandBuffer;
            auto descriptorPool = resources->descriptorPool;
            auto globalsBuffer = resources->globalsBuffer;
            auto instanceBuffer = resources->instanceBuffer;
            auto backBuffer = frame->backBuffer();

            descriptorPool->reset();

            commandBuffer->begin();
            commandBuffer->bindDescriptorPool(descriptorPool);
            commandBuffer->setViewPort(0,0,backBuffer->width(),backBuffer->height(),0,1);
            commandBuffer->setScissors(slag::Rectangle{{0,0},{backBuffer->width(),backBuffer->height()}});

            slag::Attachment colorAttachment{.texture = backBuffer,.autoClear = true, .clearValue = slag::ClearColor{.floats = {0,.2,1,1} }};
            slag::Attachment depthAttachment{.texture = depthTexture,.autoClear = true };
            depthAttachment.clearValue.depthStencil = {1,0};

            commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{{0,0},{backBuffer->width(),backBuffer->height()}});
            commandBuffer->bindGraphicsShaderPipeline(texturedDepthPipeline);

            glm::mat4 projection = glm::perspective(95.0f,(float)backBuffer->width()/(float)backBuffer->height(),.01f,100.0f);
            glm::mat4 view(1.0f);
            view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
            view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
            view = glm::inverse(view);
            glm::mat4 projectionView = projection * view;
            auto globalHandle = globalsBuffer->as<glm::mat4>();
            globalHandle[0] = projection;
            globalHandle[1] = view;
            globalHandle[2] = projectionView;

            auto instanceHandle = instanceBuffer->as<glm::mat4>();
            instanceHandle[0] = instanceMatrix;
            instanceMatrix = glm::rotate(instanceMatrix,glm::radians(45.0f * delta),glm::vec3(0.0f,1.0f,0.0f));

            auto globals = descriptorPool->makeBundle(texturedDepthPipeline->descriptorGroup(0));
            globals.setUniformBuffer(GlobalsIndex,0,globalsBuffer,0,globalsBuffer->size());
            auto instance = descriptorPool->makeBundle(texturedDepthPipeline->descriptorGroup(1));
            instance.setUniformBuffer(InstanceIndex,0,instanceBuffer,0,instanceBuffer->size());
            instance.setSampledTexture(InstanceTextureIndex,0,texture);
            instance.setSampler(InstanceSamplerIndex,0,defaultSampler);

            commandBuffer->bindGraphicsDescriptorBundle(0,globals);
            commandBuffer->bindGraphicsDescriptorBundle(1,instance);
            commandBuffer->bindIndexBuffer(cubeIndices,slag::Buffer::IndexSize::UINT16,0);


            commandBuffer->bindVertexBuffers(0,vertexBuffers,vertexOffsets,vertexStrides,2);
            commandBuffer->setBlendConstants(1,.5,0,1);
            commandBuffer->drawIndexed(cindexes.size(),1,0,0,0);
            commandBuffer->endRendering();
            commandBuffer->end();

            slag::QueueSubmissionBatch submissionBatch;
            submissionBatch.commandBuffers = &commandBuffer;
            submissionBatch.commandBufferCount = 1;
            queue->submit(&submissionBatch,1,frame);
        }
    }
    delete swapChain;
    delete texture;
    delete defaultSampler;
    delete depthTexture;
    delete texturedDepthPipeline;
    delete cubeVerts;
    delete cubeUVs;
    delete cubeIndices;
    SDL_DestroyWindow(window);
    slag::cleanup();
}
