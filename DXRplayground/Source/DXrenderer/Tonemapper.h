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

private:
    void CreateRenderTarget(RenderContext& ctx);
    void CreateGeometry(RenderContext& context);
    void CreatePSO(RenderContext& ctx, ID3D12RootSignature* rootSig);

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
    Model* m_model = nullptr;
    UINT m_rtvOffset = 0;
    UINT m_srvOffset = 0;
    UINT m_resourceIdx = 0;
    DXGI_FORMAT m_rtFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    UploadBuffer* m_hdrRtBuffer = nullptr;
};

inline DXGI_FORMAT Tonemapper::GetHDRTargetFormat() const
{
    return m_rtFormat;
}

inline UINT Tonemapper::GetRtIndex() const
{
    return m_rtvOffset;
}
}
