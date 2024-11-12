#include "DX12Environment.h"
#include <slag/SlagLib.h>

namespace slag
{
    namespace tests
    {
        bool DX12_TESTS_POST_SET_UP = false;
        void DX12DebugTest(std::string& message, SlagInitDetails::DebugLevel level, int32_t messageID)
        {
            if(messageID == 0)
            {
                return;
            }
            std::cout << message <<std::endl;
            if(level != SlagInitDetails::SLAG_MESSAGE && DX12_TESTS_POST_SET_UP)
            {
                GTEST_FAIL();
            }
        }

        void DX12Environment::SetUp()
        {
            SlagInitDetails details{.backend = DirectX12,.debug=true,.slagDebugHandler=DX12DebugTest};
            slag::SlagLib::initialize(details);
            DX12_TESTS_POST_SET_UP = true;
        }

        void DX12Environment::TearDown()
        {
            //this is a hack, but wait for 2 seconds to allow GPU to finish destroying objects before destroying so we don't get false positives on undestroyed objects
            std::this_thread::sleep_for(std::chrono::seconds(2));
            slag::SlagLib::cleanup();
            DX12_TESTS_POST_SET_UP = false;
        }
    } // tests
} // slag