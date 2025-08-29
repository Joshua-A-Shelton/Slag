#define SDL_MAIN_HANDLED
#include <gtest/gtest.h>
#include <slag/Slag.h>

#include "VulkanEnvironment.h"


int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    ::testing::AddGlobalTestEnvironment(new slag::VulkanEnvironment());

    auto run = RUN_ALL_TESTS();
    return run;
}
