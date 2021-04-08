#include "Scene/GltfViewer.h"

#include <array>

#include "DXrenderer/Swapchain.h"

#include "DXrenderer/Model.h"
#include "DXrenderer/Shader.h"
#include "DXrenderer/TextureManager.h"
#include "DXrenderer/Tonemapper.h"
#include "DXrenderer/LightManager.h"

#include "DXrenderer/Buffers/UploadBuffer.h"

#include "External/IMGUI/imgui.h"

namespace DirectxPlayground
{

GltfViewer::~GltfViewer()
{
    SafeDelete(m_cameraCb);
    SafeDelete(m_camera);
    SafeDelete(m_cameraController);
    SafeDelete(m_objectCb);
    SafeDelete(m_gltfMesh);
    SafeDelete(m_tonemapper);
    SafeDelete(m_lightManager);
    SafeDelete(m_debugBuffer);
}

void GltfViewer::InitResources(RenderContext& context)
{
    using Microsoft::WRL::ComPtr;

    m_camera = new Camera(1.0472f, 1.77864583f, 0.001f, 1000.0f);
    m_cameraCb = new UploadBuffer(*context.Device, sizeof(CameraShaderData), true, context.FramesCount);
    m_objectCb = new UploadBuffer(*context.Device, sizeof(XMFLOAT4X4), true, context.FramesCount);
    m_debugBuffer = new UploadBuffer(*context.Device, UINT(m_debugData.size() * sizeof(float)), true, context.FramesCount);
    m_cameraController = new CameraController(m_camera);
    m_lightManager = new LightManager(context);
    Light l = { { 1.0f, 1.0f, 1.0f, 1.0f}, { 0.70710678f, -0.70710678f, 0.0f } };
    m_directionalLightInd = m_lightManager->AddLight(l);

    LoadGeometry(context);
    CreateRootSignature(context);

    m_tonemapper = new Tonemapper();
    m_tonemapper->InitResources(context, m_commonRootSig.Get());

    CreatePSOs(context);
}

void GltfViewer::Render(RenderContext& context)
{
    m_cameraController->Update();
    UpdateLights(context);
    UpdateDebugBuffer(context);

    UINT frameIndex = context.SwapChain->GetCurrentBackBufferIndex();

    XMFLOAT4X4 toWorld;
    XMStoreFloat4x4(&toWorld, XMMatrixTranspose(XMMatrixTranslation(0.0f, 0.0f, 3.0f)));
    m_cameraData.ViewProj = TransposeMatrix(m_camera->GetViewProjection());
    XMFLOAT4 camPos = m_camera->GetPosition();
    m_cameraData.Position = { camPos.x, camPos.y, camPos.z };
    m_cameraCb->UploadData(frameIndex, m_cameraData);
    m_objectCb->UploadData(frameIndex, toWorld);
    m_gltfMesh->UpdateMeshes(frameIndex);

    auto toRt = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    context.CommandList->ResourceBarrier(1, &toRt);

    auto rtCpuHandle = context.TexManager->GetRtHandle(context, m_tonemapper->GetRtIndex());

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
    context.CommandList->ClearRenderTargetView(rtCpuHandle, m_tonemapper->GetClearColor(), 0, nullptr);
    context.CommandList->ClearDepthStencilView(context.SwapChain->GetDSCPUhandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    context.CommandList->SetGraphicsRootSignature(m_commonRootSig.Get());
    context.CommandList->SetPipelineState(m_pso.Get());
    ID3D12DescriptorHeap* descHeap[] = { context.TexManager->GetDescriptorHeap() };
    context.CommandList->SetDescriptorHeaps(1, descHeap);
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(0), m_cameraCb->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(1), m_objectCb->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(3), m_lightManager->GetLightsBufferGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(0, 1), m_debugBuffer->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootDescriptorTable(TextureTableIndex, context.TexManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    for (const auto mesh : m_gltfMesh->GetMeshes())
    {
        context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(2), mesh->GetMaterialBufferGpuAddress(frameIndex));

        context.CommandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        context.CommandList->IASetIndexBuffer(&mesh->GetIndexBufferView());

        context.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context.CommandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
    }
    m_tonemapper->Render(context);

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
    CreateCommonRootSignature(context.Device, IID_PPV_ARGS(&m_commonRootSig));
    NAME_D3D12_OBJECT(m_commonRootSig);
}

void GltfViewer::CreatePSOs(RenderContext& context)
{
    auto& inputLayout = GetInputLayoutUV_N_T();

    auto shaderPath = std::string(ASSETS_DIR) + std::string("Shaders//BlinnPhong.hlsl");
    Shader vs = Shader::CompileFromFile(shaderPath, "vs", "vs_5_1");
    Shader ps = Shader::CompileFromFile(shaderPath, "ps", "ps_5_1");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = GetDefaultOpaquePsoDescriptor(m_commonRootSig.Get(), 1);
    desc.VS = vs.GetBytecode();
    desc.PS = ps.GetBytecode();
    desc.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };

    desc.DSVFormat = context.SwapChain->GetDepthStencilFormat();
    desc.RTVFormats[0] = m_tonemapper->GetHDRTargetFormat();

    ThrowIfFailed(context.Device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pso)));
}

void GltfViewer::UpdateLights(RenderContext& context)
{
    Light& dirLight = m_lightManager->GetLightRef(m_directionalLightInd);
    ImGui::Begin("Lights");
    ImGui::InputFloat4("Color", reinterpret_cast<float*>(&dirLight.Color));
    ImGui::InputFloat3("Direction", reinterpret_cast<float*>(&dirLight.Direction));
    ImGui::End();

    m_lightManager->UpdateLights(context.SwapChain->GetCurrentBackBufferIndex());
}

void GltfViewer::UpdateDebugBuffer(RenderContext& context)
{
    ImGui::Begin("Debug");
    ImGui::InputFloat("Debug0", &m_debugData[0]);
    ImGui::InputFloat("Debug1", &m_debugData[1]);
    ImGui::InputFloat("Debug2", &m_debugData[2]);
    ImGui::InputFloat("Debug3", &m_debugData[3]);
    ImGui::End();
    m_debugBuffer->UploadData(context.SwapChain->GetCurrentBackBufferIndex(), reinterpret_cast<byte*>(m_debugData.data()));
}

}