#pragma once

#include "DXrenderer/DXhelpers.h"
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{
class UnorderedAccessBuffer;

class EnvironmentMap
{
public:
    EnvironmentMap(RenderContext& ctx, const std::string& path, UINT width, UINT height);
    ~EnvironmentMap();

    void ConvertToCubemap(RenderContext& ctx);
    bool IsConvertedToCubemap() const;

private:
    UnorderedAccessBuffer* m_buffer = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_commonRootSig;
    const std::string m_psoName = "CubemapConvertor_PBR";
    bool m_converted = false;
};

}
