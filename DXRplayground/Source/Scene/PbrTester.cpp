#include "Scene/PbrTester.h"

#include <array>

#include "DXrenderer/Swapchain.h"

#include "DXrenderer/Model.h"
#include "DXrenderer/Shader.h"
#include "DXrenderer/PsoManager.h"
#include "DXrenderer/Textures/TextureManager.h"
#include "DXrenderer/Tonemapper.h"
#include "DXrenderer/LightManager.h"

#include "DXrenderer/Buffers/UploadBuffer.h"

#include "External/IMGUI/imgui.h"

namespace DirectxPlayground
{

PbrTester::~PbrTester()
{
    SafeDelete(mCameraCb);
    SafeDelete(mCamera);
    SafeDelete(mCameraController);
    SafeDelete(mObjectCbs);
    SafeDelete(mMaterials);
    SafeDelete(mGltfMesh);
    SafeDelete(mTonemapper);
    SafeDelete(mLightManager);
}

void PbrTester::InitResources(RenderContext& context)
{
    using Microsoft::WRL::ComPtr;

    mCamera = new Camera(1.0472f, 1.77864583f, 0.001f, 1000.0f);
    mCameraCb = new UploadBuffer(*context.Device, sizeof(CameraShaderData), true, context.FramesCount);
    mObjectCbs = new UploadBuffer(*context.Device, sizeof(InstanceBuffers), true, context.FramesCount);
    mMaterials = new UploadBuffer(*context.Device, sizeof(InstanceMaterials), true, context.FramesCount);
    mCameraController = new CameraController(mCamera, 1.0f, 12.0f);
    mLightManager = new LightManager(context);
    Light l = { { 300.0f, 300.0f, 300.0f, 1.0f}, { 5.0f, 5.0f, 5.0f } };
    mLightManager->AddLight(l);
    l.Direction = { -5.0f, 5.0f, 5.0f };
    mLightManager->AddLight(l);
    l.Direction = { 5.0f, -5.0f, 5.0f };
    mLightManager->AddLight(l);
    l.Direction = { -5.0f, -5.0f, 5.0f };
    mLightManager->AddLight(l);

    InstanceBuffers transforms;
    for (UINT i = 0; i < 10; ++i)
    {
        for (UINT j = 0; j < 10; ++j)
        {
            UINT index = i * 10 + j;
            mInstanceMaterials.Materials[index].Albedo = { 1.0f, 0.0f, 0.0f, 1.0f };
            mInstanceMaterials.Materials[index].Metallic = 0.1f * i;
            mInstanceMaterials.Materials[index].Roughness = 0.1f * j;
            mInstanceMaterials.Materials[index].AO = 1.0f;

            float x = -6.25f + j * 1.25f;
            float y = -6.25f + i * 1.25f;
            float z = 10.0f;

            XMFLOAT4X4 toWorld;
            XMStoreFloat4x4(&toWorld, XMMatrixTranspose(XMMatrixTranslation(x, y, z)));
            transforms.ToWorld[index] = toWorld;
        }
    }
    mObjectCbs->UploadData(0, transforms.ToWorld);
    mMaterials->UploadData(0, mInstanceMaterials.Materials);

    LoadGeometry(context);
    CreateRootSignature(context);

    mTonemapper = new Tonemapper();
    mTonemapper->InitResources(context, mCommonRootSig.Get());

    CreatePSOs(context);
}

void PbrTester::Render(RenderContext& context)
{
    mCameraController->Update();
    UpdateLights(context);

    UINT frameIndex = context.SwapChain->GetCurrentBackBufferIndex();

    mCameraData.ViewProj = TransposeMatrix(mCamera->GetViewProjection());
    XMFLOAT4 camPos = mCamera->GetPosition();
    mCameraData.Position = { camPos.x, camPos.y, camPos.z };
    mCameraCb->UploadData(frameIndex, mCameraData);
    mMaterials->UploadData(frameIndex, mInstanceMaterials.Materials);

    auto toRt = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    context.CommandList->ResourceBarrier(1, &toRt);

    auto rtCpuHandle = context.TexManager->GetRtHandle(context, mTonemapper->GetRtIndex());

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

    auto lValue = context.SwapChain->GetDSCPUhandle();
    context.CommandList->OMSetRenderTargets(1, &rtCpuHandle, false, &lValue);
    context.CommandList->ClearRenderTargetView(rtCpuHandle, mTonemapper->GetClearColor(), 0, nullptr);
    context.CommandList->ClearDepthStencilView(context.SwapChain->GetDSCPUhandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    context.CommandList->SetGraphicsRootSignature(mCommonRootSig.Get());
    context.CommandList->SetPipelineState(context.PsoManager->GetPso(mPsoName));
    ID3D12DescriptorHeap* descHeap[] = { context.TexManager->GetDescriptorHeap() };
    context.CommandList->SetDescriptorHeaps(1, descHeap);
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(0), mCameraCb->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(1), mObjectCbs->GetFrameDataGpuAddress(0));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(2), mMaterials->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(3), mLightManager->GetLightsBufferGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootDescriptorTable(TextureTableIndex, context.TexManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    for (const auto& mesh : mGltfMesh->GetMeshes())
    {
        context.CommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
        context.CommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());

        context.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context.CommandList->DrawIndexedInstanced(mesh.GetIndexCount(), m_instanceCount, 0, 0, 0);
    }
    mTonemapper->Render(context);

    auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    context.CommandList->ResourceBarrier(1, &toPresent);
}

void PbrTester::LoadGeometry(RenderContext& context)
{
    auto path = ASSETS_DIR + std::string("Models//sphere//sphere.gltf");
    mGltfMesh = new Model(context, path);
}

void PbrTester::CreateRootSignature(RenderContext& context)
{
    CreateCommonRootSignature(context.Device, IID_PPV_ARGS(&mCommonRootSig));
    AUTO_NAME_D3D12_OBJECT(mCommonRootSig);
}

void PbrTester::CreatePSOs(RenderContext& context)
{
    auto& inputLayout = GetInputLayoutUV_N_T();
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = GetDefaultOpaquePsoDescriptor(mCommonRootSig.Get(), 1);
    desc.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };

    desc.DSVFormat = context.SwapChain->GetDepthStencilFormat();
    desc.RTVFormats[0] = mTonemapper->GetHDRTargetFormat();

    auto shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//PbrTestInstanced.hlsl");
    context.PsoManager->CreatePso(context, mPsoName, shaderPath, desc);
}

void PbrTester::UpdateLights(RenderContext& context)
{
    Light* lights = mLightManager->GetLights();
    ImGui::Begin("Lights");
    //for (UINT i = 0; i < 4; ++i)
    //{
    //    ImGui::InputFloat4("Color", reinterpret_cast<float*>(&lights[0].Color));
    //    ImGui::InputFloat3("Direction", reinterpret_cast<float*>(&lights[0].Direction));
    //    ImGui::Text("");
    //}
    //ImGui::SliderFloat("roughness", &m_instanceMaterials.Materials[0].Roughness, 0.0f, 1.0f);
    //ImGui::SliderFloat("metallness", &m_instanceMaterials.Materials[0].Metallic, 0.0f, 1.0f);
    ImGui::End();

    mLightManager->UpdateLights(context.SwapChain->GetCurrentBackBufferIndex());
}

}