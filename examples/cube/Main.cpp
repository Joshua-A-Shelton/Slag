#define SDL_MAIN_HANDLED
#include <iostream>
#include <SDL2/SDL.h>
#include <slag/Slag.h>
#include <glm/glm.hpp>

#include "SDL_syswm.h"
class DefaultResources: public slag::FrameResources
{
public:
    slag::CommandBuffer* commandBuffer=nullptr;
    slag::Semaphore* commandsFinished=nullptr;
    DefaultResources():slag::FrameResources()
    {
        commandBuffer = slag::CommandBuffer::newCommandBuffer(slag::GPUQueue::QueueType::GRAPHICS);
        commandsFinished = slag::Semaphore::newSemaphore();
    }
    ~DefaultResources()override
    {
        delete commandBuffer;
        delete commandsFinished;
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
    depthTexture = slag::Texture::newTexture(slag::Pixels::Format::D32_FLOAT,slag::Texture::Type::TEXTURE_2D,slag::Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT,swapChain->backBufferWidth(),swapChain->backBufferHeight(),1,1);
}

void backEndMessage(const std::string& message, slag::SlagDebugLevel level, int32_t messageID)
{
    std::cout << message << std::endl;
}

int main()
{
    slag::initialize(slag::SlagInitInfo{.graphicsBackend = slag::GraphicsBackend::VULKAN_GRAPHICS_BACKEND, .slagDebugHandler = backEndMessage});

    std::vector<glm::vec3> cverts =
        {
            {-0.5f, -0.5f, -0.5f},  // A 0
            {0.5f, -0.5f, -0.5f},  // B 1
            {0.5f,  0.5f, -0.5f},  // C 2
            {-0.5f,  0.5f, -0.5f},  // D 3
            {-0.5f, -0.5f,  0.5f},  // E 4
            {0.5f, -0.5f,  0.5f},  // F 5
            {0.5f,  0.5f,  0.5f},  // G 6
            {-0.5f,  0.5f,  0.5f}, // H 7

            {-0.5f,  0.5f, -0.5f}, // D 8
            {-0.5f, -0.5f, -0.5f}, // A 9
            {-0.5f, -0.5f,  0.5f}, // E 10
            {-0.5f,  0.5f,  0.5f}, // H 11
            {0.5f, -0.5f, -0.5f},  // B 12
            {0.5f,  0.5f, -0.5f},  // C 13
            {0.5f,  0.5f,  0.5f},  // G 14
            {0.5f, -0.5f,  0.5f},  // F 15

            {-0.5f, -0.5f, -0.5f}, // A 16
            {0.5f, -0.5f, -0.5f},  // B 17
            {0.5f, -0.5f,  0.5f},  // F 18
            {-0.5f, -0.5f,  0.5f}, // E 19
            {0.5f,  0.5f, -0.5f},  // C 20
            {-0.5f,  0.5f, -0.5f}, // D 21
            {-0.5f,  0.5f,  0.5f}, // H 22
            {0.5f,  0.5f,  0.5f},  // G 23
        };
        std::vector<glm::vec2> cuvs =
        {
            {0.0f, 0.0f},  // A 0
            {1.0f, 0.0f},  // B 1
            {1.0f, 1.0f},  // C 2
            {0.0f, 1.0f},  // D 3
            {0.0f, 0.0f},  // E 4
            {1.0f, 0.0f},   // F 5
            {1.0f, 1.0f},   // G 6
            {0.0f, 1.0f},   // H 7

            {0.0f, 0.0f},  // D 8
            {1.0f, 0.0f},  // A 9
            {1.0f, 1.0f},  // E 10
            {0.0f, 1.0f},  // H 11
            {0.0f, 0.0f},   // B 12
            {1.0f, 0.0f},   // C 13
            {1.0f, 1.0f},   // G 14
            {0.0f, 1.0f},   // F 15

            {0.0f, 0.0f},  // A 16
            {1.0f, 0.0f},   // B 17
            {1.0f, 1.0f},   // F 18
            {0.0f, 1.0f},  // E 19
            {0.0f, 0.0f},  // C 20
            {1.0f, 0.0f},  // D 21
            {1.0f, 1.0f},  // H 22
            {0.0f, 1.0f}  // G 23
        };
        std::vector<uint16_t> cindexes =
        {
            // front and back
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

    slag::Buffer* cubeVerts = slag::Buffer::newBuffer(cverts.data(),cverts.size()*sizeof(glm::vec3),slag::Buffer::Accessibility::GPU,slag::Buffer::UsageFlags::VERTEX_BUFFER);
    slag::Buffer* cubeUVs = slag::Buffer::newBuffer(cuvs.data(),cuvs.size()*sizeof(glm::vec2),slag::Buffer::Accessibility::GPU,slag::Buffer::UsageFlags::VERTEX_BUFFER);
    slag::Buffer* cubeIndices = slag::Buffer::newBuffer(cindexes.data(),cindexes.size()*sizeof(uint16_t),slag::Buffer::Accessibility::GPU,slag::Buffer::UsageFlags::INDEX_BUFFER);



    slag::VertexDescription vertexPosUVDescription(2);
    vertexPosUVDescription.add(slag::GraphicsType::VECTOR3,0,0).add(slag::GraphicsType::VECTOR2,0,1);

    slag::ShaderCode vertexShader(slag::ShaderStageFlags::VERTEX,slag::ShaderCode::CodeLanguage::SPIRV,"resources/shaders/TexturedDepth.vertex.spv");
    slag::ShaderCode fragmentShader(slag::ShaderStageFlags::FRAGMENT,slag::ShaderCode::CodeLanguage::SPIRV,"resources/shaders/TexturedDepth.fragment.spv");
    slag::ShaderCode* code[]{&vertexShader,&fragmentShader};
    slag::ShaderProperties properties{};
    slag::FrameBufferDescription frameBufferDescription{};
    frameBufferDescription.colorTargets[0] = slag::Pixels::Format::B8G8R8A8_UNORM_SRGB;
    frameBufferDescription.depthTarget = slag::Pixels::Format::D32_FLOAT;

    slag::ShaderPipeline* texturedDepthPipeline = slag::ShaderPipeline::newShaderPipeline(code,2,properties,vertexPosUVDescription,frameBufferDescription);
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

    auto swapChain = slag::SwapChain::newSwapChain(pd,WINDOW_WIDTH,WINDOW_HEIGHT,slag::SwapChain::PresentMode::QUEUE,2,slag::Pixels::Format::B8G8R8A8_UNORM_SRGB,slag::SwapChain::AlphaCompositing::IGNORE_ALPHA,createResource,swapChainRebuilt);
    auto queue = slag::slagGraphicsCard()->graphicsQueue();
    bool keepOpen = true;
    while(keepOpen)
    {
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

            auto backBuffer = frame->backBuffer();

            commandBuffer->begin();
            commandBuffer->clearTexture(backBuffer,slag::ClearColor{.floats = {0,.2,.8,1}});
            commandBuffer->end();

            slag::QueueSubmissionBatch submissionBatch;
            submissionBatch.commandBuffers = &commandBuffer;
            submissionBatch.commandBufferCount = 1;
            queue->submit(&submissionBatch,1,frame);
        }
    }
    delete swapChain;
    delete defaultSampler;
    delete depthTexture;
    delete texturedDepthPipeline;
    delete cubeVerts;
    delete cubeUVs;
    delete cubeIndices;
    SDL_DestroyWindow(window);
    slag::cleanup();
}
