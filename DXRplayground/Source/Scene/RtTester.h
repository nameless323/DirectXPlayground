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

class RtTester : public Scene
{
public:
    ~RtTester() override;

    void InitResources(RenderContext& context) override;
    void Render(RenderContext& context) override;

private:
    void LoadGeometry(RenderContext& context);
    void CreateRootSignature(RenderContext& context);
    void CreatePSOs(RenderContext& context);
    void UpdateLights(RenderContext& context);

    Model* m_gltfMesh = nullptr;
    Model* m_plane = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_commonRootSig; // Move to ctx. It's common after all
    UploadBuffer* m_cameraCb = nullptr;
    UploadBuffer* m_objectCb = nullptr;
    const std::string m_psoName = "Opaque_PBR";
    const std::string m_planePsoName = "Opaque_Non_Textured_PBR";

    Camera* m_camera = nullptr;
    CameraController* m_cameraController = nullptr;
    Tonemapper* m_tonemapper = nullptr;
    LightManager* m_lightManager = nullptr;
    EnvironmentMap* m_envMap = nullptr;
    UINT m_directionalLightInd = 0;
    CameraShaderData m_cameraData{};
};
}