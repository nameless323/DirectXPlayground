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

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
    Model* m_model = nullptr;
    TonemapperData m_tonemapperData{};
    UINT m_rtvOffset = 0;
    UINT m_resourceIdx = 0;
    DXGI_FORMAT m_rtFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    UploadBuffer* m_hdrRtBuffer = nullptr;

    const float m_clearColor[4] = { 0.0f, 0.4f, 0.9f, 1.0f };
};

inline DXGI_FORMAT Tonemapper::GetHDRTargetFormat() const
{
    return m_rtFormat;
}

inline UINT Tonemapper::GetRtIndex() const
{
    return m_rtvOffset;
}

inline const float* Tonemapper::GetClearColor() const
{
    return m_clearColor;
}
}
