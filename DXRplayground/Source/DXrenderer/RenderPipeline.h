#pragma once

#include <array>
#include <cassert>
#include <exception>
#include <map>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

#include "DXrenderer/RenderContext.h"

#include "DXrenderer/Swapchain.h"

namespace DirectxPlayground
{
class Scene;

class ImguiTextureManager;

class IRenderPipeline
{
public:
    virtual ~IRenderPipeline() = default;

    virtual void Flush() = 0;
    virtual void ExecuteCommandList(ID3D12GraphicsCommandList* commandList) = 0;
    virtual void ResetCommandList(ID3D12GraphicsCommandList* commandList) = 0;
};

class RenderPipeline : public IRenderPipeline
{
public:
    ~RenderPipeline();

    void Init(HWND hwnd, int width, int height, Scene* scene);
    void Flush() override;
    void ExecuteCommandList(ID3D12GraphicsCommandList* commandList) override;
    void ResetCommandList(ID3D12GraphicsCommandList* commandList) override; // TODO: Nice way to mess things up, rethink
    void Resize(int width, int height);

    void Render(Scene* scene);

    void Shutdown();

private:
    void GetHardwareAdapter(IDXGIFactory7* factory, IDXGIAdapter1** adapter);

    void InitImGui();
    void RenderImGui();
    void ShutdownImGui();
    void IncrementAllocatorIndex();

    bool mIsTearingSupported = false;

    RenderContext mContext;

    Swapchain mSwapChain;
    TextureManager* mTextureManager = nullptr;
    PsoManager* mPsoManager = nullptr;

    ImguiTextureManager* mImguiTextureManager = nullptr;

    Microsoft::WRL::ComPtr<IDXGIFactory7> mFactory;
    Microsoft::WRL::ComPtr<ID3D12Device5> mDevice;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;

    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, RenderContext::AllocatorsCount> mCommandAllocators;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> mCommandList;
    UINT64 mFenceValues[RenderContext::FramesCount]{};
    UINT64 mCurrentFence = 0;
    UINT64 mCurrentAllocatorIdx = 0; // TODO: recheck allocators
};

inline void RenderPipeline::IncrementAllocatorIndex()
{
    mCurrentAllocatorIdx = (mCurrentAllocatorIdx + 1) % RenderContext::AllocatorsCount;
}
}