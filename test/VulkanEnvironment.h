#ifndef SLAG_VULKANENVIRONMENT_H
#define SLAG_VULKANENVIRONMENT_H

#include <gtest/gtest.h>

namespace slag
{
    namespace tests
    {
        class VulkanEnvironment: public ::testing::Environment
        {
            void SetUp() override;
            void TearDown() override;
        };
    }
}
#endif //SLAG_VULKANENVIRONMENT_H
