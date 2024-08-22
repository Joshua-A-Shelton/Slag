#ifndef SLAG_DX12ENVIRONMENT_H
#define SLAG_DX12ENVIRONMENT_H
#include <gtest/gtest.h>

namespace slag
{
    namespace tests
    {

        class DX12Environment: public ::testing::Environment
        {
            void SetUp() override;
            void TearDown() override;
        };

    } // tests
} // slag

#endif //SLAG_DX12ENVIRONMENT_H
