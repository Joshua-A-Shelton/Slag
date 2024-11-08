#include "VulkanEnvironment.h"
#include <slag/SlagLib.h>
#include "gtest/gtest.h"

using namespace slag;
namespace slag
{
    namespace tests
    {

        bool POST_SET_UP = false;
        void VulkanDebugTest(std::string& message, SlagInitDetails::DebugLevel level, int32_t messageID)
        {
            std::cout << message <<std::endl;
            if(level != SlagInitDetails::SLAG_MESSAGE && POST_SET_UP)
            {
                GTEST_FAIL();
            }
        }
        void VulkanEnvironment::SetUp()
        {
            SlagInitDetails details{.backend = Vulkan,.debug=true,.slagDebugHandler=VulkanDebugTest};
            slag::SlagLib::initialize(details);
            POST_SET_UP = true;
        }

        void VulkanEnvironment::TearDown()
        {
            slag::SlagLib::cleanup();

        }
    }
}