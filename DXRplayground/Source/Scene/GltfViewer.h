#pragma once

#include <d3d12.h>
#include "Scene/Scene.h"
#include "External/Dx12Helpers/d3dx12.h"

#include "CameraController.h"
#include "Camera.h"

namespace DirectxPlayground
{
class Model;
class UploadBuffer;
class TextureManager;
class Tonemapper;
class LightManager;

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

    Model* m_gltfMesh = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_commonRootSig; // Move to ctx. It's common after all
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
    UploadBuffer* m_cameraCb = nullptr;
    UploadBuffer* m_objectCb = nullptr;

    Camera* m_camera = nullptr;
    CameraController* m_cameraController = nullptr;
    Tonemapper* m_tonemapper = nullptr;
    LightManager* m_lightManager = nullptr;
    UINT m_directionalLightInd = 0;
};
}