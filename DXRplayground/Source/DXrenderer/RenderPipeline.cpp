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
    SafeDelete(mTextureManager);
    SafeDelete(mImguiTextureManager);
}

void RenderPipeline::Init(HWND hwnd, int width, int height, Scene* scene)
{
    PixProfiler::InitGpuProfiler(hwnd);
    mContext.Pipeline = this;
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
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));

    BOOL allowTearing = FALSE;
    ComPtr<IDXGIFactory7> factory7;
    ThrowIfFailed(mFactory.As(&factory7));
    HRESULT hr = factory7->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
    mIsTearingSupported = SUCCEEDED(hr) && allowTearing;

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(mFactory.Get(), &hardwareAdapter);

    ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&mDevice)));
    mContext.Device = mDevice.Get();

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
    NAME_D3D12_OBJECT(mCommandQueue, L"main_cmd_queue");

    mSwapChain.Init(mIsTearingSupported, hwnd, mContext, mFactory.Get(), mDevice.Get(), mCommandQueue.Get());
    mContext.SwapChain = &mSwapChain;

    for (int i = 0; i < RenderContext::AllocatorsCount; ++i)
    {
        ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocators[i])));
        std::wstring name = L"command_allocator_" + std::to_wstring(i);
        NAME_D3D12_OBJECT(mCommandAllocators[i], name.c_str());
    }
    ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators[mCurrentAllocatorIdx].Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
    NAME_D3D12_OBJECT(mCommandList, L"main_command_list");
    mCommandList->Close();

    ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    NAME_D3D12_OBJECT(mFence, L"frame_fence");

    mContext.CbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    mContext.RtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mContext.DsvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mContext.SamplerDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    Shader::InitCompiler();

    mPsoManager = new PsoManager();
    mContext.PsoManager = mPsoManager;

    mTextureManager = new TextureManager(mContext);
    mContext.TexManager = mTextureManager;

    Flush();
    Resize(width, height);

    InitImGui();

    IncrementAllocatorIndex();
    mCommandAllocators[mCurrentAllocatorIdx]->Reset();
    mCommandList->Reset(mCommandAllocators[mCurrentAllocatorIdx].Get(), nullptr);

    mContext.CommandList = mCommandList.Get();
    scene->InitResources(mContext);

    mCommandList->Close();
    ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    Flush(); // 3 flushes in a row...
}

void RenderPipeline::Flush()
{
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

    if (mFence->GetCompletedValue() < mCurrentFence)
    {
        HANDLE fenceEventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEventHandle == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, fenceEventHandle));

        WaitForSingleObjectEx(fenceEventHandle, INFINITE, false);
        CloseHandle(fenceEventHandle);
        ++mCurrentFence;
    }
}

void RenderPipeline::ExecuteAndFlushCmdList(ID3D12GraphicsCommandList* commandList)
{
    ExecuteCommandList(commandList);
    ResetCommandList(commandList);
    Flush();
}

void RenderPipeline::ExecuteCommandList(ID3D12GraphicsCommandList* commandList)
{
    commandList->Close();
    ID3D12CommandList* cmdLists[] = { commandList };
    mCommandQueue->ExecuteCommandLists(1, cmdLists);
}

void RenderPipeline::ResetCommandList(ID3D12GraphicsCommandList* commandList)
{
    IncrementAllocatorIndex();
    mCommandAllocators[mCurrentAllocatorIdx]->Reset();
    commandList->Reset(mCommandAllocators[mCurrentAllocatorIdx].Get(), nullptr);
}

void RenderPipeline::Resize(int width, int height)
{
    if (width == mContext.Width && height == mContext.Height)
        return;

    ImGui::ImplDX12InvalidateDeviceObjects();

    mContext.Width = width;
    mContext.Height = height;

    for (auto fenceVals : mFenceValues)
        fenceVals = mCurrentFence;

    IncrementAllocatorIndex();
    mCommandAllocators[mCurrentAllocatorIdx]->Reset();
    ThrowIfFailed(mCommandList->Reset(mCommandAllocators[mCurrentAllocatorIdx].Get(), nullptr));

    mSwapChain.Resize(mDevice.Get(), mCommandList.Get(), mContext);

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

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
    mCommandAllocators[mCurrentAllocatorIdx]->Reset();
    mCommandList->Reset(mCommandAllocators[mCurrentAllocatorIdx].Get(), nullptr);

    mContext.CommandList = mCommandList.Get();

    mContext.PsoManager->BeginFrame(mContext);

    scene->Render(mContext);

    ImguiLogger::Logger.Draw("Logger");

    // Redundant but for this playground - ok
    auto toRt = CD3DX12_RESOURCE_BARRIER::Transition(mSwapChain.GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &toRt);

    RenderImGui();

    auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(mSwapChain.GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &toPresent);

    mCommandList->Close();
    ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    mSwapChain.Present();

    mFenceValues[mSwapChain.GetCurrentBackBufferIndex()] = ++mCurrentFence;
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);

    mSwapChain.ProceedToNextFrame();

    if (mFenceValues[mSwapChain.GetCurrentBackBufferIndex()] != 0 && mFence->GetCompletedValue() < mFenceValues[mSwapChain.GetCurrentBackBufferIndex()])
    {
        HANDLE fenceEventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEventHandle == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValues[mSwapChain.GetCurrentBackBufferIndex()], fenceEventHandle));

        WaitForSingleObjectEx(fenceEventHandle, INFINITE, false);
        CloseHandle(fenceEventHandle);
    }
}

void RenderPipeline::Shutdown()
{
    mPsoManager->Shutdown();
    SafeDelete(mTextureManager); // To dtor to safely delete stuff
    SafeDelete(mPsoManager);
    if (mContext.Device != nullptr)
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
    mImguiTextureManager = new ImguiTextureManager(&mContext);

    ImGui::ImplDX12Init(mContext.Device, RenderContext::FramesCount, mSwapChain.GetBackBufferFormat(), mImguiTextureManager->GetHeap()->GetCPUDescriptorHandleForHeapStart(),
        mImguiTextureManager->GetHeap()->GetGPUDescriptorHandleForHeapStart());
    mContext.ImguiTexManager = mImguiTextureManager;
}

void RenderPipeline::RenderImGui()
{
    GPU_SCOPED_EVENT(mContext, "ImGui");
    D3D12_CPU_DESCRIPTOR_HANDLE h = mSwapChain.GetDSCPUhandle();
    auto handle = mSwapChain.GetCurrentBackBufferCPUhandle(mContext);
    mContext.CommandList->OMSetRenderTargets(1, &handle, false, &h);

    ID3D12DescriptorHeap* descHeap[] = { mImguiTextureManager->GetHeap() };
    mContext.CommandList->SetDescriptorHeaps(1, descHeap);
    ImGui::Render();
    ImGui::ImplDX12RenderDrawData(ImGui::GetDrawData(), mContext.CommandList);
}

void RenderPipeline::ShutdownImGui()
{
    ImGui::ImplDX12Shutdown();
    ImGui::ImplWinShutdown();
    ImGui::DestroyContext();
}
}
