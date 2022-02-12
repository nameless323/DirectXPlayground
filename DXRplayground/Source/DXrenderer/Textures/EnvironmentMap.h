#pragma once

#include "DXrenderer/DXhelpers.h"
#include "External/Dx12Helpers/d3dx12.h"

#include "DXrenderer/Textures/TextureManager.h"

namespace DirectxPlayground
{
class UnorderedAccessBuffer;
class UploadBuffer;

struct EnvironmentData
{
    UINT CubemapIndex;
    UINT IrradianceMapIndex;
    UINT Pad0;
    UINT Pad1;
};

class EnvironmentMap
{
public:
    EnvironmentMap(RenderContext& ctx, const std::string& path, UINT cubemapSize, UINT irradianceMapSize);
    ~EnvironmentMap();

    void ConvertToCubemap(RenderContext& ctx);
    bool IsConvertedToCubemap() const;

    UINT GetCubemapIndex() const;
    UINT GetIrradianceMapIndex() const;

private:
    static constexpr UINT DownsampledCubeSize = 64;
    struct
    {
        DirectX::XMFLOAT4 EqMapCubeMapWH{};
    } mGraphicsData {};
    struct
    {
        DirectX::XMFLOAT2 size;
    } mConvolutionData {};
    struct
    {
        UINT iterCount;
    } mDownsampleData {};

    void CreateRootSig(const RenderContext& ctx);
    void CreateDescriptorHeap(RenderContext& ctx);
    void CreateViews(RenderContext& ctx) const;
    void Convolute(RenderContext& ctx) const;
    void Downsample(RenderContext& ctx) const;

    TexResourceData mCubemapData;
    TexResourceData mEnvMapData;
    TexResourceData mIrradianceMapData;
    TexResourceData mDownsampledCubeData;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSig;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
    UploadBuffer* mDataBuffer = nullptr;
    UploadBuffer* mConvolutionDataBuffer = nullptr;
    UploadBuffer* mDownsampleDataBuffer = nullptr;
    const std::string mPsoName = "CubemapConvertor_PBR";
    const std::string mConvolutionPsoName = "CubemapConvolution_PBR";
    const std::string mDownsamplePsoName = "CubemapDownsample_PBR";

    bool mConverted = false;
    UINT mCubemapSize = 0;
    UINT mIrradianceMapSize = 0;
};

inline UINT EnvironmentMap::GetIrradianceMapIndex() const
{
    return mIrradianceMapData.SRVOffset;
}

inline UINT EnvironmentMap::GetCubemapIndex() const
{
    return mCubemapData.SRVOffset;
}

}
