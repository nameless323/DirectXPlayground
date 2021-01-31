#include "RenderPipeline.h"

#include "d3dx12.h"
#include "DXrenderer/DXhelpers.h"

namespace DirectxPlayground
{
using Microsoft::WRL::ComPtr;

void RenderPipeline::Init(HWND hwnd, int width, int height)
{
    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

    BOOL allowTearing = FALSE;
    ComPtr<IDXGIFactory7> factory7;
    ThrowIfFailed(m_factory.As(&factory7));
    HRESULT hr = factory7->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
    m_isTearingSupported = SUCCEEDED(hr) && allowTearing;

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(m_factory.Get(), &hardwareAdapter);

    ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
    NAME_D3D12_OBJECT(m_commandQueue);

    m_swapChain.Init(m_isTearingSupported, hwnd, m_context, m_factory.Get(), m_device.Get(), m_commandQueue.Get());

    for (int i = 0; i < RenderContext::FramesCount; ++i)
    {
        ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])));
        NAME_D3D12_OBJECT(m_commandAllocators[i]);
    }
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
    NAME_D3D12_OBJECT(m_commandList);
    m_commandList->Close();

    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    NAME_D3D12_OBJECT(m_fence);

    m_context.CbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_context.RtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_context.DsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_context.SamplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    Flush();
    Resize(width, height);
}

void RenderPipeline::Flush()
{
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_swapChain.GetCurrentBackBufferIndex()]));

    if (m_fence->GetCompletedValue() < m_fenceValues[m_swapChain.GetCurrentBackBufferIndex()])
    {
        HANDLE fenceEventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEventHandle == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_swapChain.GetCurrentBackBufferIndex()], fenceEventHandle));

        WaitForSingleObjectEx(fenceEventHandle, INFINITE, false);
        CloseHandle(fenceEventHandle);
    }
}

void RenderPipeline::Resize(int width, int height)
{
    if (width == m_context.Width && height == m_context.Height)
        return;

    m_context.Width = width;
    m_context.Height = height;

    for (auto fenceVals : m_fenceValues)
        fenceVals = m_currentFence;

    ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_swapChain.GetCurrentBackBufferIndex()].Get(), nullptr));

    m_swapChain.Resize(m_device.Get(), m_commandList.Get(), m_context);

    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    Flush();
}

void RenderPipeline::Render()
{
    // To BeginFrame. Direct approach won't work here.
    m_commandAllocators[m_swapChain.GetCurrentBackBufferIndex()]->Reset();
    m_commandList->Reset(m_commandAllocators[m_swapChain.GetCurrentBackBufferIndex()].Get(), nullptr);

    auto toRt = CD3DX12_RESOURCE_BARRIER::Transition(m_swapChain.GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &toRt);

    auto rtCpuHandle = m_swapChain.GetCurrentBackBufferCPUhandle(m_context);
    m_commandList->OMSetRenderTargets(1, &rtCpuHandle, false, nullptr);
    const float clearColor[] = { 0.0f, 0.9f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtCpuHandle, clearColor, 0, nullptr);

    auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(m_swapChain.GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &toPresent);

    m_commandList->Close();
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    m_swapChain.Present();

    m_fenceValues[m_swapChain.GetCurrentBackBufferIndex()] = ++m_currentFence;
    m_commandQueue->Signal(m_fence.Get(), m_currentFence);

    m_swapChain.ProceedToNextFrame();

    if (m_fenceValues[m_swapChain.GetCurrentBackBufferIndex()] != 0 && m_fence->GetCompletedValue() < m_fenceValues[m_swapChain.GetCurrentBackBufferIndex()])
    {
        HANDLE fenceEventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEventHandle == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_swapChain.GetCurrentBackBufferIndex()], fenceEventHandle));

        WaitForSingleObjectEx(fenceEventHandle, INFINITE, false);
        CloseHandle(fenceEventHandle);
    }
}

void RenderPipeline::GetHardwareAdapter(IDXGIFactory7* factory, IDXGIAdapter1** adapter)
{
    *adapter = nullptr;
    ComPtr<IDXGIAdapter1> currAdapter;
    UINT deviceIndex = -1;
    UINT bestVendorIndex = -1;

    static const std::array<UINT, 3> preferredVendors = // http://pcidatabase.com/vendors.php?sort=id
    {
        0x10DE, // NVidia
        0x1002, // AMD
        0x8086 // Intel
    };

    for (UINT index = 0; factory->EnumAdapters1(index, &currAdapter) != DXGI_ERROR_NOT_FOUND; ++index)
    {
        DXGI_ADAPTER_DESC1 desc;
        currAdapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        if (SUCCEEDED(D3D12CreateDevice(currAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            for (UINT i = 0; i < preferredVendors.size(); ++i)
            {
                if (desc.VendorId == preferredVendors[i] && i < bestVendorIndex)
                {
                    bestVendorIndex = i;
                    deviceIndex = index;
                    break;
                }
            }
        }
    }
    if (deviceIndex != -1)
        factory->EnumAdapters1(deviceIndex, adapter);
}
}
