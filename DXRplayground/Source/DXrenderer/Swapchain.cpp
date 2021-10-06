#include "Swapchain.h"

#include "DXrenderer/DXhelpers.h"

namespace DirectxPlayground
{
using Microsoft::WRL::ComPtr;

void Swapchain::Init(bool isTearingSupported, HWND hwnd, const RenderContext& ctx,
    IDXGIFactory7* factory, ID3D12Device* device, ID3D12CommandQueue* commandQueue)
{
    m_isTearingSupported = isTearingSupported;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = RenderContext::FramesCount;
    swapChainDesc.Width = ctx.Width;
    swapChainDesc.Height = ctx.Height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    swapChainDesc.Flags = m_isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChain;
    factory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &swapChain);

    if (m_isTearingSupported)
        factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    ThrowIfFailed(swapChain.As(&m_swapChain));

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = RenderContext::FramesCount;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
}


void Swapchain::Resize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const RenderContext& ctx)
{
    m_currentFrameIndex = 0;

    m_dsResource.GetWrlPtr().Reset();
    m_dsResource.SetInitialState(D3D12_RESOURCE_STATE_COMMON);
    for (auto& backBufferRes : m_backBufferResources)
        backBufferRes.GetWrlPtr().Reset();

    UINT flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    if (m_isTearingSupported)
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    ThrowIfFailed(m_swapChain->ResizeBuffers(RenderContext::FramesCount, ctx.Width, ctx.Height, m_backBufferFormat, flags));

    BOOL fullscreenState;
    ThrowIfFailed(m_swapChain->GetFullscreenState(&fullscreenState, nullptr));
    m_isSwapChainChainInFullScreen = fullscreenState;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < RenderContext::FramesCount; ++i)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_backBufferResources[i].GetAddressOf())));
        device->CreateRenderTargetView(m_backBufferResources[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(ctx.RtvDescriptorSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = ctx.Width;
    depthStencilDesc.Height = ctx.Height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE dsClear = {};
    dsClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsClear.DepthStencil.Depth = 1.0f;
    dsClear.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(&prop,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &dsClear,
        IID_PPV_ARGS(m_dsResource.GetAddressOf())));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsViewDesc = {};
    dsViewDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsViewDesc.Texture2D.MipSlice = 0;

    device->CreateDepthStencilView(m_dsResource.Get(), &dsViewDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    m_dsResource.Transition(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void Swapchain::Present()
{
    UINT presentFlags = (m_isTearingSupported && !m_isSwapChainChainInFullScreen) ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(m_swapChain->Present(0, presentFlags));
}

void Swapchain::ProceedToNextFrame()
{
    m_currentFrameIndex = (m_currentFrameIndex + 1) % RenderContext::FramesCount;
}

}