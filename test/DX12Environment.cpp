#include "DX12Environment.h"
#include <slag/SlagLib.h>

namespace slag
{
    namespace tests
    {
        void DX12Environment::SetUp()
        {
            SlagInitDetails details{.backend = DirectX12};
            slag::SlagLib::initialize(details);
        }

        void DX12Environment::TearDown()
        {
            slag::SlagLib::cleanup();
        }
    } // tests
} // slag