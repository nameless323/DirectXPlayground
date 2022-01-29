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
    Microsoft::WRL::ComPtr<ID3D12Resource> mResource = nullptr;
    D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_GENERIC_READ;
    bool mInitialStateSet = false;
};

inline ResourceDX::ResourceDX(D3D12_RESOURCE_STATES initialState) : mState(initialState), mInitialStateSet(true)
{}

inline ID3D12Resource* ResourceDX::Get() const
{
    return mResource.Get();
}

inline ID3D12Resource* ResourceDX::Get()
{
    return mResource.Get();
}

inline ID3D12Resource** ResourceDX::GetAddressOf()
{
    return mResource.GetAddressOf();
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
    mState = state;
    mInitialStateSet = true;
}

inline D3D12_RESOURCE_STATES ResourceDX::GetCurrentState() const
{
    assert(mInitialStateSet);
    return mState;
}

inline bool ResourceDX::GetBarrier(D3D12_RESOURCE_STATES after, CD3DX12_RESOURCE_BARRIER& transition)
{
    assert(mInitialStateSet);
    if (after == mState)
        return false;
    transition = CD3DX12_RESOURCE_BARRIER::Transition(Get(), mState, after);
    mState = after;
    return true;
}

inline CD3DX12_RESOURCE_BARRIER ResourceDX::GetBarrier(D3D12_RESOURCE_STATES after)
{
    assert(mInitialStateSet);
    assert(mState != after);
    D3D12_RESOURCE_STATES before = mState;
    mState = after;
    return CD3DX12_RESOURCE_BARRIER::Transition(Get(), before, after);
}

inline void ResourceDX::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES after)
{
    assert(mInitialStateSet);
    assert(after != mState);
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(Get(), mState, after);
    mState = after;
    cmdList->ResourceBarrier(1, &barrier);
}

inline Microsoft::WRL::ComPtr<ID3D12Resource>& ResourceDX::GetWrlPtr()
{
    return mResource;
}

}