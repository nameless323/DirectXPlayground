#include "DXrenderer/Tonemapper.h"

#include "External/IMGUI/imgui.h"

#include "DXrenderer/DXhelpers.h"
#include "DXrenderer/RenderContext.h"
#include "DXrenderer/Shader.h"
#include "DXrenderer/Swapchain.h"
#include "DXrenderer/TextureManager.h"
#include "DXrenderer/PsoManager.h"
#include "Model.h"

namespace DirectxPlayground
{

Tonemapper::~Tonemapper()
{
    SafeDelete(m_model);
    SafeDelete(m_hdrRtBuffer);
}

void Tonemapper::InitResources(RenderContext& ctx, ID3D12RootSignature* rootSig)
{
    CreateRenderTarget(ctx);
    m_hdrRtBuffer = new UploadBuffer(*ctx.Device, sizeof(TonemapperData), true, RenderContext::FramesCount);
    CreateGeometry(ctx);
    CreatePSO(ctx, rootSig);
}

void Tonemapper::Render(RenderContext& ctx)
{
    ImGui::Begin("Tonemapping");
    ImGui::SliderFloat("Exposure", &m_tonemapperData.Exposure, 0.0f, 10.0f);
    ImGui::End();

    UINT frameIndex = ctx.SwapChain->GetCurrentBackBufferIndex();
    m_hdrRtBuffer->UploadData(frameIndex, m_tonemapperData);

    ID3D12Resource* hdrTex = ctx.TexManager->GetResource(m_resourceIdx);
    auto toPSResource = CD3DX12_RESOURCE_BARRIER::Transition(hdrTex, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    ctx.CommandList->ResourceBarrier(1, &toPSResource);

    D3D12_RECT scissorRect = { 0, 0, LONG(ctx.Width), LONG(ctx.Height) };
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(ctx.Width);
    viewport.Height = static_cast<float>(ctx.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    ctx.CommandList->RSSetScissorRects(1, &scissorRect);
    ctx.CommandList->RSSetViewports(1, &viewport);

    ctx.CommandList->OMSetRenderTargets(1, &ctx.SwapChain->GetCurrentBackBufferCPUhandle(ctx), false, nullptr);

    ctx.CommandList->SetPipelineState(ctx.PsoManager->GetPso(m_psoName));
    ctx.CommandList->SetGraphicsRootConstantBufferView(0, m_hdrRtBuffer->GetFrameDataGpuAddress(frameIndex));

    ctx.CommandList->IASetVertexBuffers(0, 1, &m_model->GetVertexBufferView());
    ctx.CommandList->IASetIndexBuffer(&m_model->GetIndexBufferView());

    ctx.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx.CommandList->DrawIndexedInstanced(m_model->GetIndexCount(), 1, 0, 0, 0);

    auto toRt = CD3DX12_RESOURCE_BARRIER::Transition(hdrTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    ctx.CommandList->ResourceBarrier(1, &toRt);
}

void Tonemapper::CreateRenderTarget(RenderContext& ctx)
{
    D3D12_RESOURCE_DESC desc{};
    desc.MipLevels = 1;
    desc.Format = m_rtFormat;
    desc.Width = ctx.Width;
    desc.Height = ctx.Height;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    desc.DepthOrArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Color[0] = m_clearColor[0];
    clearValue.Color[1] = m_clearColor[1];
    clearValue.Color[2] = m_clearColor[2];
    clearValue.Color[3] = m_clearColor[3];
    clearValue.Format = m_rtFormat;

    RtvSrvResourceIdx p = ctx.TexManager->CreateRT(ctx, desc, L"HDRTexture", &clearValue);
    m_rtvOffset = p.RTVOffset;
    m_resourceIdx = p.ResourceIdx;
    m_tonemapperData.HdrTexIndex = p.SRVOffset;
}

void Tonemapper::CreateGeometry(RenderContext& context)
{
    std::vector<Vertex> verts;
    verts.resize(4);
    verts[0].Pos = { -1.0f, 1.0f, 0.0f };
    verts[1].Pos = { 1.0f, 1.0f, 0.0f };
    verts[2].Pos = { 1.0f, -1.0f, 0.0f };
    verts[3].Pos = { -1.0f, -1.0f, 0.0f };
    std::vector<UINT> ind = { 0, 1, 2, 0, 2, 3};
    m_model = new Model(context, verts, ind);
}

void Tonemapper::CreatePSO(RenderContext& ctx, ID3D12RootSignature* rootSig)
{
    auto& inputLayout = GetInputLayoutUV_N_T();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = rootSig;

    desc.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    D3D12_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthEnable = false;
    dsDesc.StencilEnable = false;
    dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    desc.DepthStencilState = dsDesc;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = ctx.SwapChain->GetBackBufferFormat();

    desc.SampleMask = UINT_MAX;
    desc.SampleDesc.Count = 1;

    auto shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//Tonemapper.hlsl");
    ctx.PsoManager->CreatePso(ctx, m_psoName, shaderPath, desc);
}

}