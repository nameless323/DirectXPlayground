#include "Scene/GltfViewer.h"

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

#include "Utils/PixProfiler.h"
#include "External/IMGUI/imgui.h"

namespace DirectxPlayground
{

GltfViewer::~GltfViewer()
{
    SafeDelete(mCameraCb);
    SafeDelete(mCamera);
    SafeDelete(mCameraController);
    SafeDelete(mObjectCb);
    SafeDelete(mGltfMesh);
    SafeDelete(mSkybox);
    SafeDelete(mTonemapper);
    SafeDelete(mLightManager);
    SafeDelete(mEnvMap);
}

void GltfViewer::InitResources(RenderContext& context)
{
    using Microsoft::WRL::ComPtr;

    mCamera = new Camera(1.0472f, 1.77864583f, 0.001f, 1000.0f);
    mCameraCb = new UploadBuffer(*context.Device, sizeof(CameraShaderData), true, context.FramesCount);
    mObjectCb = new UploadBuffer(*context.Device, sizeof(XMFLOAT4X4), true, context.FramesCount);
    mCameraController = new CameraController(mCamera);
    mLightManager = new LightManager(context);

    auto path = ASSETS_DIR + std::string("Textures//colorful_studio_4k.hdr");
    mEnvMap = new EnvironmentMap(context, path, 2048, 64);
    Light l = { { 300.0f, 300.0f, 300.0f, 1.0f}, { 0.0f, 0.0f, 0.0f } };
    mDirectionalLightInd = mLightManager->AddLight(l);

    LoadGeometry(context);
    CreateRootSignature(context);

    mTonemapper = new Tonemapper();
    mTonemapper->InitResources(context, mCommonRootSig.Get());

    CreatePSOs(context);

    context.TexManager->FlushMipsQueue(context);
    mEnvMap->ConvertToCubemap(context);
}

void GltfViewer::Render(RenderContext& context)
{
    GPU_SCOPED_EVENT(context, "Render frame");
    mCameraController->Update();
    UpdateLights(context);

    UINT frameIndex = context.SwapChain->GetCurrentBackBufferIndex();

    XMFLOAT4X4 toWorld;
    XMStoreFloat4x4(&toWorld, XMMatrixTranspose(XMMatrixTranslation(0.0f, 0.0f, 3.0f)));
    mCameraData.ViewProj = TransposeMatrix(mCamera->GetViewProjection());
    XMFLOAT4 camPos = mCamera->GetPosition();
    mCameraData.Position = { camPos.x, camPos.y, camPos.z };
    mCameraData.View = TransposeMatrix(mCamera->GetView());
    mCameraData.Proj = TransposeMatrix(mCamera->GetProjection());
    mCameraCb->UploadData(frameIndex, mCameraData);
    mObjectCb->UploadData(frameIndex, toWorld);
    mGltfMesh->UpdateMeshes(frameIndex);

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
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(1), mObjectCb->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(3), mLightManager->GetLightsBufferGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootDescriptorTable(TextureTableIndex, context.TexManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    CD3DX12_GPU_DESCRIPTOR_HANDLE cubeHeapBegin(context.TexManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
    cubeHeapBegin.Offset(context.CbvSrvUavDescriptorSize * RenderContext::MaxTextures);
    context.CommandList->SetGraphicsRootDescriptorTable(CubemapTableIndex, cubeHeapBegin);

    for (const auto mesh : mGltfMesh->GetMeshes())
    {
        context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(2), mesh->GetMaterialBufferGpuAddress(frameIndex));

        context.CommandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
        context.CommandList->IASetIndexBuffer(&mesh->GetIndexBufferView());

        context.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context.CommandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
    }

    DrawSkybox(context);

    mTonemapper->Render(context);

    auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    context.CommandList->ResourceBarrier(1, &toPresent);
}

void GltfViewer::LoadGeometry(RenderContext& context)
{
    auto path = ASSETS_DIR + std::string("Models//Avocado//glTF//Avocado.gltf");
    //auto path = ASSETS_DIR + std::string("Models//FlightHelmet//glTF//FlightHelmet.gltf");
    mGltfMesh = new Model(context, path);
    path = ASSETS_DIR + std::string("Models//sphere//sphere.gltf");
    mSkybox = new Model(context, path);
}

void GltfViewer::CreateRootSignature(RenderContext& context)
{
    CreateCommonRootSignature(context.Device, IID_PPV_ARGS(&mCommonRootSig));
    AUTO_NAME_D3D12_OBJECT(mCommonRootSig);
}

void GltfViewer::CreatePSOs(RenderContext& context)
{
    auto& inputLayout = GetInputLayoutUV_N_T();
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = GetDefaultOpaquePsoDescriptor(mCommonRootSig.Get(), 1);
    desc.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };

    desc.DSVFormat = context.SwapChain->GetDepthStencilFormat();
    desc.RTVFormats[0] = mTonemapper->GetHDRTargetFormat();

    auto shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//PbrNonInstanced.hlsl");
    context.PsoManager->CreatePso(context, mPsoName, shaderPath, desc);

    desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;

    shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//Skybox.hlsl");
    context.PsoManager->CreatePso(context, mSkyboxPsoName, shaderPath, desc);
}

void GltfViewer::UpdateLights(RenderContext& context)
{
    Light& dirLight = mLightManager->GetLightRef(mDirectionalLightInd);
    ImGui::Begin("Lights");
    ImGui::InputFloat4("Color", reinterpret_cast<float*>(&dirLight.Color));
    ImGui::InputFloat3("Direction", reinterpret_cast<float*>(&dirLight.Direction));
    ImGui::End();

    mLightManager->UpdateLights(context.SwapChain->GetCurrentBackBufferIndex());
}

void GltfViewer::DrawSkybox(RenderContext& context)
{
    GPU_SCOPED_EVENT(context, "Skybox");
    context.CommandList->SetPipelineState(context.PsoManager->GetPso(mSkyboxPsoName));
    const Model::Mesh* skybox = mSkybox->GetMesh();
    context.CommandList->IASetVertexBuffers(0, 1, &skybox->GetVertexBufferView());
    context.CommandList->IASetIndexBuffer(&skybox->GetIndexBufferView());

    context.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context.CommandList->DrawIndexedInstanced(skybox->GetIndexCount(), 1, 0, 0, 0);
}

}