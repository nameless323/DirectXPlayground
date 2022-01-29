#pragma once

#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{
struct RenderContext;
class Model;
class UploadBuffer;

class Tonemapper
{
public:
    Tonemapper() = default;
    ~Tonemapper();

    void InitResources(RenderContext& ctx, ID3D12RootSignature* rootSig);
    void Render(RenderContext& ctx);

    DXGI_FORMAT GetHDRTargetFormat() const;
    UINT GetRtIndex() const;

    const float* GetClearColor() const;

private:
    struct TonemapperData
    {
        UINT HdrTexIndex = 0;
        float Exposure = 2.0f;
    };

    void CreateRenderTarget(RenderContext& ctx);
    void CreateGeometry(RenderContext& context);
    void CreatePSO(RenderContext& ctx, ID3D12RootSignature* rootSig);

    std::string mPsoName = "Tonemapper";
    Model* mModel = nullptr;
    TonemapperData mTonemapperData{};
    UINT mRtvOffset = 0;
    UINT mResourceIdx = 0;
    DXGI_FORMAT mRtFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    UploadBuffer* mHdrRtBuffer = nullptr;

    const float mClearColor[4] = { 0.001f, 0.001f, 0.001f, 1.0f };
};

inline DXGI_FORMAT Tonemapper::GetHDRTargetFormat() const
{
    return mRtFormat;
}

inline UINT Tonemapper::GetRtIndex() const
{
    return mRtvOffset;
}

inline const float* Tonemapper::GetClearColor() const
{
    return mClearColor;
}
}
