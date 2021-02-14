#pragma once

#include <windows.h>

namespace DirectxPlayground
{
class Swapchain;

struct RenderContext
{
    static constexpr UINT FramesCount = 3;
    UINT CbvSrvUavDescriptorSize = -1;
    UINT RtvDescriptorSize = -1;
    UINT DsvDescriptorSize = -1;
    UINT SamplerDescriptorSize = -1;

    int Width = 0;
    int Height = 0;

    ID3D12GraphicsCommandList* CommandList = nullptr;
    ID3D12Device* Device = nullptr;
    Swapchain* SwapChain = nullptr;
};
}
