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
    auto memoryType = sequentialEnumRange(BufferMemoryType::UNIFORM,BufferMemoryType::GENERAL);
    auto cpuAccess = sequentialEnumRange(BufferCPUAccess::NONE, BufferCPUAccess::READ_WRITE);

    for (auto type: memoryType)
    {
        for (auto cpu: cpuAccess)
        {
            auto buffer = std::unique_ptr<Buffer>(card->newBuffer(256,type,cpu));
            GTEST_ASSERT_EQ(buffer->size(),256);
            GTEST_ASSERT_EQ(buffer->memoryType(),type);
            GTEST_ASSERT_EQ(buffer->cpuAccess(),cpu);
            GTEST_ASSERT_EQ(buffer->graphicsCard(),card);
            GTEST_ASSERT_EQ(buffer->userData(),nullptr);
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

TEST(Buffer, ReadWrite)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto memoryType = sequentialEnumRange(BufferMemoryType::UNIFORM,BufferMemoryType::GENERAL);
    std::vector<uint8_t> data(1024);
    for (auto i=0; i<data.size(); ++i)
    {
        data[i] = i%256;
    }

    for (auto type: memoryType)
    {
        auto buffer = std::unique_ptr<Buffer>(card->newBuffer(data.size(),type,BufferCPUAccess::READ_WRITE));
        for (int i=0; i< buffer->size(); ++i)
        {
            buffer->as<uint8_t>()[i] = data[i];
            GTEST_ASSERT_EQ(buffer->as<uint8_t>()[i],data[i]);
        }
    }
}

TEST(Buffer, UserData)
{
    auto card = Slag::backend()->graphicsCard(0);
    auto buffer = std::unique_ptr<Buffer>(card->newBuffer(256));
    buffer->setUserData(card);
    GTEST_ASSERT_EQ(buffer->userData(),card);
    uint32_t index = 3;
    buffer->setUserData(&index);
    GTEST_ASSERT_EQ(*((uint32_t*)buffer->userData()),index);
}

#ifdef SLAG_DEBUG
TEST(Buffer, ShaderReadOnlyAlignment)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);
    EXPECT_DEATH(auto buffer = std::unique_ptr<Buffer>(card->newBuffer(64,BufferMemoryType::UNIFORM,BufferCPUAccess::NONE)),"Buffers with BufferMemoryType::UNIFORM must be a multiple of 256 bytes in size");
}

TEST(Buffer, ShaderReadOnlyMaxSize)
{
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    auto card = Slag::backend()->graphicsCard(0);
    auto size = card->maxShaderAccessUniformBufferSize()+256;
    EXPECT_DEATH(auto buffer = std::unique_ptr<Buffer>(card->newBuffer(size,BufferMemoryType::UNIFORM,BufferCPUAccess::NONE)),"Buffers with BufferMemoryType::UNIFORM cannot exceed size found in GraphicsCard::maxShaderAccessUniformBufferSize");
}
#endif
