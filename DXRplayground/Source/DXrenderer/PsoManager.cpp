#include "DXrenderer/PsoManager.h"

#include <cassert>
#include <thread>

#include "DXrenderer/Shader.h"
#include "DXrenderer/DXhelpers.h"
#include "Utils/FileWatcher.h"

namespace DirectxPlayground
{

PsoManager::PsoManager()
{
    m_psos.reserve(m_maxPso);
    std::wstring shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//Fallback.hlsl");
    bool vsSucceeded = Shader::CompileFromFile(shaderPath, "vs", "vs_5_1", m_vsFallback);
    bool psSucceeded = Shader::CompileFromFile(shaderPath, "ps", "ps_5_1", m_psFallback);
    assert(vsSucceeded && psSucceeded && "Fallback compilation failed");

    std::wstring s = LR"()";
    m_shaderWatcher = new FileWatcher(ASSETS_DIR_W + std::wstring(L"Shaders")); // Will be deleted in DirectoryModificationCallback in FileWatcher. In if (errorCode == ERROR_OPERATION_ABORTED)
    std::thread shaderWatcherThread(std::ref(*m_shaderWatcher));
    shaderWatcherThread.detach();
}

void PsoManager::BeginFrame(RenderContext& context)
{
    while (auto changedFile = m_shaderWatcher->GetModifiedFilesQueue().Pop())
    {
        const auto& path = *changedFile;
        auto psoVectorIt = m_shadersPsos.find(path);
        if (psoVectorIt != m_shadersPsos.end())
        {
            std::vector<PsoDesc*>& psoVector = psoVectorIt->second;
            for (auto psoDesc : psoVector)
            {
                CompilePsoWithShader(context, IID_PPV_ARGS(&psoDesc->CompiledPso), path, psoDesc->Desc);
            }
        }
    }
}

void PsoManager::Shutdown()
{
    m_shaderWatcher->Shutdown();
}

void PsoManager::CreatePso(RenderContext& context, std::string name, std::wstring shaderPath, D3D12_GRAPHICS_PIPELINE_STATE_DESC desc)
{
    assert(m_psoMap.find(name) == m_psoMap.end() && "PSO with the same name has already been created");
    assert(m_psos.size() < (m_maxPso - 1) && "PSO max size reached, it will lead to reallocation and to breaking all pointers saved in m_psoMap and m_shadersPsos");

    m_psos.emplace_back();
    PsoDesc* currPsoDesc = &m_psos.back();

    CompilePsoWithShader(context, IID_PPV_ARGS(&currPsoDesc->CompiledPso), shaderPath, desc);

    currPsoDesc->Desc = std::move(desc);
    m_psoMap[std::move(name)] = currPsoDesc;
    m_shadersPsos[std::move(shaderPath)].push_back(currPsoDesc);
}

ID3D12PipelineState* PsoManager::GetPso(const std::string& name)
{
    assert(m_psoMap.find(name) != m_psoMap.end() && "no pso with the required name has been created");
    return m_psoMap[name]->CompiledPso.Get();
}

void PsoManager::CompilePsoWithShader(RenderContext& context, REFIID psoRiid, void** psoPpv, const std::wstring& shaderPath, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
    Shader vs;
    bool vsSucceeded = Shader::CompileFromFile(shaderPath, "vs", "vs_5_1", vs);
    Shader ps;
    bool psSucceeded = Shader::CompileFromFile(shaderPath, "ps", "ps_5_1", ps);
    desc.VS = vsSucceeded ? vs.GetBytecode() : m_vsFallback.GetBytecode();
    desc.PS = psSucceeded ? ps.GetBytecode() : m_psFallback.GetBytecode();
    ThrowIfFailed(context.Device->CreateGraphicsPipelineState(&desc, psoRiid, psoPpv));
}

}