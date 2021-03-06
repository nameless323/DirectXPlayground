#include "Scene/GltfViewer.h"

#include <array>

#include "DXrenderer/Swapchain.h"

#include "DXrenderer/Model.h"
#include "DXrenderer/Shader.h"
#include "DXrenderer/TextureManager.h"

#include "DXrenderer/Buffers/UploadBuffer.h"

namespace DirectxPlayground
{

GltfViewer::~GltfViewer()
{
    SafeDelete(m_cameraCb);
    SafeDelete(m_camera);
    SafeDelete(m_cameraController);
    SafeDelete(m_objectCb);
    SafeDelete(m_gltfMesh);
}

void GltfViewer::InitResources(RenderContext& context)
{
    using Microsoft::WRL::ComPtr;

    m_camera = new Camera(1.0472f, 1.77864583f, 0.001f, 1000.0f);
    m_cameraCb = new UploadBuffer(*context.Device, sizeof(XMFLOAT4X4), true, context.FramesCount);
    m_objectCb = new UploadBuffer(*context.Device, sizeof(XMFLOAT4X4), true, context.FramesCount);
    m_cameraController = new CameraController(m_camera);

    LoadGeometry(context);
    CreateRootSignature(context);
    CreatePSOs(context);
}

void GltfViewer::Render(RenderContext& context)
{
    m_cameraController->Update();

    UINT frameIndex = context.SwapChain->GetCurrentBackBufferIndex();
    XMFLOAT4X4 toWorld;
    XMStoreFloat4x4(&toWorld, XMMatrixTranspose(XMMatrixTranslation(0.0f, 0.0f, 3.0f)));
    m_cameraCb->UploadData(frameIndex, TransposeMatrix(m_camera->GetViewProjection()));
    m_objectCb->UploadData(frameIndex, toWorld);
    m_gltfMesh->UpdateMeshes(frameIndex);

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

    context.CommandList->OMSetRenderTargets(1, &rtCpuHandle, false, &context.SwapChain->GetDSCPUhandle());
    const float clearColor[] = { 0.0f, 0.4f, 0.9f, 1.0f };
    context.CommandList->ClearRenderTargetView(rtCpuHandle, clearColor, 0, nullptr);
    context.CommandList->ClearDepthStencilView(context.SwapChain->GetDSCPUhandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    context.CommandList->SetPipelineState(m_pso.Get());
    context.CommandList->SetGraphicsRootSignature(m_commonRootSig.Get());
    ID3D12DescriptorHeap* descHeap[] = { context.TexManager->GetDescriptorHeap() };
    context.CommandList->SetDescriptorHeaps(1, descHeap);
    context.CommandList->SetGraphicsRootConstantBufferView(0, m_cameraCb->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(1, m_objectCb->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootDescriptorTable(3, context.TexManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    for (const auto mesh : m_gltfMesh->GetMeshes())
    {
        context.CommandList->SetGraphicsRootConstantBufferView(2, mesh->GetMaterialBufferGpuAddress(frameIndex));

        context.CommandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        context.CommandList->IASetIndexBuffer(&mesh->GetIndexBufferView());

        context.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context.CommandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
    }

    auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    context.CommandList->ResourceBarrier(1, &toPresent);
}

void GltfViewer::LoadGeometry(RenderContext& context)
{
    auto path = std::string(ASSETS_DIR) + std::string("Models//FlightHelmet//glTF//FlightHelmet.gltf");
    m_gltfMesh = new Model(context, path);
}

void GltfViewer::CreateRootSignature(RenderContext& context)
{
    std::vector<CD3DX12_ROOT_PARAMETER1> cbParams;
    cbParams.emplace_back();
    cbParams.back().InitAsConstantBufferView(0, 0);
    cbParams.emplace_back();
    cbParams.back().InitAsConstantBufferView(1, 0);
    cbParams.emplace_back();
    cbParams.back().InitAsConstantBufferView(2, 0);

    D3D12_DESCRIPTOR_RANGE1 texRange;
    texRange.NumDescriptors = RenderContext::MaxTextures;
    texRange.BaseShaderRegister = 0;
    texRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    texRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
    texRange.RegisterSpace = 0;
    texRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    cbParams.emplace_back();
    cbParams.back().InitAsDescriptorTable(1, &texRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        1,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    std::array<CD3DX12_STATIC_SAMPLER_DESC, 2> staticSamplers = { linearClamp, linearWrap };

    D3D12_FEATURE_DATA_ROOT_SIGNATURE signatureData = {};
    signatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(context.Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &signatureData, sizeof(signatureData))))
        signatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

    D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(UINT(cbParams.size()), cbParams.data(), UINT(staticSamplers.size()), staticSamplers.data(), flags);

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureCreationError;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, signatureData.HighestVersion, &signature, &rootSignatureCreationError);
    if (hr != S_OK)
        OutputDebugStringA(reinterpret_cast<char*>(rootSignatureCreationError->GetBufferPointer()));
    hr = context.Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_commonRootSig));

    NAME_D3D12_OBJECT(m_commonRootSig);
}

void GltfViewer::CreatePSOs(RenderContext& context)
{
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
    inputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    inputLayout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    inputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
    inputLayout.push_back({ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });


    auto shaderPath = std::string(ASSETS_DIR) + std::string("Shaders//Unlit.hlsl");
    Shader vs = Shader::CompileFromFile(shaderPath, "vs", "vs_5_1");
    Shader ps = Shader::CompileFromFile(shaderPath, "ps", "ps_5_1");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = m_commonRootSig.Get();
    desc.VS = vs.GetBytecode();
    desc.PS = ps.GetBytecode();
    desc.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    D3D12_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthEnable = true;
    dsDesc.StencilEnable = false;
    dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    desc.DepthStencilState = dsDesc;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    desc.DSVFormat = context.SwapChain->GetDepthStencilFormat();
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = context.SwapChain->GetBackBufferFormat();

    desc.SampleMask = UINT_MAX;
    desc.SampleDesc.Count = 1;

    ThrowIfFailed(context.Device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pso)));
}
}