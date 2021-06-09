#pragma once

#include <d3d12.h>
#include "Scene/Scene.h"
#include "External/Dx12Helpers/d3dx12.h"

#include "DXrenderer/Shader.h"

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
    void UpdateGui(RenderContext& context);

    // rt

    void InitRaytracingPipeline(RenderContext& context);
    void CreateRtRootSigs(RenderContext& context);
    void CreateRtPSO(RenderContext& context);
    void BuildAccelerationStructures(RenderContext& context);
    void BuildShaderTables(RenderContext& context);
    void RaytraceShadows(RenderContext& context);
    //

    Model* m_suzanne = nullptr;
    Model* m_floor = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_commonRootSig; // Move to ctx. It's common after all
    UploadBuffer* m_cameraCb = nullptr;
    UploadBuffer* m_objectCb = nullptr;
    UploadBuffer* m_floorTransformCb = nullptr;
    UploadBuffer* m_floorMaterialCb = nullptr;
    const std::string m_psoName = "Opaque_PBR";
    const std::string m_floorPsoName = "Opaque_Non_Textured_PBR";
    const std::string m_depthPrepassPsoName = "Depth_Prepass";

    Camera* m_camera = nullptr;
    CameraController* m_cameraController = nullptr;
    Tonemapper* m_tonemapper = nullptr;
    LightManager* m_lightManager = nullptr;
    UINT m_directionalLightInd = 0;
    CameraShaderData m_cameraData{};

    bool m_drawFloor = true;
    bool m_drawSuzanne = true;
    bool m_useRasterizer = true;

    // rt
    struct RtCb
    {
        XMFLOAT4X4 InvViewProj;
        XMFLOAT4 CamPosition;
    };

    UploadBuffer* m_rtSceneDataCB = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rtGlobalRootSig;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rtLocalRootSig;
    Microsoft::WRL::ComPtr<ID3D12StateObject> m_dxrStateObject;

    UploadBuffer* m_missShaderTable = nullptr;
    UploadBuffer* m_hitGroupShaderTable = nullptr;
    UploadBuffer* m_rayGenShaderTable = nullptr;
    UploadBuffer* m_instanceDescs = nullptr; // todo: after flush should be ok to delete, but it's not.
    UnorderedAccessBuffer* m_scratchBuffer = nullptr; // todo: same shit as above

    Shader m_rayGenShader;
    Shader m_closestHitShader;
    Shader m_missShader;
    UnorderedAccessBuffer* m_modelBlas = nullptr;
    UnorderedAccessBuffer* m_floorBlas = nullptr;
    UnorderedAccessBuffer* m_tlas = nullptr;
};
}