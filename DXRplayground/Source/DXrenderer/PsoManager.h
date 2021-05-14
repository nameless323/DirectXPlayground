#pragma once

#include <d3d12.h>
#include <map>
#include <string>
#include <filesystem>
#include "External/Dx12Helpers/d3dx12.h"
#include "DXrenderer/Shader.h"

namespace DirectxPlayground
{
struct RenderContext;
class FileWatcher;

class PsoManager
{
public:
    PsoManager();
    ~PsoManager() = default;
    PsoManager& operator= (const PsoManager&) = delete;

    void BeginFrame(RenderContext& context);
    void Shutdown();
    void CreatePso(RenderContext& context, std::string name, std::wstring shaderPath, D3D12_GRAPHICS_PIPELINE_STATE_DESC desc);
    void CreatePso(RenderContext& context, std::string name, std::wstring shaderPath, D3D12_COMPUTE_PIPELINE_STATE_DESC desc);

    ID3D12PipelineState* GetPso(const std::string& name);

private:
    struct PsoDesc
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc{};
        Microsoft::WRL::ComPtr<ID3D12PipelineState> CompiledPso;
    };
    struct ComputePsoDesc
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC Desc{};
        Microsoft::WRL::ComPtr<ID3D12PipelineState> CompiledPso;
    };
    static constexpr UINT m_maxPso = 4096;

    void CompilePsoWithShader(RenderContext& context, REFIID psoRiid, void** psoPpv, const std::wstring& shaderPath, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
    void CompilePsoWithShader(RenderContext& context, REFIID psoRiid, void** psoPpv, const std::wstring& shaderPath, D3D12_COMPUTE_PIPELINE_STATE_DESC& desc);

    std::vector<PsoDesc> m_psos;
    std::map<std::string, PsoDesc*> m_psoMap;
    std::map<std::filesystem::path, std::vector<PsoDesc*>> m_shadersPsos;

    std::vector<PsoDesc> m_computePsos;
    std::map<std::string, ComputePsoDesc*> m_computePsoMap;
    std::map<std::filesystem::path, std::vector<ComputePsoDesc*>> m_computeShadersPsos;

    Shader m_vsFallback;
    Shader m_psFallback;
    Shader m_csFallback;

    FileWatcher* m_shaderWatcher = nullptr;
};
}