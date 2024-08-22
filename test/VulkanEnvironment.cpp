#include "VulkanEnvironment.h"
#include <slag/SlagLib.h>
using namespace slag;
namespace slag
{
    namespace tests
    {

        void VulkanEnvironment::SetUp()
        {
            SlagInitDetails details{.backend = Vulkan};
            slag::SlagLib::initialize(details);
        }

        void VulkanEnvironment::TearDown()
        {
            slag::SlagLib::cleanup();
        }
    }
}