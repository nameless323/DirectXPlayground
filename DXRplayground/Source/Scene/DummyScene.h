#pragma once

#include <d3d12.h>
#include "Scene/Scene.h"
#include "DXrenderer/Shader.h"
#include "External/Dx12Helpers/d3dx12.h"

#include "CameraController.h"
#include "Camera.h"

namespace DirectxPlayground
{
class Mesh;
class UploadBuffer;

class DummyScene : public Scene
{
public:
    ~DummyScene() override;

    void InitResources(RenderContext& context) override;
    void Render(RenderContext& context) override;

private:
    Mesh* m_mesh = nullptr;
    Mesh* m_sphere = nullptr;
    Shader m_ps;
    Shader m_vs;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_triangleRootSig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
    UploadBuffer* m_cameraCb = nullptr;
    UploadBuffer* m_objectCb = nullptr;

    Camera* m_camera = nullptr;
    CameraController* m_cameraController = nullptr;
};
}