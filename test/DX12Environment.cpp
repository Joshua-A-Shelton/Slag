#include "DX12Environment.h"
#include <slag/SlagLib.h>

namespace slag
{
    namespace tests
    {
        void DX12DebugTest(std::string& message, SlagInitDetails::DebugLevel level, int32_t messageID)
        {
            std::cout << message <<std::endl;
            if(level != SlagInitDetails::SLAG_MESSAGE)
            {
                GTEST_FAIL();
            }
        }

        void DX12Environment::SetUp()
        {
            SlagInitDetails details{.backend = DirectX12,.debug=true,.slagDebugHandler=DX12DebugTest};
            slag::SlagLib::initialize(details);
        }

        void DX12Environment::TearDown()
        {
            slag::SlagLib::cleanup();
        }
    } // tests
} // slag