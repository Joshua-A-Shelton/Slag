#include <gtest/gtest.h>
#include <slag/Slag.h>
#include "../utilities/GeneralUtilities.h"
using namespace slag;

TEST(Buffer, Create)
{
#ifdef SLAG_DEBUG
    GTEST_FLAG_SET(death_test_style, "threadsafe");
#endif
    auto card = Slag::backend()->graphicsCard(0);
    auto bufferType = sequentialEnumRange(BufferUsage::ARBITRARY,BufferUsage::INDIRECT);
    auto shaderAccess = sequentialEnumRange(BufferShaderAccess::READ_ONLY,BufferShaderAccess::READ_WRITE);
    auto cpuAccess = sequentialEnumRange(BufferCPUAccess::NONE, BufferCPUAccess::READ_WRITE);

    for (auto type: bufferType)
    {
        for (auto shader: shaderAccess)
        {
            for (auto cpu: cpuAccess)
            {
                auto buffer = std::unique_ptr<Buffer>(card->newBuffer(256,type,shader,cpu));
                GTEST_ASSERT_EQ(buffer->size(),256);
                GTEST_ASSERT_EQ(buffer->usage(),type);
                GTEST_ASSERT_EQ(buffer->shaderAccess(),shader);
                GTEST_ASSERT_EQ(buffer->cpuAccess(),cpu);
                GTEST_ASSERT_EQ(buffer->graphicsCard(),card);
                if (cpu != BufferCPUAccess::NONE)
                {
                    auto ptr = buffer->data();
                    GTEST_ASSERT_TRUE(ptr != nullptr);
                }
#ifdef SLAG_DEBUG
                else
                {
                    EXPECT_DEBUG_DEATH(auto ptr = buffer->data(),"Attempted to access cpu handle of inaccessible buffer");
                }
#endif
            }
        }
    }
}

TEST(Buffer, ReadWrite)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto bufferType = sequentialEnumRange(BufferUsage::ARBITRARY,BufferUsage::INDIRECT);
    auto shaderAccess = sequentialEnumRange(BufferShaderAccess::READ_ONLY,BufferShaderAccess::READ_WRITE);
    std::vector<uint8_t> data(1024);
    for (auto i=0; i<data.size(); ++i)
    {
        data[i] = i%256;
    }

    for (auto type: bufferType)
    {
        for (auto shader: shaderAccess)
        {
            auto buffer = std::unique_ptr<Buffer>(card->newBuffer(data.size(),type,shader,BufferCPUAccess::READ_WRITE));
            for (int i=0; i< buffer->size(); ++i)
            {
                buffer->as<uint8_t>()[i] = data[i];
                GTEST_ASSERT_EQ(buffer->as<uint8_t>()[i],data[i]);
            }
        }
    }
}
#ifdef SLAG_DEBUG
TEST(Buffer, ShaderReadOnlyAlignment)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);
    EXPECT_DEATH(auto buffer = std::unique_ptr<Buffer>(card->newBuffer(64,BufferUsage::ARBITRARY,BufferShaderAccess::READ_ONLY,BufferCPUAccess::NONE)),"Buffers with BufferShaderAccess::READ_ONLY must be a multiple of 256 bytes in size");
}

TEST(Buffer, ShaderReadOnlyMaxSize)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);
    auto size = card->maxShaderAccessReadOnlyBufferSize()+256;
    EXPECT_DEATH(auto buffer = std::unique_ptr<Buffer>(card->newBuffer(size,BufferUsage::ARBITRARY,BufferShaderAccess::READ_ONLY,BufferCPUAccess::NONE)),"Buffers with BufferShaderAccess::READ_ONLY cannot exceed size found in GraphicsCard::maxShaderAccessReadOnlyBufferSize");
}
#endif
