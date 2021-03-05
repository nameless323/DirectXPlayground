#pragma once

#include <string>

#include <wrl.h>
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{
struct RenderContext;

class TextureManager
{
public:
    TextureManager(RenderContext& ctx);
    UINT CreateTexture(RenderContext& ctx, const std::string& filename);

    ID3D12DescriptorHeap* GetDescriptorHeap() const;

private:
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_resources;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_uploadResources;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap = nullptr;

    UINT m_currentTexCount = 0;
};

inline ID3D12DescriptorHeap* TextureManager::GetDescriptorHeap() const
{
    return m_srvHeap.Get();
}

}