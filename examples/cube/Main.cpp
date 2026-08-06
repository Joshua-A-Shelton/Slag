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
#include <thread>
#include <chrono>

using namespace slag;

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
    graphicsCard->transferQueue()->submit(batch);
    finished->waitForValue(1);
    return texture;
}

int main()
{

    BackendAPI backend = BackendAPI::VULKAN;

#ifdef SLAG_DX12_BACKEND
    backend = BackendAPI::DX12;
#endif

#ifdef SLAG_VULKAN_BACKEND
#ifdef SLAG_DX12_BACKEND
#endif
    int number = -1;
    while (number != 1 && number != 2)
    {
        std::cout << "Pick a backend:\n1) Vulkan\n2) DirectX 12" << std::endl;
        if (std::cin >> number)
        {
            if (number == 1)
            {
                backend = BackendAPI::VULKAN;
            }
            else if (number == 2)
            {
                backend = BackendAPI::DX12;
            }
        }
        else
        {
            std::cout << "Invalid input" << std::endl;
            return 1;
        }
    }
#endif

     auto result = slag::Slag::initialize(slag::InitializationData
    {
        .backend = backend,
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
         std::this_thread::sleep_for(std::chrono::milliseconds(5000));
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
    SDL_Window* window = SDL_CreateWindow("Slag Cube Example", (int)(300 * main_scale), (int)(300 * main_scale), window_flags);



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
        pd.details.wayland.surface = static_cast<wl_surface*>(SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));
        pd.details.wayland.display = static_cast<wl_display*>(SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr));
    }
    else
    {
        pd.platform = slag::Platform::X11;
        pd.details.x11.display = static_cast<Display*>(SDL_GetPointerProperty(properties,SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr));
        pd.details.x11.window = SDL_GetNumberProperty(properties,SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    }
#endif

    slag::SwapChainParameters swapchainParameters{};
    auto swapChain = graphicsCard->newSwapchain(pd,300,300,swapchainParameters);
    auto queue = graphicsCard->graphicsQueue();
    auto resourceHeap = graphicsCard->newResourceDescriptorHeap(100000);
    auto samplerHeap = graphicsCard->newSamplerDescriptorHeap(100);

    auto resourceHeapPtr = (unsigned char*) resourceHeap->data();
    auto samplerHeapPtr = (unsigned char*) samplerHeap->data();


    slag::Semaphore* commandsFinished = nullptr;
    slag::CommandBuffer* commandBuffer = graphicsCard->newCommandBuffer(slag::QueueType::GRAPHICS);
    slag::Texture* depthTarget = graphicsCard->newTexture2D(300,300,slag::PixelFormat::D32_FLOAT,slag::TextureUsageFlags::DEPTH_STENCIL_TARGET);

    auto sampler = graphicsCard->newSampler();
    auto texture = loadTexture("resources/examples/textures/gradient.jpg",graphicsCard);
    auto globals = graphicsCard->newBuffer(256,slag::BufferCPUAccess::WRITE_ONLY, slag::BufferMemoryType::UNIFORM);
    auto transform = graphicsCard->newBuffer(256,slag::BufferCPUAccess::WRITE_ONLY, slag::BufferMemoryType::UNIFORM);
    auto proj = glm::perspective(95.0f,(float)depthTarget->width()/(float)depthTarget->height(),.01f,100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view,glm::vec3(0.0f,2.0f,5.0f));
    view = glm::rotate(view,glm::radians(-20.0f),glm::vec3(1.0f,0.0f,0.0f));
    view = glm::inverse(view);
    glm::mat4 projectionView = proj*view;
    auto globalsPtr = globals->as<glm::mat4>();
    globalsPtr[0] = proj;
    globalsPtr[1] = view;
    globalsPtr[2] = projectionView;

    glm::mat4 objectTransform = glm::rotate(glm::mat4(1),glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));
    auto transformPtr = transform->as<glm::mat4>();
    transformPtr[0] = objectTransform;

    auto vertexModule = createShaderModule(graphicsCard, "resources/examples/shaders/compiled/TexturedDepthBindless.vertex");
    auto fragmentModule = createShaderModule(graphicsCard, "resources/examples/shaders/compiled/TexturedDepthBindless.fragment");
    std::vector<slag::VertexBinding> vertexBindings = {
        slag::VertexBinding(0,sizeof(float)*3,InputRate::PER_VERTEX,std::vector<slag::VertexAttribute>{slag::VertexAttribute("POSITION",slag::PixelFormat::R32G32B32_FLOAT,0)}),
        slag::VertexBinding(1,sizeof(float)*2,InputRate::PER_VERTEX,std::vector<slag::VertexAttribute>{slag::VertexAttribute("UV_COORDINATES",slag::PixelFormat::R32G32_FLOAT,0)}),
    };
    slag::VertexDescription vertexDescription(vertexBindings);
    slag::PipelineState pipelineState{};
    slag::FramebufferDescription framebufferDesc;
    framebufferDesc.colorFormats[0] = swapChain->parameters().imageFormat;
    framebufferDesc.depthFormat = depthTarget->format();
    auto pipeline = graphicsCard->newShaderPipeline(vertexDescription,vertexModule.details,fragmentModule.details,pipelineState,framebufferDesc);

    auto descriptorDetails = graphicsCard->descriptorHeapDetails();

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
            auto colorTarget = frame->renderBuffer();
            if (depthTarget->width() != colorTarget->width() || depthTarget->height() != colorTarget->height())
            {
                delete depthTarget;
                depthTarget = graphicsCard->newTexture2D(colorTarget->width(),colorTarget->height(),slag::PixelFormat::D32_FLOAT,slag::TextureUsageFlags::DEPTH_STENCIL_TARGET);
            }

            objectTransform = glm::rotate(objectTransform,glm::radians(45.0f)*delta,glm::vec3(0.0f,1.0f,0.0f));
            transformPtr[0] = objectTransform;

            commandBuffer->begin();

            commandBuffer->bindDescriptorHeaps(resourceHeap,samplerHeap);
            commandBuffer->setViewPort(0,0,colorTarget->width(),colorTarget->height(),0.0f,1.0f);
            commandBuffer->setScissors(slag::Rectangle{0,0,colorTarget->width(),colorTarget->height()});
            Attachment colorAttachment(colorTarget,true,ClearValue{.5,.2,.1,1});
            Attachment depthAttachment(depthTarget,true,ClearValue{1,0});

            TextureBarrier barriers[]
            {
                TextureBarrier
                {
                    colorTarget,
                    0,
                    1,
                    0,
                    1,
                    SyncStages::ALL,
                    SyncStages::ALL_GRAPHICS,
                    MemoryCaches::NONE,
                    MemoryCaches::NONE,
                    TextureLayout::UNKNOWN,
                    TextureLayout::COLOR_TARGET
                },
                TextureBarrier
                {
                    depthTarget,
                    0,
                    1,
                    0,
                    1,
                    SyncStages::ALL,
                    SyncStages::ALL_GRAPHICS,
                    MemoryCaches::NONE,
                    MemoryCaches::NONE,
                    TextureLayout::UNKNOWN,
                    TextureLayout::DEPTH_STENCIL_TARGET
                },
            };
            commandBuffer->insertBarriers(barriers,2);

            commandBuffer->beginRendering(&colorAttachment,1,&depthAttachment,slag::Rectangle{0,0,colorTarget->width(),colorTarget->height()});

            Buffer* buffers[] = {cubeVerts, cubeUVs};
            uint64_t offsets[] = {0,0};
            uint64_t strides[] = {sizeof(glm::vec3),sizeof(glm::vec2)};
            commandBuffer->bindVertexBuffers(0,buffers,offsets,strides,2);
            commandBuffer->bindIndexBuffer(cubeIndices,IndexBufferType::UINT_16,0);
            commandBuffer->bindShaderPipeline(pipeline);


            graphicsCard->writeUniformBufferDescriptor(globals,0,globals->size(),resourceHeapPtr);
            graphicsCard->writeUniformBufferDescriptor(transform,0,transform->size(),resourceHeapPtr+descriptorDetails.textureDescriptorSize);
            graphicsCard->writeUniformTextureDescriptor(texture,0,1,0,1,resourceHeapPtr+(descriptorDetails.textureDescriptorSize*2));
            graphicsCard->writeSamplerDescriptor(sampler,samplerHeapPtr);

            uint32_t globalIndex = 0;
            uint32_t instanceIndex = 1;
            uint32_t textureIndex = 2;
            uint32_t samplerIndex = 0;
            commandBuffer->setGraphicsShaderParameters(0,&globalIndex,sizeof(uint32_t));
            commandBuffer->setGraphicsShaderParameters(8,&instanceIndex,sizeof(uint32_t));
            commandBuffer->setGraphicsShaderParameters(16,&textureIndex,sizeof(uint32_t));
            commandBuffer->setGraphicsShaderParameters(24,&samplerIndex,sizeof(uint32_t));

            commandBuffer->drawIndexed(cindexes.size(),1,0,0,0);

            commandBuffer->endRendering();

            barriers[0].layoutBefore = TextureLayout::COLOR_TARGET;
            barriers[0].layoutAfter = TextureLayout::PRESENT;
            barriers[0].syncBefore = SyncStages::ALL_GRAPHICS;
            barriers[0].syncAfter = SyncStages::ALL;
            barriers[0].flush = MemoryCaches::COLOR_TARGET;
            barriers[0].invalidate = MemoryCaches::NONE;
            commandBuffer->insertBarriers(barriers,1);

            commandBuffer->end();

            slag::SemaphoreValue signal{.semaphore = commandsFinished, .value = 1};

            slag::SubmissionBatch submissionBatch;
            submissionBatch.commandBuffers = &commandBuffer;
            submissionBatch.commandBufferCount = 1;
            submissionBatch.signalSemaphores = &signal;
            submissionBatch.signalSemaphoreCount = 1;
            queue->submit(submissionBatch);
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
    delete depthTarget;
    delete pipeline;
    delete cubeVerts;
    delete cubeUVs;
    delete cubeIndices;
    SDL_DestroyWindow(window);
    slag::Slag::cleanup();
}
