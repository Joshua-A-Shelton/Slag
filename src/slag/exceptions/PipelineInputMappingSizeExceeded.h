#ifndef SLAG_PIPELINEINPUTMAPPINGSIZEEXCEEDED_H
#define SLAG_PIPELINEINPUTMAPPINGSIZEEXCEEDED_H
#include <stdexcept>

namespace slag
{
    class PipelineInputMappingSizeExceeded: public std::runtime_error
    {
    public:
        PipelineInputMappingSizeExceeded(const std::string& message):runtime_error(message){}
    };
}
#endif //SLAG_PIPELINEINPUTMAPPINGSIZEEXCEEDED_H