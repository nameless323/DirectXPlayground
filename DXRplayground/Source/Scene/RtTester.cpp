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
#include "DXrenderer/RenderPipeline.h"
#include "DXrenderer/DXR/AccelerationStructure.h"

#include "External/IMGUI/imgui.h"
#include "Utils/PixProfiler.h"

namespace DirectxPlayground
{

namespace
{
static constexpr UINT AccelStructSlot = 0;
static constexpr UINT DXROutputSlot = 1;
static constexpr UINT SceneCBSlot = 2;
}

using Microsoft::WRL::ComPtr;

RtTester::~RtTester()
{
    SafeDelete(mCameraCb);
    SafeDelete(mCamera);
    SafeDelete(mCameraController);
    SafeDelete(mObjectCb);
    SafeDelete(mSuzanne);
    SafeDelete(mSkybox);
    SafeDelete(mTonemapper);
    SafeDelete(mLightManager);
    SafeDelete(mFloor);
    SafeDelete(mFloorMaterialCb);
    SafeDelete(mFloorTransformCb);
    SafeDelete(mEnvMap);
    SafeDelete(mEnvCb);

    // dxr
    SafeDelete(mTlas);
    SafeDelete(mModelBlas);
    SafeDelete(mFloorBlas);
    SafeDelete(mSdfBlas);
    SafeDelete(mMissShaderTable);
    SafeDelete(mHitGroupShaderTable);
    SafeDelete(mRayGenShaderTable);
    SafeDelete(mInstanceDescs);
    SafeDelete(mScratchBuffer);
    SafeDelete(mRtSceneDataCb);
    SafeDelete(mShadowMapCb);
    SafeDelete(mRtShadowRaysBuffer);
}

void RtTester::InitResources(RenderContext& context)
{
    using Microsoft::WRL::ComPtr;

    mCamera = new Camera(1.0472f, 1.77864583f, 0.001f, 1000.0f);
    mCamera->SetWorldPosition({ 0.0f, 2.0f, -2.0f });
    mCameraCb = new UploadBuffer(*context.Device, sizeof(CameraShaderData), true, context.FramesCount);
    mObjectCb = new UploadBuffer(*context.Device, sizeof(XMFLOAT4X4) * 2, true, 1);
    mEnvCb = new UploadBuffer(*context.Device, sizeof(EnvironmentData), true, 1);

    auto path = ASSETS_DIR + std::string("Textures//colorful_studio_4k.hdr");
    mEnvMap = new EnvironmentMap(context, path, 2048, 64);

    XMFLOAT4X4 toWorld[2];
    XMStoreFloat4x4(&toWorld[0], XMMatrixTranspose(XMMatrixTranslation(0.0f, 2.0f, 3.0f)));
    XMStoreFloat4x4(&toWorld[1], XMMatrixTranspose(XMMatrixTranslation(5.0f, 2.0f, 3.0f)));
    mObjectCb->UploadData(0, toWorld);

    mFloorTransformCb = new UploadBuffer(*context.Device, sizeof(XMFLOAT4X4), true, context.FramesCount);
    XMStoreFloat4x4(&toWorld[0], XMMatrixTranspose(XMMatrixTranslation(0.0f, 0.0f, 0.0f)));
    for (UINT i = 0; i < context.FramesCount; ++i)
        mFloorTransformCb->UploadData(i, toWorld);

    mFloorMaterialCb = new UploadBuffer(*context.Device, sizeof(NonTexturedMaterial), true, context.FramesCount);
    m_floorMaterial.Albedo = { 0.8f, 0.8f, 0.8f, 1.0f };
    m_floorMaterial.AO = 1.0f;
    m_floorMaterial.Metallic = 0.0f;
    m_floorMaterial.Roughness = 1.0f;
    for (UINT i = 0; i < context.FramesCount; ++i)
        mFloorMaterialCb->UploadData(i, m_floorMaterial);

    mCameraController = new CameraController(mCamera);
    mLightManager = new LightManager(context);

    Light l = { { 300.0f, 300.0f, 300.0f, 1.0f}, { 0.0f, 10.0f, 10.0f } };
    mDirectionalLightInd = mLightManager->AddLight(l);

    LoadGeometry(context);
    CreateRootSignature(context);

    mTonemapper = new Tonemapper();
    mTonemapper->InitResources(context, mCommonRootSig.Get());

    CreatePSOs(context);

    //InitRaytracingPipeline(context);
    mShadowMapCb = new UploadBuffer(*context.Device, sizeof(UINT), true, 1);
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    resDesc.Width = context.Width;
    resDesc.Height = context.Height;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    resDesc.DepthOrArraySize = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    mShadowMapIndex = context.TexManager->CreateDxrOutput(context, resDesc);
    mShadowMapCb->UploadData(0, mShadowMapIndex);

    context.TexManager->FlushMipsQueue(context);
    mEnvMap->ConvertToCubemap(context);

    EnvironmentData envData{ mEnvMap->GetCubemapIndex(), mEnvMap->GetIrradianceMapIndex() };
    mEnvCb->UploadData(0, envData);
}

void RtTester::Render(RenderContext& context)
{
    GPU_SCOPED_EVENT(context, "Render");
    mCameraController->Update();
    UpdateLights(context);

    UINT frameIndex = context.SwapChain->GetCurrentBackBufferIndex();

    mCameraData.ViewProj = TransposeMatrix(mCamera->GetViewProjection());
    XMFLOAT4 camPos = mCamera->GetPosition();
    mCameraData.Position = { camPos.x, camPos.y, camPos.z };
    mCameraCb->UploadData(frameIndex, mCameraData);
    mSuzanne->UpdateMeshes(frameIndex);

    auto toRt = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    context.CommandList->ResourceBarrier(1, &toRt);

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

    DepthPrepass(context);
    //RaytraceShadows(context);
    RenderForwardObjects(context);
    DrawSkybox(context);

    //ImGui::Begin("TexTest");
    //ImGui::Image(context.ImguiTexManager->GetTextureId(context.TexManager->GetDXRResource()), { 256, 256 });
    //ImGui::End();

    mTonemapper->Render(context);

    auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(context.SwapChain->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    context.CommandList->ResourceBarrier(1, &toPresent);
}

void RtTester::DepthPrepass(RenderContext& context)
{
    GPU_SCOPED_EVENT(context, "DepthPrepass");

    UINT frameIndex = context.SwapChain->GetCurrentBackBufferIndex();
    auto rtCpuHandle = context.TexManager->GetRtHandle(context, mTonemapper->GetRtIndex());

    auto lValue = context.SwapChain->GetDSCPUhandle();
    context.CommandList->OMSetRenderTargets(0, nullptr, false, &lValue);
    context.CommandList->ClearDepthStencilView(context.SwapChain->GetDSCPUhandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    context.CommandList->SetGraphicsRootSignature(mCommonRootSig.Get());
    context.CommandList->SetPipelineState(context.PsoManager->GetPso(mDepthPrepassPsoName));
    ID3D12DescriptorHeap* descHeap[] = { context.TexManager->GetDescriptorHeap() };
    context.CommandList->SetDescriptorHeaps(1, descHeap);
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(0), mCameraCb->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(1), mObjectCb->GetFrameDataGpuAddress(0));

    if (mDrawSuzanne)
    {
        for (const auto& mesh : mSuzanne->GetMeshes())
        {
            context.CommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
            context.CommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());

            context.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context.CommandList->DrawIndexedInstanced(mesh.GetIndexCount(), 2, 0, 0, 0);
        }
    }

    if (mDrawFloor)
    {
        context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(1), mFloorTransformCb->GetFrameDataGpuAddress(frameIndex));
        context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(4), mShadowMapCb->GetFrameDataGpuAddress(0));
        context.CommandList->IASetVertexBuffers(0, 1, &mFloor->GetVertexBufferView());
        context.CommandList->IASetIndexBuffer(&mFloor->GetIndexBufferView());
        context.CommandList->DrawIndexedInstanced(mFloor->GetIndexCount(), 1, 0, 0, 0);
    }
}

void RtTester::RenderForwardObjects(RenderContext& context)
{
    GPU_SCOPED_EVENT(context, "RenderForwardObjects");
    UINT frameIndex = context.SwapChain->GetCurrentBackBufferIndex();
    auto rtCpuHandle = context.TexManager->GetRtHandle(context, mTonemapper->GetRtIndex());

    auto lValue = context.SwapChain->GetDSCPUhandle();
    context.CommandList->OMSetRenderTargets(1, &rtCpuHandle, false, &lValue);
    context.CommandList->ClearRenderTargetView(rtCpuHandle, mTonemapper->GetClearColor(), 0, nullptr);

    context.CommandList->SetGraphicsRootSignature(mCommonRootSig.Get());
    context.CommandList->SetPipelineState(context.PsoManager->GetPso(mPsoName));
    ID3D12DescriptorHeap* descHeap[] = { context.TexManager->GetDescriptorHeap() };
    context.CommandList->SetDescriptorHeaps(1, descHeap);
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(0), mCameraCb->GetFrameDataGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(1), mObjectCb->GetFrameDataGpuAddress(0));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(3), mLightManager->GetLightsBufferGpuAddress(frameIndex));
    context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(5), mEnvCb->GetFrameDataGpuAddress(0));
    context.CommandList->SetGraphicsRootDescriptorTable(TextureTableIndex, context.TexManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());

    CD3DX12_GPU_DESCRIPTOR_HANDLE cubeHeapBegin(context.TexManager->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
    cubeHeapBegin.Offset(context.CbvSrvUavDescriptorSize * RenderContext::MaxTextures);
    context.CommandList->SetGraphicsRootDescriptorTable(CubemapTableIndex, cubeHeapBegin);

    if (mDrawSuzanne)
    {
        for (const auto& mesh : mSuzanne->GetMeshes())
        {
            context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(2), mesh.GetMaterialBufferGpuAddress(frameIndex));

            context.CommandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
            context.CommandList->IASetIndexBuffer(&mesh.GetIndexBufferView());

            context.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context.CommandList->DrawIndexedInstanced(mesh.GetIndexCount(), 2, 0, 0, 0);
        }
    }

    if (mDrawFloor)
    {
        context.CommandList->SetPipelineState(context.PsoManager->GetPso(mFloorPsoName));
        context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(1), mFloorTransformCb->GetFrameDataGpuAddress(frameIndex));
        context.CommandList->SetGraphicsRootConstantBufferView(GetCBRootParamIndex(2), mFloorMaterialCb->GetFrameDataGpuAddress(frameIndex));
        context.CommandList->IASetVertexBuffers(0, 1, &mFloor->GetVertexBufferView());
        context.CommandList->IASetIndexBuffer(&mFloor->GetIndexBufferView());
        context.CommandList->DrawIndexedInstanced(mFloor->GetIndexCount(), 1, 0, 0, 0);
    }
}

void RtTester::LoadGeometry(RenderContext& context)
{
    //auto path = ASSETS_DIR + std::string("Models//Suzanne//glTF//Suzanne.gltf");
    auto path = ASSETS_DIR + std::string("Models//FlightHelmet//glTF//FlightHelmet.gltf");

    mSuzanne = new Model(context, path);
    path = ASSETS_DIR + std::string("Models//sphere//sphere.gltf");
    mSkybox = new Model(context, path);

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
    mFloor = new Model(context, verts, ind);
}

void RtTester::CreateRootSignature(RenderContext& context)
{
    CreateCommonRootSignature(context.Device, IID_PPV_ARGS(&mCommonRootSig));
    AUTO_NAME_D3D12_OBJECT(mCommonRootSig);
}

void RtTester::CreatePSOs(RenderContext& context)
{
    auto& inputLayout = GetInputLayoutUV_N_T();
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = GetDefaultOpaquePsoDescriptor(mCommonRootSig.Get(), 0);
    desc.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC skyboxDesc = desc;

    desc.DSVFormat = context.SwapChain->GetDepthStencilFormat();

    std::vector<DxcDefine> instancingDefines = { { L"INSTANCING", L"" } };

    auto shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//DepthPrepass.hlsl");
    context.PsoManager->CreatePso(context, mDepthPrepassPsoName, shaderPath, desc, &instancingDefines);

    desc.NumRenderTargets = 1;
    desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.RTVFormats[0] = mTonemapper->GetHDRTargetFormat();

    shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//PbrForward.hlsl");
    context.PsoManager->CreatePso(context, mPsoName, shaderPath, desc, &instancingDefines);

    shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//PbrNonInstancedDxrShadowed.hlsl");
    context.PsoManager->CreatePso(context, mFloorPsoName, shaderPath, desc);

    desc.DSVFormat = context.SwapChain->GetDepthStencilFormat();
    desc.RTVFormats[0] = mTonemapper->GetHDRTargetFormat();
    desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;

    shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//Skybox.hlsl");
    context.PsoManager->CreatePso(context, mSkyboxPsoName, shaderPath, desc);
}

void RtTester::UpdateLights(RenderContext& context)
{
    Light& dirLight = mLightManager->GetLightRef(mDirectionalLightInd);
    ImGui::Begin("SceneControls");
    ImGui::Text("Lights");
    ImGui::InputFloat4("Color", reinterpret_cast<float*>(&dirLight.Color));
    ImGui::InputFloat3("Direction", reinterpret_cast<float*>(&dirLight.Direction));
    ImGui::Text("");
    ImGui::Text("Rasterizer");
    ImGui::Checkbox("Use Rasterizer", &mUseRasterizer);
    ImGui::Checkbox("Draw Suzanne", &mDrawSuzanne);
    ImGui::Checkbox("Draw Floor", &mDrawFloor);
    ImGui::End();

    mLightManager->UpdateLights(context.SwapChain->GetCurrentBackBufferIndex());
}

void RtTester::DrawSkybox(RenderContext& context)
{
    GPU_SCOPED_EVENT(context, "Skybox");
    context.CommandList->SetPipelineState(context.PsoManager->GetPso(mSkyboxPsoName));
    const Model::Mesh* skybox = mSkybox->GetMesh();
    context.CommandList->IASetVertexBuffers(0, 1, &skybox->GetVertexBufferView());
    context.CommandList->IASetIndexBuffer(&skybox->GetIndexBufferView());

    context.CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context.CommandList->DrawIndexedInstanced(skybox->GetIndexCount(), 1, 0, 0, 0);
}

void RtTester::InitRaytracingPipeline(RenderContext& context)
{
    mRtSceneDataCb = new UploadBuffer(*context.Device, sizeof(RtCb), true, context.FramesCount);
    mShadowMapCb = new UploadBuffer(*context.Device, sizeof(UINT), true, 1);
    mRtShadowRaysBuffer = new UploadBuffer(*context.Device, sizeof(XMFLOAT4), true, 1);

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    resDesc.Width = context.Width;
    resDesc.Height = context.Height;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    resDesc.DepthOrArraySize = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    mShadowMapIndex = context.TexManager->CreateDxrOutput(context, resDesc);
    mShadowMapCb->UploadData(0, mShadowMapIndex);

    context.ImguiTexManager->AddTexture(context.TexManager->GetDXRResource());

    CreateRtRootSigs(context);
    BuildAccelerationStructures(context);
    CreateRtPSO(context);
    BuildShaderTables(context);
}

void RtTester::CreateRtRootSigs(RenderContext& context)
{
    std::vector<CD3DX12_ROOT_PARAMETER> params{};
    params.push_back({});
    params.back().InitAsShaderResourceView(0); // Acceleration structure

    CD3DX12_DESCRIPTOR_RANGE uavDesc;
    uavDesc.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    params.push_back({});
    params.back().InitAsDescriptorTable(1, &uavDesc); // Output rt

    params.push_back({});
    params.back().InitAsConstantBufferView(0); // Scene cb (camera etc)

    CD3DX12_ROOT_SIGNATURE_DESC globalRootSigDesc{ UINT(params.size()), params.data() };
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed(D3D12SerializeRootSignature(&globalRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
    ThrowIfFailed(context.Device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&mRtGlobalRootSig)));

    // dont overlap buffers
    CD3DX12_ROOT_PARAMETER localRootParams[1]{};
    localRootParams[0].InitAsConstantBufferView(0, 1);
    CD3DX12_ROOT_SIGNATURE_DESC localRootSigDesc(1, localRootParams);
    localRootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    ThrowIfFailed(D3D12SerializeRootSignature(&localRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
    ThrowIfFailed(context.Device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&mShadowLocalRootSig)));
}

void RtTester::BuildAccelerationStructures(RenderContext& context)
{
    mModelBlas = new DXR::BottomLevelAccelerationStructure(context, mSuzanne);
    mFloorBlas = new DXR::BottomLevelAccelerationStructure(context, mFloor);

    D3D12_RAYTRACING_AABB aabb =
    { -3.0f, -3.0f, -3.0f,
      3.0f, 3.0f, 3.0f };
    mSdfBlas = new DXR::BottomLevelAccelerationStructure(context, aabb);

    mTlas = new DXR::TopLevelAccelerationStructure();

    mModelBlas->Prebuild(context);
    mFloorBlas->Prebuild(context);
    mSdfBlas->Prebuild(context);

    XMFLOAT4X4 transform = IdentityMatrix;

    mTlas->AddDescriptor(*mFloorBlas, transform, 2);

    transform._24 = 2.0f;
    transform._34 = 3.0f;

    mTlas->AddDescriptor(*mModelBlas, transform, 1);

    transform._14 = 5.0f;
    mTlas->AddDescriptor(*mModelBlas, transform, 1);

    transform._14 = -6.0f;
    transform._24 = 2.0f;
    transform._34 = 3.0f;

    mTlas->AddDescriptor(*mSdfBlas, transform, 1, 0, 0, 1);
    mTlas->Prebuild(context);

    UINT64 maxScratchBufferSize = DXR::AccelerationStructure::GetMaxScratchSize({ mModelBlas, mFloorBlas, mSdfBlas, mTlas });
    mScratchBuffer = new UnorderedAccessBuffer(context.CommandList, *context.Device, UINT(maxScratchBufferSize));
    mModelBlas->Build(context, mScratchBuffer, true);
    mFloorBlas->Build(context, mScratchBuffer, true);
    mSdfBlas->Build(context, mScratchBuffer, true);
    mTlas->Build(context, mScratchBuffer, false);
}

void RtTester::CreateRtPSO(RenderContext& context)
{
    CD3DX12_STATE_OBJECT_DESC rtPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    CD3DX12_DXIL_LIBRARY_SUBOBJECT* lib = rtPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

    auto shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//Raytracing.hlsl");

    bool success = Shader::CompileFromFile(shaderPath, L"", L"lib_6_3", mRayGenShader, nullptr);
    assert(success);

    lib->SetDXILLibrary(&mRayGenShader.GetBytecode());

    lib->DefineExport(L"Raygen");
    lib->DefineExport(L"ClosestHit");
    lib->DefineExport(L"Miss");
    lib->DefineExport(L"AnyHit");

    lib->DefineExport(L"ShadowClosestHit");
    lib->DefineExport(L"ShadowMiss");

    lib->DefineExport(L"SdfIntersection");
    lib->DefineExport(L"SdfClosestHit");

    CD3DX12_HIT_GROUP_SUBOBJECT* hitGroup = rtPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitGroup->SetClosestHitShaderImport(L"ClosestHit");
    hitGroup->SetAnyHitShaderImport(L"AnyHit");
    hitGroup->SetHitGroupExport(L"TriangleHitGroup");
    hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    CD3DX12_HIT_GROUP_SUBOBJECT* shadowHitGroup = rtPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    shadowHitGroup->SetClosestHitShaderImport(L"ShadowClosestHit");
    shadowHitGroup->SetHitGroupExport(L"ShadowHitGroup");
    shadowHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    CD3DX12_HIT_GROUP_SUBOBJECT* sdfHitGroup = rtPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    sdfHitGroup->SetIntersectionShaderImport(L"SdfIntersection");
    sdfHitGroup->SetClosestHitShaderImport(L"SdfClosestHit");
    sdfHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE);
    sdfHitGroup->SetHitGroupExport(L"SdfHitGroup");

    // max sizes
    CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* shaderConfig = rtPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    UINT payloadSize = sizeof(XMFLOAT4); // float4 color
    UINT attribSize = sizeof(XMFLOAT2); //float2 barycentrics. default for trigs
    shaderConfig->Config(payloadSize, attribSize);

    CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT* localRootSig = rtPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    localRootSig->SetRootSignature(mShadowLocalRootSig.Get());

    CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT* localRootSigAssociation = rtPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    localRootSigAssociation->SetSubobjectToAssociate(*localRootSig);
    localRootSigAssociation->AddExport(L"TriangleHitGroup");

    CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* globalRootSig = rtPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSig->SetRootSignature(mRtGlobalRootSig.Get());

    CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* pipelineConfig = rtPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    UINT maxRecursionDepth = 2;
    pipelineConfig->Config(maxRecursionDepth);

    ThrowIfFailed(context.Device->CreateStateObject(rtPipeline, IID_PPV_ARGS(&mDxrStateObject)), L"Couldn't create raytracing state object.\n");
}

void RtTester::BuildShaderTables(RenderContext& context)
{
    ComPtr<ID3D12StateObjectProperties> stateObjectProps;
    ThrowIfFailed(mDxrStateObject.As(&stateObjectProps));
    void* rayGenShaderIdentifier = stateObjectProps->GetShaderIdentifier(L"Raygen");
    void* missShaderIdentifier = stateObjectProps->GetShaderIdentifier(L"Miss");
    void* hitGroupShaderIdentifier = stateObjectProps->GetShaderIdentifier(L"TriangleHitGroup");

    void* shadowMissShaderIdentifier = stateObjectProps->GetShaderIdentifier(L"ShadowMiss");
    void* shadowHitGroupShaderIdentifier = stateObjectProps->GetShaderIdentifier(L"ShadowHitGroup");

    void* sdfHitGroupShaderIdentifier = stateObjectProps->GetShaderIdentifier(L"SdfHitGroup");

    UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; // + localRootSigSize

    UINT numShaderRecords = 1;
    // Local root sig for each table + copy data there. should be alighed by max root sig + BYTE_ALIGNMENT
    // might be needed for specific hit gropus - light position only for shadows
    UINT shaderRecordSize = Align(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

    mRayGenShaderTable = new UploadBuffer(*context.Device, shaderRecordSize, false, 1, true);
    mRayGenShaderTable->UploadData(0, reinterpret_cast<byte*>(rayGenShaderIdentifier));
    mRayGenShaderTable->SetName(L"RayGenShaderTable");

    UINT hitGroupShaderRecordSize = Align(shaderIdentifierSize + sizeof(D3D12_GPU_VIRTUAL_ADDRESS), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    mHitGroupStride = hitGroupShaderRecordSize;
    std::vector<byte> hitGroups;
    hitGroups.resize(size_t(hitGroupShaderRecordSize) * NumHitGroups); // hit group for primary rays + hit group for shadow rays + sdf hit group
    byte* data = hitGroups.data();
    // TRIANGLE HIT GROUP local root sig - 1cb. shader record + cb
    memcpy(data, hitGroupShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mRtShadowRaysBuffer->GetFrameDataGpuAddress(0);
    memcpy(data + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, &cbAddress, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));

    // SHADOW HIT GROUP no local root sig for shadow rays
    memcpy(data + hitGroupShaderRecordSize, shadowHitGroupShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // SDF HIT GROUP
    memcpy(data + UINT64(hitGroupShaderRecordSize) * 2UL, sdfHitGroupShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    mHitGroupShaderTable = new UploadBuffer(*context.Device, hitGroupShaderRecordSize * NumHitGroups, false, 1, true);
    mHitGroupShaderTable->UploadData(0, reinterpret_cast<const byte*>(data));
    mHitGroupShaderTable->SetName(L"HitGroupShaderTable");

    struct Misses
    {
        byte miss[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
        byte shadowMiss[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
    } misses;
    memcpy(&misses.miss, missShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(&misses.shadowMiss, shadowMissShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    mMissShaderTable = new UploadBuffer(*context.Device, sizeof(Misses), false, 1, true);
    mMissShaderTable->UploadData(0, misses);
    mMissShaderTable->SetName(L"MissShaderTable");

}

void RtTester::RaytraceShadows(RenderContext& context)
{
    GPU_SCOPED_EVENT(context, "RaytraceShadows");
    XMMATRIX viewProj = XMLoadFloat4x4(&mCamera->GetViewProjection());
    XMVECTOR det = XMMatrixDeterminant(viewProj);
    XMMATRIX invViewProjSimd = XMMatrixInverse(&det, viewProj);
    XMFLOAT4X4 invViewProj;
    XMStoreFloat4x4(&invViewProj, XMMatrixTranspose(invViewProjSimd));
    RtCb rtSceneData{};
    rtSceneData.InvViewProj = invViewProj;
    rtSceneData.CamPosition = mCamera->GetPosition();
    mRtSceneDataCb->UploadData(context.SwapChain->GetCurrentBackBufferIndex(), rtSceneData);

    Light& dirLight = mLightManager->GetLightRef(mDirectionalLightInd);
    mRtShadowRaysBuffer->UploadData(0, XMFLOAT4{ dirLight.Direction.x, dirLight.Direction.y, dirLight.Direction.z, 1.0f });

    auto cmdList = context.CommandList;

    auto transition = CD3DX12_RESOURCE_BARRIER::Transition(context.TexManager->GetDXRResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    context.CommandList->ResourceBarrier(1, &transition);

    ID3D12DescriptorHeap* descHeaps[] = { context.TexManager->GetDXRUavHeap() };
    cmdList->SetComputeRootSignature(mRtGlobalRootSig.Get());
    cmdList->SetDescriptorHeaps(1, descHeaps);
    cmdList->SetComputeRootConstantBufferView(SceneCBSlot, mRtSceneDataCb->GetFrameDataGpuAddress(context.SwapChain->GetCurrentBackBufferIndex()));
    cmdList->SetComputeRootShaderResourceView(AccelStructSlot, mTlas->GetGpuAddress());
    cmdList->SetComputeRootDescriptorTable(DXROutputSlot, context.TexManager->GetDXRUavHeap()->GetGPUDescriptorHandleForHeapStart());

    D3D12_DISPATCH_RAYS_DESC desc = {};
    desc.Width = context.Width;
    desc.Height = context.Height;
    desc.Depth = 1;

    desc.HitGroupTable.StartAddress = mHitGroupShaderTable->GetFrameDataGpuAddress(0);
    desc.HitGroupTable.SizeInBytes = UINT64(mHitGroupStride) * NumHitGroups;
    desc.HitGroupTable.StrideInBytes = mHitGroupStride;

    desc.MissShaderTable.StartAddress = mMissShaderTable->GetFrameDataGpuAddress(0);
    desc.MissShaderTable.SizeInBytes = mMissShaderTable->GetBufferSize();
    desc.MissShaderTable.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    desc.RayGenerationShaderRecord.StartAddress = mRayGenShaderTable->GetFrameDataGpuAddress(0);
    desc.RayGenerationShaderRecord.SizeInBytes = mRayGenShaderTable->GetBufferSize();

    cmdList->SetPipelineState1(mDxrStateObject.Get());
    cmdList->DispatchRays(&desc);

    transition = CD3DX12_RESOURCE_BARRIER::Transition(context.TexManager->GetDXRResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    context.CommandList->ResourceBarrier(1, &transition);
}

}
