#include "d3d12/D3D12Backend.h"

namespace slag {

const BackendInfo* slagBackend_d3d12()
{
    static const BackendFunctions functions = {
#define SLAG_BACKEND_FN(ReturnType, name, parameters) &d3d12::name,
#include "common/BackendFunctions.inl"
#undef SLAG_BACKEND_FN
    };
    static const BackendInfo info = {Backend::D3D12, "D3D12", &functions};
    return &info;
}

} // namespace slag
