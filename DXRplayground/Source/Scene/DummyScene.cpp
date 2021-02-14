#include "Scene/DummyScene.h"

#include <d3d12.h>
#include "External/Dx12Helpers/d3dx12.h"
#include "DXrenderer/Swapchain.h"

namespace DirectxPlayground
{

void DummyScene::InitResources(RenderContext& context)
{

}

void DummyScene::Render(RenderContext& context)
{
    auto toRt = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    context.CommandList->ResourceBarrier(1, &toRt);

    auto rtCpuHandle = context.SwapChain->GetCurrentBackBufferCPUhandle(context);
    context.CommandList->OMSetRenderTargets(1, &rtCpuHandle, false, nullptr);
    const float clearColor[] = { 1.0f, 0.9f, 0.4f, 1.0f };
    context.CommandList->ClearRenderTargetView(rtCpuHandle, clearColor, 0, nullptr);

    auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    context.CommandList->ResourceBarrier(1, &toPresent);

}

}