#include "Scene/DummyScene.h"

#include "DXrenderer/Swapchain.h"

#include "DXrenderer/Mesh.h"
#include "DXrenderer/Shader.h"

#include "DXrenderer/Buffers/UploadBuffer.h"

namespace DirectxPlayground
{

DummyScene::~DummyScene()
{
    if (m_mesh != nullptr)
        delete m_mesh;
    if (m_cameraCb != nullptr)
        delete m_cameraCb;
    if (m_camera != nullptr)
        delete m_camera;
    if (m_cameraController != nullptr)
        delete m_cameraController;
    if (m_objectCb != nullptr)
        delete m_objectCb;
    if (m_sphere != nullptr)
        delete m_sphere;
}

void DummyScene::InitResources(RenderContext& context)
{
    using Microsoft::WRL::ComPtr;

    m_camera = new Camera(1.0472f, 1.77864583f, 0.001f, 1000.0f);
    m_cameraCb = new UploadBuffer(*context.Device, sizeof(XMFLOAT4X4), true, context.FramesCount);
    m_objectCb = new UploadBuffer(*context.Device, sizeof(XMFLOAT4X4), true, context.FramesCount);
    m_cameraController = new CameraController(m_camera);

    std::vector<Vertex> verts;
    std::vector<UINT> ind;

    verts = {
        { { -0.5f, -0.5f, 3.0f }, {}, {} },
        { { 0.5f, 0.5f, 3.0f }, {}, {} },
        { { 0.5f, -0.5f, 3.0f }, {}, {} }
    };
    ind = { 0, 1, 2 };

    m_mesh = new Mesh(context, verts, ind);
    auto shperePath = std::string(ASSETS_DIR) + std::string("Models//Sphere.glb");
    m_sphere = new Mesh(context, shperePath);

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
    inputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    inputLayout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    inputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

    std::vector<CD3DX12_ROOT_PARAMETER1> cbParams;
    cbParams.emplace_back();
    cbParams.back().InitAsConstantBufferView(0, 0);
    cbParams.emplace_back();
    cbParams.back().InitAsConstantBufferView(1, 0);

    D3D12_FEATURE_DATA_ROOT_SIGNATURE signatureData = {};
    signatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(context.Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &signatureData, sizeof(signatureData))))
        signatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

    D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(UINT(cbParams.size()), cbParams.data(), 0, nullptr, flags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> rootSignatureCreationError;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, signatureData.HighestVersion, &signature, &rootSignatureCreationError);
    if (hr != S_OK)
        OutputDebugStringA(reinterpret_cast<char*>(rootSignatureCreationError->GetBufferPointer()));
    hr = context.Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_triangleRootSig));

    NAME_D3D12_OBJECT(m_triangleRootSig);

    auto shaderPath = std::string(ASSETS_DIR) + std::string("Shaders//Trig.hlsl");
    m_vs = Shader::CompileFromFile(shaderPath, "vs", "vs_5_1");
    m_ps = Shader::CompileFromFile(shaderPath, "ps", "ps_5_1");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = m_triangleRootSig.Get();
    desc.VS = m_vs.GetBytecode();
    desc.PS = m_ps.GetBytecode();
    desc.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    D3D12_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthEnable = false;
    dsDesc.StencilEnable = false;

    desc.DepthStencilState = dsDesc;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    desc.DSVFormat = context.SwapChain->GetDepthStencilFormat();
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = context.SwapChain->GetBackBufferFormat();

    desc.SampleMask = UINT_MAX;
    desc.SampleDesc.Count = 1;

    ThrowIfFailed(context.Device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pso)));
}

void DummyScene::Render(RenderContext& context)
{
    UINT frameIndex = context.SwapChain->GetCurrentBackBufferIndex();
    XMFLOAT4X4 vp;
    XMStoreFloat4x4(&vp, XMMatrixTranspose(m_camera->GetViewProjection()));
    XMFLOAT4X4 toWorld;
    XMStoreFloat4x4(&toWorld, XMMatrixTranspose(XMMatrixTranslation(0.0f, 0.0f, 3.0f)));
    m_cameraCb->UploadData(frameIndex, vp);
    m_objectCb->UploadData(frameIndex, toWorld);

    m_cameraController->Update();
    auto toRt = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    context.CommandList->ResourceBarrier(1, &toRt);

    auto rtCpuHandle = context.SwapChain->GetCurrentBackBufferCPUhandle(context);

    D3D12_RECT scissorRect = { 0, 0, LONG(context.Width), LONG(context.Height) };
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(context.Width);
    viewport.Height = static_cast<float>(context.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    context.CommandList->RSSetScissorRects(1, &scissorRect);
    context.CommandList->RSSetViewports(1, &viewport);

    context.CommandList->OMSetRenderTargets(1, &rtCpuHandle, false, nullptr);
    const float clearColor[] = { 0.0f, 0.4f, 0.9f, 1.0f };
    context.CommandList->ClearRenderTargetView(rtCpuHandle, clearColor, 0, nullptr);

    context.CommandList->SetPipelineState(m_pso.Get());
    context.CommandList->SetGraphicsRootSignature(m_triangleRootSig.Get());
    context.CommandList->SetGraphicsRootConstantBufferView(0, m_cameraCb->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(1, m_objectCb->GetFrameDataGpuAddress(frameIndex));

    context.CommandList->IASetVertexBuffers(0, 1, &m_sphere->GetVertexBufferView());
    context.CommandList->IASetIndexBuffer(&m_sphere->GetIndexBufferView());

    context.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context.CommandList->DrawIndexedInstanced(m_sphere->GetIndexCount(), 1, 0, 0, 0);

    auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    context.CommandList->ResourceBarrier(1, &toPresent);

}

}