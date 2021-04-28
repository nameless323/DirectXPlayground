#include "Scene/PbrTester.h"

#include <array>

#include "DXrenderer/Swapchain.h"

#include "DXrenderer/Model.h"
#include "DXrenderer/Shader.h"
#include "DXrenderer/PsoManager.h"
#include "DXrenderer/TextureManager.h"
#include "DXrenderer/Tonemapper.h"
#include "DXrenderer/LightManager.h"

#include "DXrenderer/Buffers/UploadBuffer.h"

#include "External/IMGUI/imgui.h"

namespace DirectxPlayground
{

PbrTester::~PbrTester()
{
    SafeDelete(m_cameraCb);
    SafeDelete(m_camera);
    SafeDelete(m_cameraController);
    SafeDelete(m_objectCbs);
    SafeDelete(m_materials);
    SafeDelete(m_gltfMesh);
    SafeDelete(m_tonemapper);
    SafeDelete(m_lightManager);
}

void PbrTester::InitResources(RenderContext& context)
{
    using Microsoft::WRL::ComPtr;

    m_camera = new Camera(1.0472f, 1.77864583f, 0.001f, 1000.0f);
    m_cameraCb = new UploadBuffer(*context.Device, sizeof(CameraShaderData), true, context.FramesCount);
    m_objectCbs = new UploadBuffer(*context.Device, sizeof(InstanceBuffers), true, context.FramesCount);
    m_materials = new UploadBuffer(*context.Device, sizeof(InstanceMaterials), true, context.FramesCount);
    m_cameraController = new CameraController(m_camera, 1.0f, 12.0f);
    m_lightManager = new LightManager(context);
    Light l = { { 1.0f, 1.0f, 1.0f, 1.0f}, { 0.70710678f, -0.70710678f, 0.0f } };
    m_directionalLightInd = m_lightManager->AddLight(l);

    InstanceBuffers transforms;
    InstanceMaterials materials;
    for (UINT i = 0; i < 10; ++i)
    {
        for (UINT j = 0; j < 10; ++j)
        {
            UINT index = i * 10 + j;
            materials.Materials[index].DiffuseColor = { 0.1f * j, 0.1f * i, 0.0f, 1.0f };

            float x = -6.25f + j * 1.25f;
            float y = -6.25f + i * 1.25f;
            float z = 10.0f;

            XMFLOAT4X4 toWorld;
            XMStoreFloat4x4(&toWorld, XMMatrixTranspose(XMMatrixTranslation(x, y, z)));
            transforms.ToWorld[index] = toWorld;
        }
    }
    m_objectCbs->UploadData(0, transforms.ToWorld);
    m_materials->UploadData(0, materials.Materials);

    LoadGeometry(context);
    CreateRootSignature(context);

    m_tonemapper = new Tonemapper();
    m_tonemapper->InitResources(context, m_commonRootSig.Get());

    CreatePSOs(context);
}

void PbrTester::Render(RenderContext& context)
{
    m_cameraController->Update();
    UpdateLights(context);

    UINT frameIndex = context.SwapChain->GetCurrentBackBufferIndex();

    m_cameraData.ViewProj = TransposeMatrix(m_camera->GetViewProjection());
    XMFLOAT4 camPos = m_camera->GetPosition();
    m_cameraData.Position = { camPos.x, camPos.y, camPos.z };
    m_cameraCb->UploadData(frameIndex, m_cameraData);

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
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(1), m_objectCbs->GetFrameDataGpuAddress(0));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(2), m_materials->GetFrameDataGpuAddress(0));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(3), m_lightManager->GetLightsBufferGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootDescriptorTable(TextureTableIndex, context.TexManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    for (const auto mesh : m_gltfMesh->GetMeshes())
    {
        context.CommandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        context.CommandList->IASetIndexBuffer(&mesh->GetIndexBufferView());

        context.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context.CommandList->DrawIndexedInstanced(mesh->GetIndexCount(), m_renderObjectsNum, 0, 0, 0);
    }
    m_tonemapper->Render(context);

    auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    context.CommandList->ResourceBarrier(1, &toPresent);
}

void PbrTester::LoadGeometry(RenderContext& context)
{
    auto path = ASSETS_DIR + std::string("Models//sphere//sphere.gltf");
    m_gltfMesh = new Model(context, path);
}

void PbrTester::CreateRootSignature(RenderContext& context)
{
    CreateCommonRootSignature(context.Device, IID_PPV_ARGS(&m_commonRootSig));
    NAME_D3D12_OBJECT(m_commonRootSig);
}

void PbrTester::CreatePSOs(RenderContext& context)
{
    auto& inputLayout = GetInputLayoutUV_N_T();
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = GetDefaultOpaquePsoDescriptor(m_commonRootSig.Get(), 1);
    desc.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };

    desc.DSVFormat = context.SwapChain->GetDepthStencilFormat();
    desc.RTVFormats[0] = m_tonemapper->GetHDRTargetFormat();

    auto shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//PbrTestInstanced.hlsl");
    context.PsoManager->CreatePso(context, m_psoName, shaderPath, desc);
}

void PbrTester::UpdateLights(RenderContext& context)
{
    Light& dirLight = m_lightManager->GetLightRef(m_directionalLightInd);
    ImGui::Begin("Lights");
    ImGui::InputFloat4("Color", reinterpret_cast<float*>(&dirLight.Color));
    ImGui::InputFloat3("Direction", reinterpret_cast<float*>(&dirLight.Direction));
    ImGui::End();

    m_lightManager->UpdateLights(context.SwapChain->GetCurrentBackBufferIndex());
}

}