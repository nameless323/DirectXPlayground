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

    void BeginFrame(const RenderContext& context);
    void Shutdown() const;
    void CreatePso(const RenderContext& context, const std::string& name, std::wstring shaderPath, D3D12_GRAPHICS_PIPELINE_STATE_DESC desc, const std::vector<DxcDefine>* defines = nullptr);
    void CreatePso(const RenderContext& context, const std::string& name, std::wstring shaderPath, D3D12_COMPUTE_PIPELINE_STATE_DESC desc, const std::vector<DxcDefine>* defines = nullptr);

    ID3D12PipelineState* GetPso(const std::string& name);

private:
    struct PsoDesc // todo: template + sfinae. but think about vectors
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc{};
        std::vector<DxcDefine> Defines;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> CompiledPso;

        void SetDefines(const std::vector<DxcDefine>* defines)
        {
            if (defines == nullptr)
                return;
            Defines = *defines;
        }
        std::vector<DxcDefine>* GetDefines()
        {
            if (Defines.empty())
                return nullptr;
            return &Defines;
        }
    };
    struct ComputePsoDesc
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC Desc{};
        std::vector<DxcDefine> Defines;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> CompiledPso;

        void SetDefines(const std::vector<DxcDefine>* defines)
        {
            if (defines == nullptr)
                return;
            Defines = *defines;
        }
        std::vector<DxcDefine>* GetDefines()
        {
            if (Defines.empty())
                return nullptr;
            return &Defines;
        }
    };
    static constexpr UINT MaxPso = 4096;

    void CompilePsoWithShader(const RenderContext& context, REFIID psoRiid, void** psoPpv, const std::wstring& shaderPath, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const std::vector<DxcDefine>* defines);
    void CompilePsoWithShader(const RenderContext& context, REFIID psoRiid, void** psoPpv, const std::wstring& shaderPath, D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, const std::vector<DxcDefine>* defines);

    std::vector<PsoDesc> mPsos;
    std::map<std::string, PsoDesc*> mPsoMap;
    std::map<std::filesystem::path, std::vector<PsoDesc*>> mShadersPsos;

    std::vector<ComputePsoDesc> mComputePsos;
    std::map<std::string, ComputePsoDesc*> mComputePsoMap;
    std::map<std::filesystem::path, std::vector<ComputePsoDesc*>> mComputeShadersPsos;

    Shader mVsFallback;
    Shader mPsFallback;
    Shader mCsFallback;

    FileWatcher* mShaderWatcher = nullptr;
};
}