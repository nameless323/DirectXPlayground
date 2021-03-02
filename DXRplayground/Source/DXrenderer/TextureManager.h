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
    void CreateTexture(RenderContext& ctx, const std::string& filename);

    ID3D12DescriptorHeap* GetDescriptorHeap() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadResource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap = nullptr;
};

inline ID3D12DescriptorHeap* TextureManager::GetDescriptorHeap() const
{
    return m_srvHeap.Get();
}

}