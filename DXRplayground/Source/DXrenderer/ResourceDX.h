#pragma once

#include "External/Dx12Helpers/d3dx12.h"
#include "DXrenderer/DXhelpers.h"

namespace DirectxPlayground
{
class ResourceDX
{
public:
    ResourceDX() = default;
    ResourceDX(D3D12_RESOURCE_STATES initialState);
    ~ResourceDX() = default;

    ID3D12Resource* Get() const;
    ID3D12Resource* Get();
    ID3D12Resource** GetAddressOf();

    void SetName(const std::string& name);
    void SetName(const std::wstring& name);

    void SetInitialState(D3D12_RESOURCE_STATES state);
    D3D12_RESOURCE_STATES GetCurrentState() const;
    bool GetBarrier(D3D12_RESOURCE_STATES after, CD3DX12_RESOURCE_BARRIER& transition);
    CD3DX12_RESOURCE_BARRIER GetBarrier(D3D12_RESOURCE_STATES after);
    void Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES after);

    Microsoft::WRL::ComPtr<ID3D12Resource>& GetWrlPtr();

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource = nullptr;
    D3D12_RESOURCE_STATES m_state = D3D12_RESOURCE_STATE_GENERIC_READ;
    bool m_initialStateSet = false;
};

inline ResourceDX::ResourceDX(D3D12_RESOURCE_STATES initialState) : m_state(initialState), m_initialStateSet(true)
{}

inline ID3D12Resource* ResourceDX::Get() const
{
    return m_resource.Get();
}

inline ID3D12Resource* ResourceDX::Get()
{
    return m_resource.Get();
}

inline ID3D12Resource** ResourceDX::GetAddressOf()
{
    return m_resource.GetAddressOf();
}

inline void ResourceDX::SetName(const std::string& name)
{
    std::wstring n{ name.begin(), name.end() };
    SetName(n);
}

inline void ResourceDX::SetName(const std::wstring& name)
{
    SetDXobjectName(Get(), name.c_str());
}

inline void ResourceDX::SetInitialState(D3D12_RESOURCE_STATES state)
{
    m_state = state;
    m_initialStateSet = true;
}

inline D3D12_RESOURCE_STATES ResourceDX::GetCurrentState() const
{
    assert(m_initialStateSet);
    return m_state;
}

inline bool ResourceDX::GetBarrier(D3D12_RESOURCE_STATES after, CD3DX12_RESOURCE_BARRIER& transition)
{
    assert(m_initialStateSet);
    if (after == m_state)
        return false;
    transition = CD3DX12_RESOURCE_BARRIER::Transition(Get(), m_state, after);
    m_state = after;
    return true;
}

inline CD3DX12_RESOURCE_BARRIER ResourceDX::GetBarrier(D3D12_RESOURCE_STATES after)
{
    assert(m_initialStateSet);
    assert(m_state != after);
    D3D12_RESOURCE_STATES before = m_state;
    m_state = after;
    return CD3DX12_RESOURCE_BARRIER::Transition(Get(), before, after);
}

inline void ResourceDX::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES after)
{
    assert(m_initialStateSet);
    assert(after != m_state);
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(Get(), m_state, after);
    m_state = after;
    cmdList->ResourceBarrier(1, &barrier);
}

inline Microsoft::WRL::ComPtr<ID3D12Resource>& ResourceDX::GetWrlPtr()
{
    return m_resource;
}

}