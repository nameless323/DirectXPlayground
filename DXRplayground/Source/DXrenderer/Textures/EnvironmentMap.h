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
    EnvironmentMap(RenderContext& ctx, const std::string& path, UINT cubemapSize, UINT irradianceMapSize);
    ~EnvironmentMap();

    void ConvertToCubemap(RenderContext& ctx);
    bool IsConvertedToCubemap() const;

private:
    struct
    {
        DirectX::XMFLOAT4 EqMapCubeMapWH{};
    } m_graphicsData;
    struct
    {
        float size;
    } m_convolutionData;

    void CreateRootSig(RenderContext& ctx);
    void CreateDescriptorHeap(RenderContext& ctx);
    void CreateViews(RenderContext& ctx);
    void Convolute(RenderContext& ctx);
    
    TexResourceData m_cubemapData;
    TexResourceData m_envMapData;
    TexResourceData m_irradianceMapData;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
    UploadBuffer* m_dataBuffer = nullptr;
    UploadBuffer* m_convolutionDataBuffer = nullptr;
    const std::string m_psoName = "CubemapConvertor_PBR";
    bool m_converted = false;
    UINT m_cubemapSize = 0;
    UINT m_irradianceMapSize = 0;
};

}
