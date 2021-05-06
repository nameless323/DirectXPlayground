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

    Model* m_gltfMesh = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_commonRootSig; // Move to ctx. It's common after all
    UploadBuffer* m_cameraCb = nullptr;
    UploadBuffer* m_objectCbs = nullptr;
    UploadBuffer* m_materials = nullptr;

    const std::string m_psoName = "Opaque_PBR";

    Camera* m_camera = nullptr;
    CameraController* m_cameraController = nullptr;
    Tonemapper* m_tonemapper = nullptr;
    LightManager* m_lightManager = nullptr;
    CameraShaderData m_cameraData{};

    InstanceMaterials m_instanceMaterials;
};
}