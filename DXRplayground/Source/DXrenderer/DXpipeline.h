#pragma once

#include <array>
#include <exception>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

namespace DXRplayground
{
class DXpipeline
{
public:
    void Init(HWND hwnd, int width, int height);

private:
    void GetHardwareAdapter(IDXGIFactory7* factory, IDXGIAdapter1** adapter);

    inline static constexpr UINT FrameCount = 3;

    bool m_isTearingSupported = false;

    Microsoft::WRL::ComPtr<IDXGIFactory7> Factory;
    Microsoft::WRL::ComPtr<ID3D12Device> Device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue;

    Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FrameCount> CommandAllocators; // [a_vorontcov] For each render thread?
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
};
}