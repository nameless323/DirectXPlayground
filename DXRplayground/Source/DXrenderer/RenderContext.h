#pragma once

#include <windows.h>

namespace DirectxPlayground
{
class Swapchain;
class TextureManager;
class PsoManager;
class IRenderPipeline;
class ImguiTextureManager;

struct RenderContext
{
    static constexpr UINT FramesCount = 3;
    static constexpr UINT MaxTextures = 10000;
    static constexpr UINT MaxCubemaps = 64;
    static constexpr UINT MaxCubemapsUAV = 64;
    static constexpr UINT MaxRT = 100;
    static constexpr UINT AllocatorsCount = 6;
    UINT CbvSrvUavDescriptorSize = -1;
    UINT RtvDescriptorSize = -1;
    UINT DsvDescriptorSize = -1;
    UINT SamplerDescriptorSize = -1;

    UINT Width = 0;
    UINT Height = 0;

    ID3D12GraphicsCommandList5* CommandList = nullptr;
    ID3D12Device5* Device = nullptr;
    Swapchain* SwapChain = nullptr;
    TextureManager* TexManager = nullptr;
    ImguiTextureManager* ImguiTexManager = nullptr;
    PsoManager* PsoManager = nullptr;

    IRenderPipeline* Pipeline = nullptr;
};
}
