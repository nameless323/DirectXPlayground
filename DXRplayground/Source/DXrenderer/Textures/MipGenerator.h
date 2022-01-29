#pragma once

#include <map>

#include <wrl.h>
#include "DXrenderer/ResourceDX.h"
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{
struct RenderContext;
class UploadBuffer;

class MipGenerator
{
public:
    MipGenerator(RenderContext& ctx);

    void GenerateMips(RenderContext& ctx, ResourceDX* resource);
    void Flush(RenderContext& ctx);

private:
    struct MipGenData
    {
        UINT BaseMip = 0;
        UINT Width = 0;
        UINT Height = 0;
    };
    static constexpr UINT m_maxTextureMipsInFrame = 1024U;

    void CreateUavHeap(RenderContext& ctx);
    void CreatePso(RenderContext& ctx);
    UINT CreateMipViews(RenderContext& ctx, ResourceDX* resource);

    std::vector<MipGenData> mQueuedMips;
    std::vector<ResourceDX*> mResourcesPtrs;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mUavHeap;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mCommonRootSig;
    UINT mCurrViewsCount = 0;
    UINT mQueuedMipsToGenerateNumber = 0;
    const std::string mPsoName = "Generate_Mips";
    UploadBuffer* mConstantBuffers = nullptr;
};
}