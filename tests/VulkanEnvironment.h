#ifndef SLAG_VULKANENVOIRNMENT_H
#define SLAG_VULKANENVOIRNMENT_H
#include <gtest/gtest.h>

namespace slag
{
    class VulkanEnvironment: public ::testing::Environment
    {
        void SetUp() override;
        void TearDown() override;
    };
} // slag

#endif //SLAG_VULKANENVOIRNMENT_H
