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
    } mGraphicsData {};
    struct
    {
        float size;
    } mConvolutionData {};

    void CreateRootSig(const RenderContext& ctx);
    void CreateDescriptorHeap(RenderContext& ctx);
    void CreateViews(RenderContext& ctx) const;
    void Convolute(RenderContext& ctx) const;
    
    TexResourceData mCubemapData;
    TexResourceData mEnvMapData;
    TexResourceData mIrradianceMapData;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSig;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
    UploadBuffer* mDataBuffer = nullptr;
    UploadBuffer* mConvolutionDataBuffer = nullptr;
    const std::string mPsoName = "CubemapConvertor_PBR";
    const std::string mConvolutionPsoName = "CubemapConvolution_PBR";
    bool mConverted = false;
    UINT mCubemapSize = 0;
    UINT mIrradianceMapSize = 0;
};

}
