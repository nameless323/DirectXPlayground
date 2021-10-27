#pragma once

#include "DXrenderer/DXhelpers.h"
#include "External/Dx12Helpers/d3dx12.h"

#include "DXrenderer/Textures/TextureManager.h"

namespace DirectxPlayground
{
class UnorderedAccessBuffer;
class UploadBuffer;

class EnvironmentMap
{
public:
    EnvironmentMap(RenderContext& ctx, const std::string& path, UINT width, UINT height);
    ~EnvironmentMap();

    void ConvertToCubemap(RenderContext& ctx);
    bool IsConvertedToCubemap() const;

private:
    struct
    {
        UINT EnvMapIndex;
        UINT CubemapIndex;
    } m_graphicsData;

    void CreateRootSig(RenderContext& ctx);
    void CreateDescriptorHeap(RenderContext& ctx);
    void CreateViews(RenderContext& ctx);
    
    TexResourceData m_cubemapData;
    TexResourceData m_envMapData;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
    UploadBuffer* m_indicesBuffer = nullptr;
    const std::string m_psoName = "CubemapConvertor_PBR";
    bool m_converted = false;
    UINT m_cubemapHeight = 0;
    UINT m_cubemapWidth = 0;
};

}
