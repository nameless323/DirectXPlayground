#pragma once

#include <d3d12.h>
#include "Scene/Scene.h"
#include "External/Dx12Helpers/d3dx12.h"

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

class GltfViewer : public Scene
{
public:
    ~GltfViewer() override;

    void InitResources(RenderContext& context) override;
    void Render(RenderContext& context) override;

private:
    void LoadGeometry(RenderContext& context);
    void CreateRootSignature(RenderContext& context);
    void CreatePSOs(RenderContext& context);
    void UpdateLights(RenderContext& context);
    void DrawSkybox(RenderContext& context);

    Model* mGltfMesh = nullptr;
    Model* mSkybox = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mCommonRootSig; // Move to ctx. It's common after all
    UploadBuffer* mCameraCb = nullptr;
    UploadBuffer* mObjectCb = nullptr;
    UploadBuffer* mEnvCb = nullptr;
    const std::string mPsoName = "Opaque_PBR";
    const std::string mSkyboxPsoName = "Skybox";

    Camera* mCamera = nullptr;
    CameraController* mCameraController = nullptr;
    Tonemapper* mTonemapper = nullptr;
    LightManager* mLightManager = nullptr;
    EnvironmentMap* mEnvMap = nullptr;
    UINT mDirectionalLightInd = 0;
    CameraShaderData mCameraData{};
};
}