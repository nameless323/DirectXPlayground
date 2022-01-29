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

class PbrTester : public Scene
{
public:
    ~PbrTester() override;

    void InitResources(RenderContext& context) override;
    void Render(RenderContext& context) override;

private:
    inline static constexpr UINT m_instanceCount = 100;
    struct InstanceBuffers
    {
        XMFLOAT4X4 ToWorld[m_instanceCount];
    };
    struct Material
    {
        XMFLOAT4 Albedo{};
        float Metallic{};
        float Roughness{};
        float AO{};
        float Padding{};
    };
    struct InstanceMaterials
    {
        Material Materials[m_instanceCount];
    };

    void LoadGeometry(RenderContext& context);
    void CreateRootSignature(RenderContext& context);
    void CreatePSOs(RenderContext& context);
    void UpdateLights(RenderContext& context);

    Model* mGltfMesh = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mCommonRootSig; // Move to ctx. It's common after all
    UploadBuffer* mCameraCb = nullptr;
    UploadBuffer* mObjectCbs = nullptr;
    UploadBuffer* mMaterials = nullptr;

    const std::string mPsoName = "Opaque_PBR";

    Camera* mCamera = nullptr;
    CameraController* mCameraController = nullptr;
    Tonemapper* mTonemapper = nullptr;
    LightManager* mLightManager = nullptr;
    CameraShaderData mCameraData{};

    InstanceMaterials mInstanceMaterials;
};
}