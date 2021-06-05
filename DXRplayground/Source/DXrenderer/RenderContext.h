#pragma once

#include <windows.h>

namespace DirectxPlayground
{
class Swapchain;
class TextureManager;
class PsoManager;

struct RenderContext
{
    static constexpr UINT FramesCount = 3;
    static constexpr UINT MaxTextures = 10000;
    static constexpr UINT MaxCubemaps = 64;
    static constexpr UINT MaxCubemapsUAV = 64;
    static constexpr UINT MaxRT = 100;
    UINT CbvSrvUavDescriptorSize = -1;
    UINT RtvDescriptorSize = -1;
    UINT DsvDescriptorSize = -1;
    UINT SamplerDescriptorSize = -1;

    UINT Width = 0;
    UINT Height = 0;

    ID3D12GraphicsCommandList* CommandList = nullptr;
    ID3D12Device5* Device = nullptr;
    Swapchain* SwapChain = nullptr;
    TextureManager* TexManager = nullptr;
    PsoManager* PsoManager = nullptr;
};
}
