#include "RenderPipeline.h"

#include "External/Dx12Helpers/d3dx12.h"
#include "External/IMGUI/imgui.h"
#include "External/IMGUI/imgui_impl_dx12.h"
#include "External/IMGUI/imgui_impl_win32.h"

#include "WindowsApp.h"

#include "DXrenderer/DXhelpers.h"
#include "DXrenderer/TextureManager.h"
#include "DXrenderer/PsoManager.h"

#include "Scene/Scene.h"

#include "Utils/Logger.h"

namespace DirectxPlayground
{
using Microsoft::WRL::ComPtr;

RenderPipeline::~RenderPipeline()
{
    SafeDelete(m_textureManager);
}

void RenderPipeline::Init(HWND hwnd, int width, int height, Scene* scene)
{
    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug1> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            debugController->SetEnableGPUBasedValidation(true);
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
    m_context.Device = m_device.Get();

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
    NAME_D3D12_OBJECT(m_commandQueue);

    m_swapChain.Init(m_isTearingSupported, hwnd, m_context, m_factory.Get(), m_device.Get(), m_commandQueue.Get());
    m_context.SwapChain = &m_swapChain;

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

    m_textureManager = new TextureManager(m_context);
    m_context.TexManager = m_textureManager;

    m_psoManager = new PsoManager();
    m_context.PsoManager = m_psoManager;

    Flush();
    Resize(width, height);

    InitImGui();

    m_commandAllocators[m_swapChain.GetCurrentBackBufferIndex()]->Reset();
    m_commandList->Reset(m_commandAllocators[m_swapChain.GetCurrentBackBufferIndex()].Get(), nullptr);

    m_context.CommandList = m_commandList.Get();
    scene->InitResources(m_context);

    m_commandList->Close();
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    Flush(); // 3 flushes in a row...
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

    ImGui::ImplDX12InvalidateDeviceObjects();

    m_context.Width = width;
    m_context.Height = height;

    for (auto fenceVals : m_fenceValues)
        fenceVals = m_currentFence;

    ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_swapChain.GetCurrentBackBufferIndex()].Get(), nullptr));

    m_swapChain.Resize(m_device.Get(), m_commandList.Get(), m_context);

    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    ImGui::ImplDX12CreateDeviceObjects();

    Flush();
}

void RenderPipeline::Render(Scene* scene)
{
    ImGui::ImplDX12NewFrame();
    ImGui::ImplWinNewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Stats", NULL, ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::Text("Avg %.3f ms/F (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    // To BeginFrame. Direct approach won't work here.
    m_commandAllocators[m_swapChain.GetCurrentBackBufferIndex()]->Reset();
    m_commandList->Reset(m_commandAllocators[m_swapChain.GetCurrentBackBufferIndex()].Get(), nullptr);

    m_context.CommandList = m_commandList.Get();

    m_context.PsoManager->BeginFrame(m_context);

    scene->Render(m_context);

    ImguiLogger::Logger.Draw("Logger");

    // Redundant but for this playground - ok
    auto toRt = CD3DX12_RESOURCE_BARRIER::Transition(m_swapChain.GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &toRt);

    RenderImGui();

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

void RenderPipeline::Shutdown()
{
    m_psoManager->Shutdown();
    SafeDelete(m_textureManager); // To dtor to safely delete stuff
    SafeDelete(m_psoManager);
    if (m_context.Device != nullptr)
        Flush();

    ShutdownImGui();
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

void RenderPipeline::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::ImplWinInit(WindowsApp::GetHWND());
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        m_context.Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_imguiDescriptorHeap));
    }

    ImGui::ImplDX12Init(m_context.Device, RenderContext::FramesCount, m_swapChain.GetBackBufferFormat(), m_imguiDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        m_imguiDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void RenderPipeline::RenderImGui()
{
    m_context.CommandList->OMSetRenderTargets(1, &m_swapChain.GetCurrentBackBufferCPUhandle(m_context), false, &m_swapChain.GetDSCPUhandle());
    m_context.CommandList->SetDescriptorHeaps(1, &m_imguiDescriptorHeap);
    ImGui::Render();
    ImGui::ImplDX12RenderDrawData(ImGui::GetDrawData(), m_context.CommandList);
}

void RenderPipeline::ShutdownImGui()
{
    ImGui::ImplDX12Shutdown();
    ImGui::ImplWinShutdown();
    ImGui::DestroyContext();
}

}
