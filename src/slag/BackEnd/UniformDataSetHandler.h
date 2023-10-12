#ifndef SLAG_UNIFORMDATASETHANDLER_H
#define SLAG_UNIFORMDATASETHANDLER_H
#include "UniformDataSetFunctionProvider.h"
#include "../SlagLib.h"
namespace slag
{

    class UniformDataSetHandler
    {
    public:
        static void initialize(BackEnd backEnd);
        static void cleanup();
        static UniformDataSetFunctionProvider* dataProvider();
    };

} // slag

#endif //SLAG_UNIFORMDATASETHANDLER_H
