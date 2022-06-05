#pragma once

#include <d3d12.h>
#include "Scene/Scene.h"
#include "External/Dx12Helpers/d3dx12.h"

#include "DXrenderer/Shader.h"
#include "DXrenderer/ResourceDX.h"

#include "CameraController.h"
#include "Camera.h"

#include <array>

namespace DirectxPlayground
{
class Model;
class UploadBuffer;
class TextureManager;
class Tonemapper;
class LightManager;
class EnvironmentMap;
class UnorderedAccessBuffer;

namespace DXR
{
class BottomLevelAccelerationStructure;
class TopLevelAccelerationStructure;
}

class RtTester : public Scene
{
public:
    ~RtTester() override;

    void InitResources(RenderContext& context) override;
    void Render(RenderContext& context) override;

private:
    struct NonTexturedMaterial
    {
        XMFLOAT4 Albedo{};
        float Metallic{};
        float Roughness{};
        float AO{};
        float Padding{};
    } m_floorMaterial;

    void DepthPrepass(RenderContext& context);
    void RenderForwardObjects(RenderContext& context);
    void LoadGeometry(RenderContext& context);
    void CreateRootSignature(RenderContext& context);
    void CreatePSOs(RenderContext& context);
    void UpdateLights(RenderContext& context);

    void DrawSkybox(RenderContext& context);

    // rt

    void InitRaytracingPipeline(RenderContext& context);
    void CreateRtRootSigs(RenderContext& context);
    void CreateRtPSO(RenderContext& context);
    void BuildAccelerationStructures(RenderContext& context);
    void BuildShaderTables(RenderContext& context);
    void RaytraceShadows(RenderContext& context);
    //

    Model* mSuzanne = nullptr;
    Model* mFloor = nullptr;
    Model* mSkybox = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mCommonRootSig; // Move to ctx. It's common after all
    UploadBuffer* mCameraCb = nullptr;
    UploadBuffer* mObjectCb = nullptr;
    UploadBuffer* mFloorTransformCb = nullptr;
    UploadBuffer* mFloorMaterialCb = nullptr;
    UploadBuffer* mEnvCb = nullptr;
    const std::string mPsoName = "Opaque_PBR";
    const std::string mFloorPsoName = "Opaque_Non_Textured_PBR";
    const std::string mDepthPrepassPsoName = "Depth_Prepass";
    const std::string mSkyboxPsoName = "Skybox";

    Camera* mCamera = nullptr;
    CameraController* mCameraController = nullptr;
    Tonemapper* mTonemapper = nullptr;
    LightManager* mLightManager = nullptr;
    EnvironmentMap* mEnvMap = nullptr;
    UINT mDirectionalLightInd = 0;
    CameraShaderData mCameraData{};

    bool mDrawFloor = true;
    bool mDrawSuzanne = true;
    bool mUseRasterizer = true;

    // rt
    static constexpr UINT RootDescriptorSize = 8;
    static constexpr UINT NumHitGroups = 3;
    UploadBuffer* mShadowMapCb = nullptr;
    UploadBuffer* mRtShadowRaysBuffer = nullptr;
    struct RtCb
    {
        XMFLOAT4X4 InvViewProj;
        XMFLOAT4 CamPosition;
    };

    UploadBuffer* mRtSceneDataCb = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRtGlobalRootSig;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mShadowLocalRootSig;
    Microsoft::WRL::ComPtr<ID3D12StateObject> mDxrStateObject;

    UploadBuffer* mMissShaderTable = nullptr;
    UploadBuffer* mHitGroupShaderTable = nullptr;
    UploadBuffer* mRayGenShaderTable = nullptr;
    UploadBuffer* mInstanceDescs = nullptr; // todo: after flush should be ok to delete, but it's not.

    UnorderedAccessBuffer* mScratchBuffer = nullptr; // todo: same shit as above
    UINT mShadowMapIndex = 0;
    UINT mHitGroupStride = 0;

    Shader mRayGenShader;
    Shader mClosestHitShader;
    Shader mMissShader;
    DXR::BottomLevelAccelerationStructure* mModelBlas = nullptr;
    DXR::BottomLevelAccelerationStructure* mFloorBlas = nullptr;
    DXR::BottomLevelAccelerationStructure* mSdfBlas = nullptr;
    DXR::TopLevelAccelerationStructure* mTlas = nullptr;
};
}