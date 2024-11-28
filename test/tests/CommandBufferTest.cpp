#include "gtest/gtest.h"
#include "slag/SlagLib.h"
#include <stb_image.h>
#include <lodepng.h>

using namespace slag;

class CommandBufferTests: public ::testing::Test
{
protected:
    std::vector<GpuQueue::QueueType> commandBufferQueueTypes{GpuQueue::GRAPHICS, GpuQueue::COMPUTE, GpuQueue::TRANSFER};
    std::vector<GpuQueue*> submissionQueues;

public:
    CommandBufferTests()
    {
        auto card = SlagLib::graphicsCard();
        submissionQueues = {card->graphicsQueue(),card->computeQueue(),card->transferQueue()};

    }

    virtual ~CommandBufferTests() {}

    virtual void SetUp() {}

    virtual void TearDown() {}

    bool IsQueueGraphics(GpuQueue::QueueType type)
    {
        switch (type)
        {
            case GpuQueue::GRAPHICS:
                return true;
            case GpuQueue::COMPUTE:
                return SlagLib::graphicsCard()->computeQueue()->type() == slag::GpuQueue::GRAPHICS;
            case GpuQueue::TRANSFER:
                return SlagLib::graphicsCard()->transferQueue()->type() == slag::GpuQueue::GRAPHICS;
        }
        return false;
    }

};

TEST_F(CommandBufferTests, StartFinished)
{
    for(auto i=0; i< commandBufferQueueTypes.size(); i++)
    {
        std::unique_ptr<CommandBuffer> buffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(commandBufferQueueTypes[i]));
        ASSERT_TRUE(buffer->isFinished());
    }
}

TEST_F(CommandBufferTests, InsertBarriersMultiUseTexture)
{
    for(auto qt=0; qt< commandBufferQueueTypes.size(); qt++)
    {
        std::unique_ptr<CommandBuffer> buffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(commandBufferQueueTypes[qt]));
        std::unique_ptr<Texture> img = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UINT,slag::Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::SAMPLED_IMAGE | TextureUsageFlags::RENDER_TARGET_ATTACHMENT | TextureUsageFlags::STORAGE));
        ImageBarrier imageBarrier
                {
                        .texture = img.get(),
                };
        buffer->begin();
        std::vector<Texture::Layout> layouts =
                {
#define DEFINITION(slagName, vulkanName, directXName,directXResourceName) Texture::slagName,
                        TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
                };
        imageBarrier.oldLayout = Texture::UNDEFINED;
        for(size_t i=0; i< layouts.size(); i++)
        {
            auto newLayout = layouts[i];
            if(newLayout != Texture::UNDEFINED && newLayout != Texture::DEPTH_READ && newLayout != Texture::DEPTH_WRITE)
            {
                imageBarrier.newLayout = newLayout;
            }
            else
            {
                break;
            }
            buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
            imageBarrier.oldLayout = newLayout;

        }

        buffer->end();
        submissionQueues[qt]->submit(buffer.get());
        buffer->waitUntilFinished();
    }

}

TEST_F(CommandBufferTests, InsertBarriersSampledTexture)
{
    for(auto qt=0; qt< commandBufferQueueTypes.size(); qt++)
    {
        std::unique_ptr<CommandBuffer> buffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(commandBufferQueueTypes[qt]));
        std::unique_ptr<Texture> img = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UINT,slag::Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::SAMPLED_IMAGE));
        ImageBarrier imageBarrier
                {
                        .texture = img.get(),
                };
        buffer->begin();
        std::vector<Texture::Layout> layouts =
                {
#define DEFINITION(slagName, vulkanName, directXName,directXResourceName) Texture::slagName,
                        TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
                };
        imageBarrier.oldLayout = Texture::UNDEFINED;
        for(size_t i=0; i< layouts.size(); i++)
        {
            auto newLayout = layouts[i];
            if(newLayout != Texture::UNDEFINED && newLayout != Texture::DEPTH_READ && newLayout != Texture::DEPTH_WRITE && newLayout != Texture::RENDER_TARGET && newLayout != Texture::UNORDERED)
            {
                imageBarrier.newLayout = newLayout;
            }
            else
            {
                continue;
            }
            buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
            imageBarrier.oldLayout = newLayout;

        }

        buffer->end();
        submissionQueues[qt]->submit(buffer.get());
        buffer->waitUntilFinished();
    }
}

TEST_F(CommandBufferTests, InsertBarriersDepthTexture)
{
    for(auto qt=0; qt< commandBufferQueueTypes.size(); qt++)
    {
        std::unique_ptr<CommandBuffer> buffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(commandBufferQueueTypes[qt]));
        std::unique_ptr<Texture> img = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::D32_FLOAT_S8X24_UINT,slag::Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::DEPTH_STENCIL_ATTACHMENT));
        ImageBarrier imageBarrier
                {
                        .texture = img.get(),
                };
        buffer->begin();
        imageBarrier.oldLayout = Texture::UNDEFINED;
        imageBarrier.newLayout = Texture::GENERAL;
        buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
        imageBarrier.oldLayout = Texture::GENERAL;
        imageBarrier.newLayout = Texture::TRANSFER_SOURCE;
        buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
        imageBarrier.oldLayout = Texture::TRANSFER_SOURCE;
        imageBarrier.newLayout = Texture::TRANSFER_DESTINATION;
        buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
        imageBarrier.oldLayout = Texture::TRANSFER_DESTINATION;
        imageBarrier.newLayout = Texture::DEPTH_READ;
        buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
        imageBarrier.oldLayout = Texture::DEPTH_READ;
        imageBarrier.newLayout = Texture::DEPTH_WRITE;
        buffer->insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);
        buffer->end();
        submissionQueues[qt]->submit(buffer.get());
        buffer->waitUntilFinished();
    }
}

TEST_F(CommandBufferTests, InsertBarriersBuffers)
{
    for(auto qt=0; qt< commandBufferQueueTypes.size(); qt++)
    {
        auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(commandBufferQueueTypes[qt]));
        std::vector<Color> colors(100, {1.0f, 0.0f, 0.2f, 1.0f});
        std::vector<Color> colors2(100, {0.0f, 0.5f, 0.5f, 1.0f});
        auto data = std::unique_ptr<Buffer>(
                Buffer::newBuffer(colors.data(), colors.size() * sizeof(float) * 4, Buffer::GPU, Buffer::DATA_BUFFER));
        auto intermediate = std::unique_ptr<Buffer>(
                Buffer::newBuffer(colors.data(), colors.size() * sizeof(float) * 4, Buffer::CPU, Buffer::DATA_BUFFER));
        auto final = std::unique_ptr<Buffer>(Buffer::newBuffer(data->size(), Buffer::CPU_AND_GPU, Buffer::DATA_BUFFER));
        BufferBarrier barrier{.buffer=data.get(), .offset=0, .size=data->size(), .syncBefore=PipelineStageFlags::NONE, .syncAfter=PipelineStageFlags::TRANSFER};
        commandBuffer->begin();
        commandBuffer->copyBuffer(data.get(), 0, data->size(), intermediate.get(), 0);
        commandBuffer->insertBarriers(nullptr, 0, &barrier, 1, nullptr, 0);
        commandBuffer->copyBuffer(intermediate.get(), data->size() / 2, data->size() / 2, final.get(),
                                  data->size() / 2);
        barrier.buffer = intermediate.get();
        commandBuffer->insertBarriers(nullptr, 0, &barrier, 1, nullptr, 0);
        commandBuffer->copyBuffer(intermediate.get(), 0, data->size() / 2, final.get(), 0);

        commandBuffer->end();
        submissionQueues[qt]->submit(commandBuffer.get());
        auto dataRaw = data->downloadData();
        auto intermediateRaw = data->downloadData();
        auto finalRaw = data->downloadData();
        for (size_t i = 0; i < data->size(); i++)
        {
            GTEST_ASSERT_TRUE(dataRaw[i] == intermediateRaw[i] && intermediateRaw[i] == finalRaw[i]);
        }
    }
}

TEST_F(CommandBufferTests, InsertBarriersGlobal)
{
    for(auto qt=0; qt< commandBufferQueueTypes.size(); qt++)
    {
        auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(commandBufferQueueTypes[qt]));
        commandBuffer->begin();

        GPUMemoryBarrier barrier{.accessBefore = BarrierAccessFlags::NONE,.accessAfter=BarrierAccessFlags::ALL_WRITE,.syncBefore=PipelineStageFlags::NONE,.syncAfter=PipelineStageFlags::ALL_COMMANDS};
        commandBuffer->insertBarriers(nullptr,0, nullptr,0,&barrier,1);

        commandBuffer->end();
        submissionQueues[qt]->submit(commandBuffer.get());

    }
}

TEST_F(CommandBufferTests, ClearColorImage)
{
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(slag::GpuQueue::GRAPHICS));
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R32G32B32A32_FLOAT,slag::Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::SAMPLED_IMAGE));
    auto texture2 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R32G32B32A32_FLOAT,slag::Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::RENDER_TARGET_ATTACHMENT));
    auto texture3 = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R32G32B32A32_FLOAT,slag::Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::RENDER_TARGET_ATTACHMENT | TextureUsageFlags::SAMPLED_IMAGE));
    auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(texture->width()*texture->height()*sizeof(float)*4,Buffer::CPU,Buffer::DATA_BUFFER));
    auto buffer2 = std::unique_ptr<Buffer>(Buffer::newBuffer(texture2->width()*texture2->height()*sizeof(float)*4,Buffer::CPU,Buffer::DATA_BUFFER));
    auto buffer3 = std::unique_ptr<Buffer>(Buffer::newBuffer(texture3->width()*texture3->height()*sizeof(float)*4,Buffer::CPU,Buffer::DATA_BUFFER));


    commandBuffer->begin();
    commandBuffer->clearColorImage(texture.get(), {.floats={1,0,0,1}}, Texture::Layout::UNDEFINED, Texture::Layout::TRANSFER_SOURCE,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
    commandBuffer->clearColorImage(texture2.get(), {.floats={0,1,0,1}}, Texture::Layout::UNDEFINED, Texture::Layout::TRANSFER_SOURCE,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
    commandBuffer->clearColorImage(texture3.get(), {.floats={0,0,1,1}}, Texture::Layout::UNDEFINED, Texture::Layout::TRANSFER_SOURCE,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
    commandBuffer->copyImageToBuffer(texture.get(),Texture::Layout::TRANSFER_SOURCE,0,1,0,buffer.get(),0);
    commandBuffer->copyImageToBuffer(texture2.get(),Texture::Layout::TRANSFER_SOURCE,0,1,0,buffer2.get(),0);
    commandBuffer->copyImageToBuffer(texture3.get(),Texture::Layout::TRANSFER_SOURCE,0,1,0,buffer3.get(),0);


    commandBuffer->end();
    SlagLib::graphicsCard()->graphicsQueue()->submit(commandBuffer.get());
    commandBuffer->waitUntilFinished();
    auto data1 = buffer->downloadData();
    auto data2 = buffer2->downloadData();
    auto data3 = buffer3->downloadData();
    auto red = Color(1.0f,0,0,1);
    auto green = Color(0.0f,1,0,1);
    auto blue = Color(0.0f,0,1,1);
    for(size_t i=0; i< data1.size(); i+=4*sizeof(float))
    {
        Color color1 = *std::bit_cast<Color*>(&data1[i]);
        Color color2 = *std::bit_cast<Color*>(&data2[i]);
        Color color3 = *std::bit_cast<Color*>(&data3[i]);
        GTEST_ASSERT_TRUE(color1 == red);
        GTEST_ASSERT_TRUE(color2 == green);
        GTEST_ASSERT_TRUE(color3 == blue);
    }
}

TEST_F(CommandBufferTests, UpdateMipChain)
{

    std::unique_ptr<Texture> texture = std::unique_ptr<Texture>(Texture::newTexture("resources/test-img.png",Pixels::R8G8B8A8_UINT,5,TextureUsageFlags::SAMPLED_IMAGE,Texture::Layout::TRANSFER_SOURCE));

    std::unique_ptr<Texture> flatMipped = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R8G8B8A8_UINT,slag::Texture::TEXTURE_2D,150,100,1,1,1,TextureUsageFlags::SAMPLED_IMAGE));
    std::unique_ptr<Buffer> dataBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(flatMipped->width()*flatMipped->height()*sizeof(unsigned char)*4,Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::GRAPHICS));
    commandBuffer->begin();
    commandBuffer->updateMipChain(texture.get(),0,Texture::Layout::TRANSFER_SOURCE,Texture::Layout::TRANSFER_SOURCE,Texture::Layout::TRANSFER_SOURCE,Texture::Layout::TRANSFER_SOURCE,PipelineStageFlags::TRANSFER,PipelineStageFlags::ALL_GRAPHICS);
    commandBuffer->clearColorImage(flatMipped.get(),ClearColor{0,0,0,0},slag::Texture::UNDEFINED,slag::Texture::TRANSFER_DESTINATION,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS);
    Rectangle srcArea{.offset{},.extent{100,100}};
    Rectangle dstArea{.offset{},.extent{100,100}};
    commandBuffer->blit(texture.get(),Texture::TRANSFER_SOURCE,0,0,srcArea,flatMipped.get(),Texture::TRANSFER_DESTINATION,0,0,dstArea,Sampler::Filter::NEAREST);
    dstArea.offset.x = 100;
    for(uint32_t i=1; i< texture->mipLevels(); i++)
    {
        srcArea.extent.width = srcArea.extent.width>>1;
        srcArea.extent.height = srcArea.extent.height>>1;
        dstArea.extent.width = dstArea.extent.width>>1;
        dstArea.extent.height = dstArea.extent.height>>1;

        commandBuffer->blit(texture.get(),Texture::TRANSFER_SOURCE,0,i,srcArea,flatMipped.get(),Texture::TRANSFER_DESTINATION,0,0,dstArea,Sampler::Filter::NEAREST);

        dstArea.offset.y += dstArea.extent.height;
    }
    ImageBarrier flatMippedBarrier
    {
        .texture = flatMipped.get(),
        .oldLayout = Texture::TRANSFER_DESTINATION,
        .newLayout = Texture::TRANSFER_SOURCE,
        .accessBefore = BarrierAccessFlags::ALL_WRITE,
        .accessAfter = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE,
        .syncBefore = PipelineStageFlags::TRANSFER,
        .syncAfter = PipelineStageFlags::ALL_COMMANDS
    };
    commandBuffer->insertBarriers(&flatMippedBarrier,1, nullptr,0, nullptr,0);

    commandBuffer->copyImageToBuffer(flatMipped.get(),Texture::Layout::TRANSFER_SOURCE,0,1,0,dataBuffer.get(),0);
    commandBuffer->end();
    SlagLib::graphicsCard()->graphicsQueue()->submit(commandBuffer.get());
    commandBuffer->waitUntilFinished();

    int w, h, channels;
    auto file = std::filesystem::absolute("resources/test-img-mipped.png");
    auto rawBytes = stbi_load(file.string().c_str(),&w,&h,&channels,4);
    std::vector<std::byte> groundTruth(w*h*channels);
    memcpy(groundTruth.data(),rawBytes,w*h*channels);
    stbi_image_free(rawBytes);

    auto data = dataBuffer->downloadData();

    for(size_t i=0; i< w*h*channels; i++)
    {
        GTEST_ASSERT_EQ(data[i],groundTruth[i]);
    }
}

TEST_F(CommandBufferTests, CopyBuffer)
{
    for(auto qt=0; qt< commandBufferQueueTypes.size(); qt++)
    {
        std::unique_ptr<CommandBuffer> commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(commandBufferQueueTypes[qt]));
        commandBuffer->begin();
        std::vector<Color> rawData{Color(1.0f,0.0f,0.0f,1.0f),Color(.7f,.6f,.2f,1.0f),Color(0.0f,.9f,.25f,.33f)};
        std::unique_ptr<Buffer> cpuBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(rawData.data(), sizeof(Color) * rawData.size(), Buffer::Accessibility::CPU, Buffer::Usage::DATA_BUFFER));
        std::unique_ptr<Buffer> gpuBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(sizeof(Color) * rawData.size(), Buffer::Accessibility::GPU, Buffer::Usage::DATA_BUFFER));
        commandBuffer->copyBuffer(cpuBuffer.get(),0,cpuBuffer->size(),gpuBuffer.get(),0);
        commandBuffer->end();
        submissionQueues[qt]->submit(commandBuffer.get());
        commandBuffer->waitUntilFinished();
        auto data = gpuBuffer->downloadData();

        std::vector<Color> processedData(rawData.size());
        memcpy(processedData.data(),data.data(),data.size());
        for(int i=0; i<rawData.size(); i++)
        {
            auto original = rawData[i];
            auto proccessed = processedData[i];
            GTEST_ASSERT_TRUE(original == proccessed);
        }
    }
}

TEST_F(CommandBufferTests, CopyImageToBuffer)
{
    Color color(.5f,.3f,.1f,.8f);
    std::vector<Color> data(100,color);

    for(auto qt=0; qt< commandBufferQueueTypes.size(); qt++)
    {
        auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,slag::Texture::TEXTURE_2D,10,10,1,1,1,TextureUsageFlags::SAMPLED_IMAGE));
        auto uploadBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(data.data(),data.size()*sizeof(color),Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));
        auto downloadBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(uploadBuffer->size(),Buffer::CPU,Buffer::DATA_BUFFER));
        auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(commandBufferQueueTypes[qt]));
        commandBuffer->begin();
        ImageBarrier barrier{.texture=texture.get(),.oldLayout = Texture::UNDEFINED, .newLayout = slag::Texture::TRANSFER_DESTINATION};
        commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
        commandBuffer->copyBufferToImage(uploadBuffer.get(),0,texture.get(),slag::Texture::TRANSFER_DESTINATION,0,0);
        barrier.oldLayout = slag::Texture::TRANSFER_DESTINATION;
        barrier.newLayout = slag::Texture::TRANSFER_SOURCE;
        barrier.syncBefore = PipelineStageFlags::TRANSFER;
        barrier.syncAfter = PipelineStageFlags::TRANSFER;
        barrier.accessAfter = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
        commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
        commandBuffer->copyImageToBuffer(texture.get(),slag::Texture::TRANSFER_SOURCE,0,1,0,downloadBuffer.get(),0);
        commandBuffer->end();
        submissionQueues[qt]->submit(commandBuffer.get());
        commandBuffer->waitUntilFinished();

        auto download = downloadBuffer->downloadData();

        for(size_t i=0; i< download.size(); i+=4*sizeof(float))
        {
            Color comp = *std::bit_cast<Color*>(&download[i]);
            GTEST_ASSERT_TRUE(color == comp);
        }
    }
}

TEST_F(CommandBufferTests, CopyBufferToImage)
{
    Color color(.06f,.9f,.436f,.3f);
    std::vector<Color> data(100,color);

    for(auto qt=0; qt< commandBufferQueueTypes.size(); qt++)
    {
        auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::Format::R32G32B32A32_FLOAT,slag::Texture::TEXTURE_2D,10,10,1,1,1,TextureUsageFlags::SAMPLED_IMAGE));
        auto uploadBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(data.data(),data.size()*sizeof(color),Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));
        auto downloadBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(uploadBuffer->size(),Buffer::CPU,Buffer::DATA_BUFFER));
        auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(commandBufferQueueTypes[qt]));
        commandBuffer->begin();
        ImageBarrier barrier{.texture=texture.get(),.oldLayout = Texture::UNDEFINED, .newLayout = slag::Texture::TRANSFER_DESTINATION};
        commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
        commandBuffer->copyBufferToImage(uploadBuffer.get(),0,texture.get(),slag::Texture::TRANSFER_DESTINATION,0,0);
        barrier.oldLayout = slag::Texture::TRANSFER_DESTINATION;
        barrier.newLayout = slag::Texture::TRANSFER_SOURCE;
        barrier.syncBefore = PipelineStageFlags::TRANSFER;
        barrier.syncAfter = PipelineStageFlags::TRANSFER;
        barrier.accessAfter = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
        commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
        commandBuffer->copyImageToBuffer(texture.get(),slag::Texture::TRANSFER_SOURCE,0,1,0,downloadBuffer.get(),0);
        commandBuffer->end();
        submissionQueues[qt]->submit(commandBuffer.get());
        commandBuffer->waitUntilFinished();

        auto download = downloadBuffer->downloadData();

        for(size_t i=0; i< download.size(); i+=4*sizeof(float))
        {
            Color comp = *std::bit_cast<Color*>(&download[i]);
            GTEST_ASSERT_TRUE(color == comp);
        }
    }
}

TEST_F(CommandBufferTests, Blit)
{
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::GRAPHICS));
    auto sourceTexture = std::unique_ptr<Texture>(Texture::newTexture("resources\\solid-color.png",Pixels::R8G8B8A8_UNORM,1,TextureUsageFlags::SAMPLED_IMAGE,slag::Texture::TRANSFER_SOURCE));
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R32G32B32A32_FLOAT,slag::Texture::TEXTURE_2D,10,10,2,1,1,TextureUsageFlags::SAMPLED_IMAGE));
    auto memory = std::unique_ptr<Buffer>(Buffer::newBuffer(5*5*4*sizeof(float),Buffer::CPU,Buffer::DATA_BUFFER));
    commandBuffer->begin();
    ImageBarrier barrier{.texture=texture.get(),.baseLayer=0,.layerCount=1,.baseMipLevel=0,.mipCount=0,.oldLayout=Texture::UNDEFINED,.newLayout=Texture::TRANSFER_DESTINATION};
    commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
    commandBuffer->blit(sourceTexture.get(),Texture::TRANSFER_SOURCE,0,0,Rectangle{{0,0},{sourceTexture->width(),sourceTexture->height()}},texture.get(),Texture::TRANSFER_DESTINATION,0,0,Rectangle{{0,0},{10,10}},Sampler::NEAREST);
    barrier.mipCount=1;
    barrier.baseMipLevel = 0;
    barrier.newLayout = slag::Texture::TRANSFER_SOURCE;
    commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
    commandBuffer->blit(texture.get(),Texture::TRANSFER_SOURCE,0,0,Rectangle{{0,0},{10,10}},texture.get(),Texture::TRANSFER_DESTINATION,0,1,Rectangle{{0,0},{5,5}},Sampler::NEAREST);
    barrier.oldLayout = Texture::TRANSFER_DESTINATION;
    barrier.newLayout = Texture::TRANSFER_SOURCE;
    barrier.baseMipLevel = 1;
    commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
    commandBuffer->copyImageToBuffer(texture.get(),Texture::TRANSFER_SOURCE,0,1,1,memory.get(),0);
    commandBuffer->end();
    SlagLib::graphicsCard()->graphicsQueue()->submit(commandBuffer.get());
    commandBuffer->waitUntilFinished();
    auto data = memory->downloadData();
    Color orange(1.0f,0.607843161f,0,1);
    for(size_t i=0; i<data.size(); i+=sizeof(float)*4)
    {
        Color comp = *std::bit_cast<Color*>(&data[i]);
        auto hex = comp.hexCode();
        GTEST_ASSERT_EQ(comp,orange);
    }
}

TEST_F(CommandBufferTests, BeginQuery)
{
    GTEST_FAIL();
}
TEST_F(CommandBufferTests, BeginRendering)
{
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UNORM,Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::RENDER_TARGET_ATTACHMENT));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::GRAPHICS));
    commandBuffer->begin();
    ImageBarrier barrier{.texture = texture.get(),.oldLayout=Texture::UNDEFINED,.newLayout=Texture::RENDER_TARGET};
    commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
    Attachment colorAttachment{.texture=texture.get(),.layout=Texture::RENDER_TARGET,.clearOnLoad=false};
    commandBuffer->beginRendering(&colorAttachment,1, nullptr,Rectangle{{0,0},{texture->width(),texture->height()}});
    commandBuffer->endRendering();
    commandBuffer->end();
    SlagLib::graphicsCard()->graphicsQueue()->submit(commandBuffer.get());
    commandBuffer->waitUntilFinished();
}
TEST_F(CommandBufferTests, BindIndexBuffer)
{
    std::vector<uint16_t> indicies = {0,1,2};
    std::vector<Buffer::Accessibility> accessTypes = {Buffer::CPU,Buffer::CPU_AND_GPU,Buffer::GPU};
    for(auto access: accessTypes)
    {
        auto indexBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(indicies.data(),sizeof(uint16_t)*3,access,Buffer::INDEX_BUFFER));
        auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::GRAPHICS));
        commandBuffer->begin();
        commandBuffer->bindIndexBuffer(indexBuffer.get(),Buffer::UINT16,0);
        commandBuffer->end();
        SlagLib::graphicsCard()->graphicsQueue()->submit(commandBuffer.get());
        commandBuffer->waitUntilFinished();
    }


}

TEST_F(CommandBufferTests, BindGraphicsShader)
{
    FrameBufferDescription description;
    description.addColorTarget(Pixels::R8G8B8A8_UNORM);

    ShaderModule modules[2]={ShaderModule(ShaderStageFlags::VERTEX,"resources\\basic.vert.spv"),ShaderModule(ShaderStageFlags::FRAGMENT,"resources\\basic.frag.spv")};
    ShaderProperties shaderProps;
    auto shader = std::unique_ptr<Shader>(Shader::newShader(modules,2, nullptr,0,shaderProps, nullptr,description));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::GRAPHICS));
    commandBuffer->begin();
    commandBuffer->bindGraphicsShader(shader.get());
    commandBuffer->end();
    SlagLib::graphicsCard()->graphicsQueue()->submit(commandBuffer.get());
    commandBuffer->waitUntilFinished();
}

TEST_F(CommandBufferTests, BindComputeShader)
{
    GTEST_FAIL();
}
TEST_F(CommandBufferTests, BindVertexBuffers)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, ClearDepthStencilImage)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, CopyQueryPoolResults)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, Dispatch)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, DispatchBase)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, DispatchIndirect)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, Draw)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, DrawIndexed)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, DrawIndexedIndirect)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, DrawIndexedIndirectCount)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, DrawIndirect)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, DrawIndirectCount)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, EndQuery)
{
    GTEST_FAIL();
}

TEST_F(CommandBufferTests, EndRendering)
{
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UNORM,Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::RENDER_TARGET_ATTACHMENT));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::GRAPHICS));
    commandBuffer->begin();
    ImageBarrier barrier{.texture = texture.get(),.oldLayout=Texture::UNDEFINED,.newLayout=Texture::RENDER_TARGET};
    commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
    Attachment colorAttachment{.texture=texture.get(),.layout=Texture::RENDER_TARGET,.clearOnLoad=false};
    commandBuffer->beginRendering(&colorAttachment,1, nullptr,Rectangle{{0,0},{texture->width(),texture->height()}});
    commandBuffer->endRendering();
    commandBuffer->end();
    SlagLib::graphicsCard()->graphicsQueue()->submit(commandBuffer.get());
    commandBuffer->waitUntilFinished();
}

TEST_F(CommandBufferTests, FillBuffer)
{
    std::vector<uint32_t> initData(64,0);
    for(auto i=0; i<this->commandBufferQueueTypes.size(); i++)
    {
        auto buffer = std::unique_ptr<Buffer>(Buffer::newBuffer(initData.data(),initData.size(),Buffer::CPU_AND_GPU,Buffer::DATA_BUFFER));
        auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(commandBufferQueueTypes[i]));

        commandBuffer->begin();
        commandBuffer->fillBuffer(buffer.get(),4,buffer->size()-8,UINT32_MAX);
        commandBuffer->end();
        this->submissionQueues[i]->submit(commandBuffer.get());
        commandBuffer->waitUntilFinished();
        auto data = buffer->downloadData();
        for(size_t j=0; j< data.size(); j+=sizeof(uint32_t))
        {
            uint32_t number = *std::bit_cast<uint32_t*>(&data[j]);
            if(j==0 || j==data.size()-sizeof(uint32_t))
            {
                GTEST_ASSERT_EQ(number,0);
            }
            else
            {
                GTEST_ASSERT_EQ(number,UINT32_MAX);

            }
        }
    }

}

TEST_F(CommandBufferTests, ResetQueryPool)
{
    GTEST_FAIL();
}
//--------------------------------------------------------------------------------DEATH TESTS-----------------------------------------------------------------------------------------------------------
TEST_F(CommandBufferTests, DisallowCompute_clearColorImage)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::COMPUTE))
    {
        GTEST_SKIP();
    }

    //Compute should possibly allow clearing, look into this
    GTEST_ASSERT_TRUE(false);

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::COMPUTE));
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UINT,slag::Texture::TEXTURE_2D,10,10,1,1,1,TextureUsageFlags::SAMPLED_IMAGE));
    commandBuffer->begin();
    ASSERT_DEATH(commandBuffer->clearColorImage(texture.get(),ClearColor{0,0,0,0},Texture::UNDEFINED,Texture::GENERAL,PipelineStageFlags::NONE,PipelineStageFlags::ALL_COMMANDS),"");
}

TEST_F(CommandBufferTests, DisallowTransfer_clearColorImage)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::TRANSFER))
    {
        GTEST_SKIP();
    }

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::TRANSFER));
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UINT,slag::Texture::TEXTURE_2D,10,10,1,1,1,TextureUsageFlags::SAMPLED_IMAGE));
    commandBuffer->begin();
    ASSERT_DEATH(commandBuffer->clearColorImage(texture.get(),ClearColor{0,0,0,0},Texture::UNDEFINED,Texture::GENERAL,PipelineStageFlags::NONE,PipelineStageFlags::ALL_COMMANDS),"");
}

TEST_F(CommandBufferTests, DisallowCompute_updateMipChain)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::COMPUTE))
    {
        GTEST_SKIP();
    }


    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::COMPUTE));
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UINT,slag::Texture::TEXTURE_2D,10,10,2,1,1,TextureUsageFlags::SAMPLED_IMAGE));
    commandBuffer->begin();
    ASSERT_DEATH(commandBuffer->updateMipChain(texture.get(),0,slag::Texture::UNDEFINED,slag::Texture::GENERAL,Texture::UNDEFINED,Texture::GENERAL,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS),"");
}

TEST_F(CommandBufferTests, DisallowTransfer_updateMipChain)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::TRANSFER))
    {
        GTEST_SKIP();
    }

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::TRANSFER));
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UINT,slag::Texture::TEXTURE_2D,10,10,2,1,1,TextureUsageFlags::SAMPLED_IMAGE));
    commandBuffer->begin();
    ASSERT_DEATH(commandBuffer->updateMipChain(texture.get(),0,slag::Texture::UNDEFINED,slag::Texture::GENERAL,Texture::UNDEFINED,Texture::GENERAL,PipelineStageFlags::NONE,PipelineStageFlags::ALL_GRAPHICS),"");
}

TEST_F(CommandBufferTests, DisallowCompute_blit)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::COMPUTE))
    {
        GTEST_SKIP();
    }


    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::COMPUTE));
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UINT,slag::Texture::TEXTURE_2D,10,10,2,1,1,TextureUsageFlags::SAMPLED_IMAGE));
    commandBuffer->begin();
    ImageBarrier barrier{.texture=texture.get(),.baseLayer=0,.layerCount=1,.baseMipLevel=0,.mipCount=1,.oldLayout=Texture::UNDEFINED,.newLayout=Texture::TRANSFER_SOURCE};
    commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
    barrier.baseMipLevel = 1;
    barrier.newLayout = slag::Texture::TRANSFER_DESTINATION;
    commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
    ASSERT_DEATH(commandBuffer->blit(texture.get(),Texture::TRANSFER_SOURCE,0,0,Rectangle{{0,0,},{10,10}},texture.get(),Texture::TRANSFER_DESTINATION,0,1,Rectangle{{0,0},{5,5}},Sampler::NEAREST),"");
}

TEST_F(CommandBufferTests, DisallowTransfer_blit)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::TRANSFER))
    {
        GTEST_SKIP();
    }

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::TRANSFER));
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UINT,slag::Texture::TEXTURE_2D,10,10,2,1,1,TextureUsageFlags::SAMPLED_IMAGE));
    commandBuffer->begin();
    ImageBarrier barrier{.texture=texture.get(),.baseLayer=0,.layerCount=1,.baseMipLevel=0,.mipCount=1,.oldLayout=Texture::UNDEFINED,.newLayout=Texture::TRANSFER_SOURCE};
    commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
    barrier.baseMipLevel = 1;
    barrier.newLayout = slag::Texture::TRANSFER_DESTINATION;
    commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
    ASSERT_DEATH(commandBuffer->blit(texture.get(),Texture::TRANSFER_SOURCE,0,0,Rectangle{{0,0,},{10,10}},texture.get(),Texture::TRANSFER_DESTINATION,0,1,Rectangle{{0,0},{5,5}},Sampler::NEAREST),"");
}

TEST_F(CommandBufferTests, DisallowCompute_beginRendering)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::COMPUTE))
    {
        GTEST_SKIP();
    }
    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UNORM,Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::RENDER_TARGET_ATTACHMENT));

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::COMPUTE));
    commandBuffer->begin();
    ImageBarrier barrier{.texture = texture.get(),.oldLayout=Texture::UNDEFINED,.newLayout=Texture::RENDER_TARGET};
    commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
    Attachment colorAttachment{.texture=texture.get(),.layout=Texture::RENDER_TARGET,.clearOnLoad=false};
    ASSERT_DEATH(commandBuffer->beginRendering(&colorAttachment,1, nullptr,Rectangle{{0,0},{texture->width(),texture->height()}}),"");
}

TEST_F(CommandBufferTests, DisallowTransfer_beginRendering)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::TRANSFER))
    {
        GTEST_SKIP();
    }

    auto texture = std::unique_ptr<Texture>(Texture::newTexture(Pixels::R8G8B8A8_UNORM,Texture::TEXTURE_2D,100,100,1,1,1,TextureUsageFlags::RENDER_TARGET_ATTACHMENT));

    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::COMPUTE));
    commandBuffer->begin();
    ImageBarrier barrier{.texture = texture.get(),.oldLayout=Texture::UNDEFINED,.newLayout=Texture::RENDER_TARGET};
    commandBuffer->insertBarriers(&barrier,1, nullptr,0, nullptr,0);
    Attachment colorAttachment{.texture=texture.get(),.layout=Texture::RENDER_TARGET,.clearOnLoad=false};
    ASSERT_DEATH(commandBuffer->beginRendering(&colorAttachment,1, nullptr,Rectangle{{0,0},{texture->width(),texture->height()}}),"");
}

TEST_F(CommandBufferTests, DisallowCompute_bindIndexBuffer)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::COMPUTE))
    {
        GTEST_SKIP();
    }
    std::vector<uint16_t> indicies = {0,1,2};
    auto indexBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(indicies.data(),sizeof(uint16_t)*3,Buffer::CPU_AND_GPU,Buffer::INDEX_BUFFER));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::COMPUTE));
    commandBuffer->begin();
    ASSERT_DEATH(commandBuffer->bindIndexBuffer(indexBuffer.get(),Buffer::UINT16,0),"");
}

TEST_F(CommandBufferTests, DisallowTransfer_bindIndexBuffer)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::TRANSFER))
    {
        GTEST_SKIP();
    }
    std::vector<uint16_t> indicies = {0,1,2};
    auto indexBuffer = std::unique_ptr<Buffer>(Buffer::newBuffer(indicies.data(),sizeof(uint16_t)*3,Buffer::CPU_AND_GPU,Buffer::INDEX_BUFFER));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::TRANSFER));
    commandBuffer->begin();
    ASSERT_DEATH(commandBuffer->bindIndexBuffer(indexBuffer.get(),Buffer::UINT16,0),"");
}

TEST_F(CommandBufferTests, DisallowGraphics_bindGraphicsShader)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::COMPUTE))
    {
        GTEST_SKIP();
    }

    FrameBufferDescription description;
    description.addColorTarget(Pixels::R8G8B8A8_UNORM);

    ShaderModule modules[2]={ShaderModule(ShaderStageFlags::VERTEX,"resources\\basic.vert.spv"),ShaderModule(ShaderStageFlags::FRAGMENT,"resources\\basic.frag.spv")};
    ShaderProperties shaderProps;
    auto shader = std::unique_ptr<Shader>(Shader::newShader(modules,2, nullptr,0,shaderProps, nullptr,description));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::COMPUTE));
    commandBuffer->begin();
    ASSERT_DEATH(commandBuffer->bindGraphicsShader(shader.get()),"");

}

TEST_F(CommandBufferTests, DisallowTransfer_bindGraphicsShader)
{
#ifdef NDEBUG
    GTEST_SKIP();
#endif
    if(IsQueueGraphics(GpuQueue::TRANSFER))
    {
        GTEST_SKIP();
    }

    FrameBufferDescription description;
    description.addColorTarget(Pixels::R8G8B8A8_UNORM);

    ShaderModule modules[2]={ShaderModule(ShaderStageFlags::VERTEX,"resources\\basic.vert.spv"),ShaderModule(ShaderStageFlags::FRAGMENT,"resources\\basic.frag.spv")};
    ShaderProperties shaderProps;
    auto shader = std::unique_ptr<Shader>(Shader::newShader(modules,2, nullptr,0,shaderProps, nullptr,description));
    auto commandBuffer = std::unique_ptr<CommandBuffer>(CommandBuffer::newCommandBuffer(GpuQueue::TRANSFER));
    commandBuffer->begin();
    ASSERT_DEATH(commandBuffer->bindGraphicsShader(shader.get()),"");

}