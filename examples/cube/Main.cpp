#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <slag/Slag.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void graphicsDebug(const std::string& message, slag::DebugLevel level, int32_t id)
{
    std::cout << message << std::endl;
}
struct ShaderModule
{
    slag::ShaderCode details;
    std::vector<std::byte> loadedCode;
};
ShaderModule createShaderModule(slag::GraphicsCard* card, const std::string& shaderIdentifier)
{
    auto api = slag::Slag::backend()->api();
    std::filesystem::path shaderPath = shaderIdentifier;
    auto language =  slag::ShaderLanguage::CUSTOM;
    switch (api)
    {
    case slag::BackendAPI::VULKAN:
        shaderPath += ".spv";
        language = slag::ShaderLanguage::SPIRV;
        break;
    case slag::BackendAPI::DX12:
        shaderPath += ".dxil";
        language = slag::ShaderLanguage::DXIL;
        break;
    default:
        throw std::runtime_error("Invalid backend API");
    }
    std::ifstream file(shaderPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    ShaderModule shaderModule{};

    const std::size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    shaderModule.loadedCode.resize(fileSize);
    file.read(reinterpret_cast<char*>(shaderModule.loadedCode.data()), fileSize);
    file.close();

    shaderModule.details.code = shaderModule.loadedCode.data();
    shaderModule.details.codeLength = fileSize;
    shaderModule.details.language = language;
    return shaderModule;
}
template<typename T>
using unique_ptr_custom = std::unique_ptr<T,std::function<void(T*)>>;
slag::Texture* loadTexture(std::filesystem::path const& path, slag::GraphicsCard* graphicsCard)
{
    if (!std::filesystem::exists(path))
    {
        throw std::runtime_error("Texture does not exist");
    }
    int width, height, channels;
    unique_ptr_custom<stbi_uc> data(stbi_load(path.string().c_str(), &width, &height,&channels,4),[](stbi_uc* uc){stbi_image_free(uc);});
    slag::TextureBufferMapping mapping
    {
        .bufferOffset = 0,
        .subresource =
        {
             .aspect = slag::PixelAspect::COLOR,
             .mipLevel = 0,
             .baseArrayLayer = 0,
             .layerCount = 1,
        },
        .offset = {0,0,0},
        .extent = {(uint32_t)width,(uint32_t)height,1}
    };
    auto texture = graphicsCard->newTexture2D((uint32_t)width, (uint32_t)height,slag::PixelFormat::R8G8B8A8_UNORM,slag::TextureUsageFlags::SAMPLED,1);
    auto pixels = std::unique_ptr<slag::Buffer>(graphicsCard->newBuffer(width*height*4,slag::BufferCPUAccess::WRITE_ONLY));
    auto pixelsPtr = pixels->as<uint8_t>();
    memcpy(pixelsPtr,data.get(),width*height*4);
    auto commandBuffer = std::unique_ptr<slag::CommandBuffer>(graphicsCard->newCommandBuffer(slag::QueueType::TRANSFER));
    auto finished = std::unique_ptr<slag::Semaphore>(graphicsCard->newSemaphore());

    commandBuffer->begin();
    commandBuffer->copyBufferToTexture(pixels.get(),texture,&mapping,1);
    commandBuffer->end();

    auto cmdBuffer = commandBuffer.get();
    slag::SemaphoreValue signal{.semaphore = finished.get(),.value =1};
    slag::SubmissionBatch batch
    {
        .waitSemaphores = nullptr,
        .waitSemaphoreCount = 0,
        .commandBuffers = &cmdBuffer,
        .commandBufferCount = 1,
        .signalSemaphores = &signal,
        .signalSemaphoreCount = 1,
    };
    graphicsCard->transferQueue()->submit(&batch,1);
    finished->waitForValue(1);
    return texture;
}

int main()
{
     auto result = slag::Slag::initialize(slag::InitializationData
    {
        .backend = slag::BackendAPI::VULKAN,
        .customBackend = nullptr,
        .debugHandler = graphicsDebug
    });

     if (result != slag::SlagInitializationResult::SUCCESS)
     {
         std::cout << "Failed to initialize slag" << std::endl;
         switch (result)
         {
         case slag::SlagInitializationResult::SUCCESS:
             break;
         case slag::SlagInitializationResult::BACKEND_UNSUPPORTED:
             std::cout << "Backend not supported" << std::endl;
             break;
         case slag::SlagInitializationResult::INSUFFICIENT_CAPABILITIES:
             std::cout << "Backend api version does not support required features" << std::endl;
             break;
         case slag::SlagInitializationResult::NO_GRAPHICS_CARDS:
             std::cout << "No graphics cards with required features found" << std::endl;
             break;
         }
         return 1;
     }

    auto graphicsCard = slag::Slag::backend()->graphicsCard(0);

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

    auto cubeVerts = graphicsCard->newBuffer(cverts.size()*sizeof(float),slag::BufferCPUAccess::WRITE_ONLY);
    auto cubeUVs = graphicsCard->newBuffer(cuvs.size()*sizeof(float),slag::BufferCPUAccess::WRITE_ONLY);
    auto cubeIndices = graphicsCard->newBuffer(cindexes.size()*sizeof(uint16_t),slag::BufferCPUAccess::WRITE_ONLY);
    auto cvertsPtr = cubeVerts->as<float>();
    memcpy(cvertsPtr,cverts.data(),cverts.size()*sizeof(float));
    auto cuvsPtr = cubeUVs->as<float>();
    memcpy(cuvsPtr,cuvs.data(),cuvs.size()*sizeof(float));
    auto cindexesPtr = cubeIndices->as<uint16_t>();
    memcpy(cindexesPtr,cindexes.data(),cindexes.size()*sizeof(uint16_t));

    slag::PlatformData pd{};

    SDL_Init(SDL_INIT_VIDEO);
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui Slag Example", (int)(300 * main_scale), (int)(300 * main_scale), window_flags);



    auto properties = SDL_GetWindowProperties(window);
#ifdef _WIN32
    pd.platform = slag::Platform::WIN_32;
    pd.details.win32.hwnd = (HWND)SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    pd.details.win32.hinstance = (HINSTANCE)SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
#else
    const char* driverName = SDL_GetCurrentVideoDriver();
    if (strcmp(driverName, "wayland") == 0)
    {
        pd.platform = slag::Platform::WAYLAND;
        pd.details.wayland.surface = SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
        pd.details.wayland.display = SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
    }
    else
    {
        pd.platform = slag::Platform::X11;
        pd.details.x11.display = SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        pd.details.x11.window = reinterpret_cast<void*>(SDL_GetNumberProperty(properties,SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
    }
#endif

    slag::SwapChainParameters swapchainParameters{};
    auto swapChain = graphicsCard->newSwapchain(pd,300,300,swapchainParameters);
    auto queue = graphicsCard->graphicsQueue();
    auto resourceHeap = graphicsCard->newResourceDescriptorHeap(100000);
    auto samplerHeap = graphicsCard->newSamplerDescriptorHeap(100);
    uint64_t resourceDescriptorMemoryOffset = 0;
    uint64_t samplerDescriptorMemoryOffset = 0;

    slag::Buffer* vertexBuffers[]={cubeVerts,cubeUVs};
    uint64_t vertexOffsets[]={0,0};
    uint64_t vertexStrides[]{sizeof(glm::vec3),sizeof(glm::vec2)};

    glm::mat4 instanceMatrix(1);

    slag::Semaphore* commandsFinished = nullptr;
    slag::CommandBuffer* commandBuffer = graphicsCard->newCommandBuffer(slag::QueueType::GRAPHICS);
    slag::Texture* depthBuffer = graphicsCard->newTexture2D(300,300,slag::PixelFormat::D32_FLOAT,slag::TextureUsageFlags::DEPTH_STENCIL_TARGET);

    auto sampler = graphicsCard->newSampler();
    auto texture = loadTexture("resources/examples/textures/gradient.jpg",graphicsCard);
    auto globals = graphicsCard->newBuffer(192,slag::BufferCPUAccess::WRITE_ONLY, slag::BufferMemoryType::GENERAL);
    auto transform = graphicsCard->newBuffer(64,slag::BufferCPUAccess::WRITE_ONLY, slag::BufferMemoryType::GENERAL);
    auto proj = glm::perspective(95.0f,(float)depthBuffer->width()/(float)depthBuffer->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);
    glm::mat4 projectionView = proj*view;
    auto globalsPtr = globals->as<glm::mat4>();
    globalsPtr[0] = proj;
    globalsPtr[1] = view;
    globalsPtr[2] = projectionView;

    auto transformPtr = transform->as<glm::mat4>();
    transformPtr[0] = glm::rotate(glm::mat4(1),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));

    auto vertexModule = createShaderModule(graphicsCard, "resources/examples/shaders/compiled/TexturedDepthBindless.vertex");
    auto fragmentModule = createShaderModule(graphicsCard, "resources/examples/shaders/compiled/TexturedDepthBindless.fragment");
    std::vector<slag::VertexBinding> vertexBindings = {
        slag::VertexBinding(0,sizeof(float)*3,std::vector<slag::VertexAttribute>{slag::VertexAttribute("POSITION",slag::PixelFormat::R32G32B32_FLOAT,0)}),
        slag::VertexBinding(1,sizeof(float)*2,std::vector<slag::VertexAttribute>{slag::VertexAttribute("UV_COORDINATES",slag::PixelFormat::R32G32_FLOAT,0)}),
    };
    slag::VertexDescription vertexDescription(vertexBindings);
    slag::PipelineState pipelineState{};
    slag::FramebufferDescription framebufferDesc;
    framebufferDesc.colorFormats[0] = swapChain->parameters().imageFormat;
    framebufferDesc.depthFormat = depthBuffer->format();
    auto shader = graphicsCard->newShaderPipeline(vertexDescription,vertexModule.details,fragmentModule.details,pipelineState,framebufferDesc);

    bool keepOpen = true;
    uint32_t last_tick_time = 0;
    float delta = 0;
    while(keepOpen)
    {

        uint32_t tick_time = SDL_GetTicks();
        delta = ((float)tick_time - (float)last_tick_time)/1000;
        last_tick_time = tick_time;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                keepOpen = false;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
            {
                keepOpen = false;
            }
        }

        if (auto frame = swapChain->next())
        {
            if (commandsFinished)
            {
                delete commandsFinished;
            }
            commandsFinished = graphicsCard->newSemaphore();
            auto backBuffer = frame->renderBuffer();
            if (depthBuffer->width() != backBuffer->width() || depthBuffer->height() != backBuffer->height())
            {
                delete depthBuffer;
                depthBuffer = graphicsCard->newTexture2D(backBuffer->width(),backBuffer->height(),slag::PixelFormat::D32_FLOAT,slag::TextureUsageFlags::DEPTH_STENCIL_TARGET);
            }

            commandBuffer->begin();
            commandBuffer->bindDescriptorHeaps(resourceHeap,samplerHeap);
            slag::TextureBarrier barrier
            {
                .texture = backBuffer,
                .baseLayer = 0,
                .layerCount = 1,
                .baseMipLevel = 0,
                .mipCount = 1,
                .syncBefore = slag::SyncStages::ALL,
                .syncAfter = slag::SyncStages::ALL,
                .flush = slag::MemoryCaches::NONE,
                .invalidate = slag::MemoryCaches::NONE,
                .layoutBefore = slag::TextureLayout::UNKNOWN,
                .layoutAfter = slag::TextureLayout::COLOR_TARGET,
            };
            slag::TextureBarrier depthBarrier
            {
                .texture = depthBuffer,
                .baseLayer = 0,
                .layerCount = 1,
                .baseMipLevel = 0,
                .mipCount = 1,
                .syncBefore = slag::SyncStages::ALL,
                .syncAfter = slag::SyncStages::ALL,
                .flush = slag::MemoryCaches::NONE,
                .invalidate = slag::MemoryCaches::NONE,
                .layoutBefore = slag::TextureLayout::UNKNOWN,
                .layoutAfter = slag::TextureLayout::DEPTH_STENCIL_TARGET,
            };
            commandBuffer->insertBarriers(&barrier,1);
            commandBuffer->insertBarriers(&depthBarrier,1);
            commandBuffer->setViewPort(0,0,backBuffer->width(),backBuffer->height(),0,1);
            commandBuffer->setScissors(slag::Rectangle{{0,0},{backBuffer->width(),backBuffer->height()}});


            slag::Attachment colorAttachment{.texture = backBuffer,.autoClear = true, .clearValue = slag::ClearColor{.floats = {0,.2,1,1} }};
            slag::Attachment depthAttachment{.texture = depthBuffer,.autoClear = true };
            depthAttachment.clearValue.depthStencil = {1,0};

            commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{{0,0},{backBuffer->width(),backBuffer->height()}});

            commandBuffer->bindGraphicsPipeline(shader);

            slag::Buffer* buffers[] = {cubeVerts,cubeUVs};
            uint64_t offsets[] = {0,0};
            uint64_t strides[] = {sizeof(glm::vec3),sizeof(glm::vec2)};
            commandBuffer->bindVertexBuffers(0,buffers,offsets,strides,2);
            commandBuffer->bindIndexBuffer(cubeIndices,slag::IndexBufferType::UINT_16,0);

            resourceHeap->setStorageStructuredBuffer(0,globals,0,1, globals->size());
            resourceHeap->setStorageStructuredBuffer(1,transform,0,1,transform->size());
            resourceHeap->setUniformTexture(2,texture,0,1,0,1);
            samplerHeap->setSampler(0,sampler);

            uint32_t globalIndex = 0;
            uint32_t instanceIndex = 1;
            uint32_t textureIndex = 2;
            uint32_t samplerIndex = 0;
            commandBuffer->setGraphicsShaderParameters(0,&globalIndex,sizeof(uint32_t));
            commandBuffer->setGraphicsShaderParameters(4,&instanceIndex,sizeof(uint32_t));
            commandBuffer->setGraphicsShaderParameters(8,&textureIndex,sizeof(uint32_t));
            commandBuffer->setGraphicsShaderParameters(12,&samplerIndex,sizeof(uint32_t));

            commandBuffer->drawIndexed(cindexes.size(),1,0,0,0);
            commandBuffer->endRendering();

            barrier.layoutBefore = slag::TextureLayout::COLOR_TARGET;
            barrier.layoutAfter = slag::TextureLayout::PRESENT;
            commandBuffer->insertBarriers(&barrier,1);
            commandBuffer->end();

            slag::SemaphoreValue signal{.semaphore = commandsFinished, .value = 1};

            slag::SubmissionBatch submissionBatch;
            submissionBatch.commandBuffers = &commandBuffer;
            submissionBatch.commandBufferCount = 1;
            submissionBatch.signalSemaphores = &signal;
            submissionBatch.signalSemaphoreCount = 1;
            queue->submit(&submissionBatch,1);
            commandsFinished->waitForValue(1);
            swapChain->present();
        }
    }
    commandsFinished->waitForValue(1);
    delete commandsFinished;
    delete commandBuffer;
    delete resourceHeap;
    delete samplerHeap;
    delete globals;
    delete transform;
    delete swapChain;
    delete sampler;
    delete texture;
    delete depthBuffer;
    delete shader;
    delete cubeVerts;
    delete cubeUVs;
    delete cubeIndices;
    SDL_DestroyWindow(window);
    slag::Slag::cleanup();
}
