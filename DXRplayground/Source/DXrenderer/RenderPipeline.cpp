#include "RenderPipeline.h"

#include "External/Dx12Helpers/d3dx12.h"
#include "External/IMGUI/imgui.h"
#include "External/IMGUI/imgui_impl_dx12.h"
#include "External/IMGUI/imgui_impl_win32.h"

#include "WindowsApp.h"

#include "DXrenderer/DXhelpers.h"
#include "DXrenderer/Textures/TextureManager.h"
#include "DXrenderer/PsoManager.h"
#include "DXrenderer/Shader.h"

#include "Scene/Scene.h"

#include "Utils/PixProfiler.h"
#include "Utils/Logger.h"

namespace DirectxPlayground
{
using Microsoft::WRL::ComPtr;

RenderPipeline::~RenderPipeline()
{
    SafeDelete(m_textureManager);
    SafeDelete(m_imguiTextureManager);
}

void RenderPipeline::Init(HWND hwnd, int width, int height, Scene* scene)
{
    PixProfiler::InitGpuProfiler(hwnd);
    m_context.Pipeline = this;
    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug1> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            //debugController->SetEnableGPUBasedValidation(true);
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
    AUTO_NAME_D3D12_OBJECT(m_commandQueue);

    m_swapChain.Init(m_isTearingSupported, hwnd, m_context, m_factory.Get(), m_device.Get(), m_commandQueue.Get());
    m_context.SwapChain = &m_swapChain;

    for (int i = 0; i < RenderContext::AllocatorsCount; ++i)
    {
        ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])));
        AUTO_NAME_D3D12_OBJECT(m_commandAllocators[i]);
    }
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_currentAllocatorIdx].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
    AUTO_NAME_D3D12_OBJECT(m_commandList);
    m_commandList->Close();

    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    AUTO_NAME_D3D12_OBJECT(m_fence);

    m_context.CbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_context.RtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_context.DsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_context.SamplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    Shader::InitCompiler();

    m_psoManager = new PsoManager();
    m_context.PsoManager = m_psoManager;

    m_textureManager = new TextureManager(m_context);
    m_context.TexManager = m_textureManager;

    Flush();
    Resize(width, height);

    InitImGui();

    IncrementAllocatorIndex();
    m_commandAllocators[m_currentAllocatorIdx]->Reset();
    m_commandList->Reset(m_commandAllocators[m_currentAllocatorIdx].Get(), nullptr);

    m_context.CommandList = m_commandList.Get();
    scene->InitResources(m_context);

    m_commandList->Close();
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    Flush(); // 3 flushes in a row...
}

void RenderPipeline::Flush()
{
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_currentFence));

    if (m_fence->GetCompletedValue() < m_currentFence)
    {
        HANDLE fenceEventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEventHandle == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFence, fenceEventHandle));

        WaitForSingleObjectEx(fenceEventHandle, INFINITE, false);
        CloseHandle(fenceEventHandle);
        ++m_currentFence;
    }
}

void RenderPipeline::ExecuteCommandList(ID3D12GraphicsCommandList* commandList)
{
    commandList->Close();
    ID3D12CommandList* cmdLists[] = { commandList };
    m_commandQueue->ExecuteCommandLists(1, cmdLists);
}

void RenderPipeline::ResetCommandList(ID3D12GraphicsCommandList* commandList)
{
    IncrementAllocatorIndex();
    m_commandAllocators[m_currentAllocatorIdx]->Reset();
    commandList->Reset(m_commandAllocators[m_currentAllocatorIdx].Get(), nullptr);
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

    IncrementAllocatorIndex();
    m_commandAllocators[m_currentAllocatorIdx]->Reset();
    ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_currentAllocatorIdx].Get(), nullptr));

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

    IncrementAllocatorIndex();
    // To BeginFrame. Direct approach won't work here.
    m_commandAllocators[m_currentAllocatorIdx]->Reset();
    m_commandList->Reset(m_commandAllocators[m_currentAllocatorIdx].Get(), nullptr);

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
    m_imguiTextureManager = new ImguiTextureManager(&m_context);

    ImGui::ImplDX12Init(m_context.Device, RenderContext::FramesCount, m_swapChain.GetBackBufferFormat(), m_imguiTextureManager->GetHeap()->GetCPUDescriptorHandleForHeapStart(),
        m_imguiTextureManager->GetHeap()->GetGPUDescriptorHandleForHeapStart());
    m_context.ImguiTexManager = m_imguiTextureManager;
}

void RenderPipeline::RenderImGui()
{
    m_context.CommandList->OMSetRenderTargets(1, &m_swapChain.GetCurrentBackBufferCPUhandle(m_context), false, &m_swapChain.GetDSCPUhandle());

    ID3D12DescriptorHeap* descHeap[] = { m_imguiTextureManager->GetHeap() };
    m_context.CommandList->SetDescriptorHeaps(1, descHeap);
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
