#include "Scene/RtTester.h"

#include <array>

#include "DXrenderer/Swapchain.h"

#include "DXrenderer/Model.h"
#include "DXrenderer/Shader.h"
#include "DXrenderer/PsoManager.h"
#include "DXrenderer/Textures/TextureManager.h"
#include "DXrenderer/Tonemapper.h"
#include "DXrenderer/LightManager.h"

#include "DXrenderer/Buffers/UploadBuffer.h"
#include "DXrenderer/Textures/EnvironmentMap.h"

#include "External/IMGUI/imgui.h"

namespace DirectxPlayground
{

RtTester::~RtTester()
{
    SafeDelete(m_cameraCb);
    SafeDelete(m_camera);
    SafeDelete(m_cameraController);
    SafeDelete(m_objectCb);
    SafeDelete(m_gltfMesh);
    SafeDelete(m_tonemapper);
    SafeDelete(m_lightManager);
    SafeDelete(m_envMap);
    SafeDelete(m_floor);
    SafeDelete(m_floorMaterialCb);
    SafeDelete(m_floorTransformCb);
}

void RtTester::InitResources(RenderContext& context)
{
    using Microsoft::WRL::ComPtr;

    m_camera = new Camera(1.0472f, 1.77864583f, 0.001f, 1000.0f);
    m_cameraCb = new UploadBuffer(*context.Device, sizeof(CameraShaderData), true, context.FramesCount);
    m_objectCb = new UploadBuffer(*context.Device, sizeof(XMFLOAT4X4), true, 1);

    XMFLOAT4X4 toWorld;
    XMStoreFloat4x4(&toWorld, XMMatrixTranspose(XMMatrixTranslation(0.0f, 0.0f, 3.0f)));
    m_objectCb->UploadData(0, toWorld);

    m_floorTransformCb = new UploadBuffer(*context.Device, sizeof(XMFLOAT4X4), true, context.FramesCount);
    XMStoreFloat4x4(&toWorld, XMMatrixTranspose(XMMatrixTranslation(0.0f, 0.0f, 0.0f)));
    for (UINT i = 0; i < context.FramesCount; ++i)
        m_floorTransformCb->UploadData(i, toWorld);

    m_floorMaterialCb = new UploadBuffer(*context.Device, sizeof(NonTexturedMaterial), true, context.FramesCount);
    m_floorMaterial.Albedo = { 0.8f, 0.8f, 0.8f, 1.0f };
    m_floorMaterial.AO = 1.0f;
    m_floorMaterial.Metallic = 0.0f;
    m_floorMaterial.Roughness = 1.0f;
    for (UINT i = 0; i < context.FramesCount; ++i)
        m_floorMaterialCb->UploadData(i, m_floorMaterial);

    m_cameraController = new CameraController(m_camera);
    m_lightManager = new LightManager(context);

    auto path = ASSETS_DIR + std::string("Textures//colorful_studio_4k.hdr");
    m_envMap = new EnvironmentMap(context, path, 512, 512);
    Light l = { { 300.0f, 300.0f, 300.0f, 1.0f}, { 0.0f, 5.70710678118f, 0.0f } };
    m_directionalLightInd = m_lightManager->AddLight(l);

    LoadGeometry(context);
    CreateRootSignature(context);

    m_tonemapper = new Tonemapper();
    m_tonemapper->InitResources(context, m_commonRootSig.Get());

    CreatePSOs(context);
}

void RtTester::Render(RenderContext& context)
{
    m_cameraController->Update();
    UpdateGui(context);
    m_envMap->ConvertToCubemap(context);

    UINT frameIndex = context.SwapChain->GetCurrentBackBufferIndex();

    m_cameraData.ViewProj = TransposeMatrix(m_camera->GetViewProjection());
    XMFLOAT4 camPos = m_camera->GetPosition();
    m_cameraData.Position = { camPos.x, camPos.y, camPos.z };
    m_cameraCb->UploadData(frameIndex, m_cameraData);
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
    context.CommandList->SetPipelineState(context.PsoManager->GetPso(m_psoName));
    ID3D12DescriptorHeap* descHeap[] = { context.TexManager->GetDescriptorHeap() };
    context.CommandList->SetDescriptorHeaps(1, descHeap);
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(0), m_cameraCb->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(1), m_objectCb->GetFrameDataGpuAddress(0));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(3), m_lightManager->GetLightsBufferGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootDescriptorTable(TextureTableIndex, context.TexManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    if (m_drawHelmet)
    {
        for (const auto mesh : m_gltfMesh->GetMeshes())
        {
            context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(2), mesh->GetMaterialBufferGpuAddress(frameIndex));

            context.CommandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            context.CommandList->IASetIndexBuffer(&mesh->GetIndexBufferView());

            context.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context.CommandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
        }
    }

    if (m_drawFloor)
    {
        context.CommandList->SetPipelineState(context.PsoManager->GetPso(m_floorPsoName));
        context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(1), m_floorTransformCb->GetFrameDataGpuAddress(frameIndex));
        context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(2), m_floorMaterialCb->GetFrameDataGpuAddress(frameIndex));
        context.CommandList->IASetVertexBuffers(0, 1, &m_floor->GetVertexBufferView());
        context.CommandList->IASetIndexBuffer(&m_floor->GetIndexBufferView());
        context.CommandList->DrawIndexedInstanced(m_floor->GetIndexCount(), 1, 0, 0, 0);
    }

    m_tonemapper->Render(context);

    auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    context.CommandList->ResourceBarrier(1, &toPresent);
}

void RtTester::LoadGeometry(RenderContext& context)
{
    auto path = ASSETS_DIR + std::string("Models//FlightHelmet//glTF//FlightHelmet.gltf");
    m_gltfMesh = new Model(context, path);

    std::vector<Vertex> verts;
    verts.resize(4);
    verts[0].Pos = { -100.0f, 0.0f, 100.0f };
    verts[1].Pos = { 100.0f, 0.0f, 100.0f };
    verts[2].Pos = { 100.0f, 0.0f, -100.0f };
    verts[3].Pos = { -100.0f, 0.0f, -100.0f };
    verts[0].Norm = { 0.0f, 1.0f, 0.0f };
    verts[1].Norm = { 0.0f, 1.0f, 0.0f };
    verts[2].Norm = { 0.0f, 1.0f, 0.0f };
    verts[3].Norm = { 0.0f, 1.0f, 0.0f };
    std::vector<UINT> ind = { 0, 1, 2, 0, 2, 3 };
    m_floor = new Model(context, verts, ind);
}

void RtTester::CreateRootSignature(RenderContext& context)
{
    CreateCommonRootSignature(context.Device, IID_PPV_ARGS(&m_commonRootSig));
    NAME_D3D12_OBJECT(m_commonRootSig);
}

void RtTester::CreatePSOs(RenderContext& context)
{
    auto& inputLayout = GetInputLayoutUV_N_T();
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = GetDefaultOpaquePsoDescriptor(m_commonRootSig.Get(), 1);
    desc.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };

    desc.DSVFormat = context.SwapChain->GetDepthStencilFormat();
    desc.RTVFormats[0] = m_tonemapper->GetHDRTargetFormat();

    auto shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//PbrNonInstanced.hlsl");
    context.PsoManager->CreatePso(context, m_psoName, shaderPath, desc);

    shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//PbrNonTexturedNonInstanced.hlsl");
    context.PsoManager->CreatePso(context, m_floorPsoName, shaderPath, desc);
}

void RtTester::UpdateGui(RenderContext& context)
{
    Light& dirLight = m_lightManager->GetLightRef(m_directionalLightInd);
    ImGui::Begin("SceneControls");
    ImGui::Text("Lights");
    ImGui::InputFloat4("Color", reinterpret_cast<float*>(&dirLight.Color));
    ImGui::InputFloat3("Direction", reinterpret_cast<float*>(&dirLight.Direction));
    ImGui::Text("");
    ImGui::Text("Rasterizer");
    ImGui::Checkbox("Use Rasterizer", &m_useRasterizer);
    ImGui::Checkbox("Draw Helmet", &m_drawHelmet);
    ImGui::Checkbox("Draw Floor", &m_drawFloor);
    ImGui::End();

    m_lightManager->UpdateLights(context.SwapChain->GetCurrentBackBufferIndex());
}

}