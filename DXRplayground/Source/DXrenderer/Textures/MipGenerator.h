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

    void GenerateMips(const ID3D12Resource* resource);

private:
    void CreateUavHeap();
    void CreatePso();

    std::map<ID3D12Resource*, UINT> m_baseViewOffset;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_uavHeap;
    UINT m_currCount = 0;
    const std::string m_psoName = "Generate_Mips";
    UploadBuffer* m_constantBuffers = nullptr;
};
}