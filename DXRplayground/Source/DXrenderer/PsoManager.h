#pragma once

#include <d3d12.h>
#include <map>
#include <string>
#include "External/Dx12Helpers/d3dx12.h"
#include "DXrenderer/Shader.h"

namespace DirectxPlayground
{
struct RenderContext;

class PsoManager
{
public:
    PsoManager();
    ~PsoManager() = default;
    PsoManager& operator= (const PsoManager&) = delete;

    void Init();
    void Shutdown();
    void CreatePso(RenderContext& context, std::string name, std::string shaderPath, D3D12_GRAPHICS_PIPELINE_STATE_DESC desc);
    ID3D12PipelineState* GetPso(const std::string& name);

private:
    struct PsoDesc
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> CompiledPso;
    };
    static constexpr UINT m_maxPso = 4096;

    std::vector<PsoDesc> m_psos;
    std::map<std::string, PsoDesc*> m_psoMap;
    std::map<std::string, std::vector<PsoDesc*>> m_shadersPsos;
    Shader m_vsFallback;
    Shader m_psFallback;
};
}