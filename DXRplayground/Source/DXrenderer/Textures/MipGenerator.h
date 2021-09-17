#pragma once

#include <map>

#include <wrl.h>
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{
struct RenderContext;
class UploadBuffer;

class MipGenerator
{
public:
    MipGenerator(RenderContext& ctx);

    void GenerateMips(RenderContext& ctx, ID3D12Resource* resource);
    void Flush(RenderContext& ctx);

private:
    struct MipGenData
    {
        UINT BaseMip = 0;
        UINT Width = 0;
        UINT Height = 0;
        UINT NumMips = 0;
    };
    static constexpr UINT m_maxResourcesInFrame = 256U;
    static constexpr UINT m_maxMipsPerInvocation = 4U;

    void CreateUavHeap(RenderContext& ctx);
    void CreatePso(RenderContext& ctx);
    UINT CreateMipViews(RenderContext& ctx, ID3D12Resource* resource);

    std::vector<MipGenData> m_queuedMips;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_uavHeap;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_commonRootSig;
    UINT m_currViewsCount = 0;
    UINT m_currentResourceInFrame = 0;
    const std::string m_psoName = "Generate_Mips";
    UploadBuffer* m_constantBuffers = nullptr;
};
}