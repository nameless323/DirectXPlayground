#pragma once

#include <d3d12.h>
#include "Scene/Scene.h"
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{
class Shader;

class DummyScene : public Scene
{
public:
    ~DummyScene() override;

    void InitResources(RenderContext& context) override;
    void Render(RenderContext& context) override;

private:
    Mesh* m_mesh = nullptr;
    Shader m_ps;
    Shader m_vs;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_triangleRootSig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
};
}