#pragma once

#include <windows.h>

namespace DXRplayground
{
struct DXsharedState
{
    static constexpr UINT FramesCount = 3;
    UINT CbvSrvUavDescriptorSize = -1;
    UINT RtvDescriptorSize = -1;
    UINT DsvDescriptorSize = -1;
    UINT SamplerDescriptorSize = -1;
};
}
