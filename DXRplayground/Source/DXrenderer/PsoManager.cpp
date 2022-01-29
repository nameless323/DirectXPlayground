#include "DXrenderer/PsoManager.h"

#include <cassert>
#include <thread>
#include <set>

#include "DXrenderer/Shader.h"
#include "DXrenderer/DXhelpers.h"
#include "Utils/FileWatcher.h"

#include "Utils/Logger.h"

namespace DirectxPlayground
{

PsoManager::PsoManager()
{
    mPsos.reserve(MaxPso);
    mComputePsos.reserve(MaxPso);
    std::wstring shaderPath = ASSETS_DIR_W + std::wstring(L"Shaders//Fallback.hlsl");
    bool vsSucceeded = Shader::CompileFromFile(shaderPath, L"vs", L"vs_6_6", mVsFallback);
    bool psSucceeded = Shader::CompileFromFile(shaderPath, L"ps", L"ps_6_6", mPsFallback);
    bool csSucceeded = Shader::CompileFromFile(shaderPath, L"cs", L"cs_6_6", mCsFallback);

    assert(vsSucceeded && psSucceeded && csSucceeded && "Fallback compilation failed");

    std::wstring s = LR"()";
    mShaderWatcher = new FileWatcher(ASSETS_DIR_W + std::wstring(L"Shaders")); // Will be deleted in DirectoryModificationCallback in FileWatcher. In if (errorCode == ERROR_OPERATION_ABORTED)
    std::thread shaderWatcherThread(std::ref(*mShaderWatcher));
    shaderWatcherThread.detach();
}

void PsoManager::BeginFrame(RenderContext& context)
{
    std::set<std::filesystem::path> processedFiles;
    while (auto changedFile = mShaderWatcher->GetModifiedFilesQueue().Pop())
    {
        const auto& path = *changedFile;
        if (processedFiles.find(path) != processedFiles.end())
            continue;
        processedFiles.insert(std::filesystem::path{ path });

        if (auto psoVectorIt = mShadersPsos.find(path); psoVectorIt != mShadersPsos.end())
        {
            std::vector<PsoDesc*>& psoVector = psoVectorIt->second;
            for (auto psoDesc : psoVector)
            {
                CompilePsoWithShader(context, IID_PPV_ARGS(&psoDesc->CompiledPso), path, psoDesc->Desc);
            }
        }
        if (auto psoVectorIt = mComputeShadersPsos.find(path); psoVectorIt != mComputeShadersPsos.end())
        {
            std::vector<ComputePsoDesc*>& psoVector = psoVectorIt->second;
            for (auto psoDesc : psoVector)
            {
                CompilePsoWithShader(context, IID_PPV_ARGS(&psoDesc->CompiledPso), path, psoDesc->Desc);
            }
        }
        LOG_W("Shader with the path \"", path, "\" has been recompiled \n");
    }
}

void PsoManager::Shutdown()
{
    mShaderWatcher->Shutdown();
}

void PsoManager::CreatePso(RenderContext& context, std::string name, std::wstring shaderPath, D3D12_GRAPHICS_PIPELINE_STATE_DESC desc)
{
    assert(mPsoMap.find(name) == mPsoMap.end() && "PSO with the same name has already been created");
    assert(mComputePsoMap.find(name) == mComputePsoMap.end() && "PSO with the same name has already been created");
    assert(mPsos.size() < (MaxPso - 1) && "PSO max size reached, it will lead to reallocation and to breaking all pointers saved in m_psoMap and m_shadersPsos");

    mPsos.emplace_back();
    PsoDesc* currPsoDesc = &mPsos.back();

    CompilePsoWithShader(context, IID_PPV_ARGS(&currPsoDesc->CompiledPso), shaderPath, desc);

    currPsoDesc->Desc = std::move(desc);
    mPsoMap[std::move(name)] = currPsoDesc;
    mShadersPsos[std::move(shaderPath)].push_back(currPsoDesc);
}

void PsoManager::CreatePso(RenderContext& context, std::string name, std::wstring shaderPath, D3D12_COMPUTE_PIPELINE_STATE_DESC desc)
{
    assert(mPsoMap.find(name) == mPsoMap.end() && "PSO with the same name has already been created");
    assert(mComputePsoMap.find(name) == mComputePsoMap.end() && "PSO with the same name has already been created");
    assert(mComputePsos.size() < (MaxPso - 1) && "PSO max size reached, it will lead to reallocation and to breaking all pointers saved in m_psoMap and m_shadersPsos");

    mComputePsos.emplace_back();
    ComputePsoDesc* currPsoDesc = &mComputePsos.back();

    CompilePsoWithShader(context, IID_PPV_ARGS(&currPsoDesc->CompiledPso), shaderPath, desc);

    currPsoDesc->Desc = std::move(desc);
    mComputePsoMap[std::move(name)] = currPsoDesc;
    mComputeShadersPsos[std::move(shaderPath)].push_back(currPsoDesc);
}

ID3D12PipelineState* PsoManager::GetPso(const std::string& name)
{
    if (auto graphicsPso = mPsoMap.find(name); graphicsPso != mPsoMap.end())
        return graphicsPso->second->CompiledPso.Get();
    if (auto computePso = mComputePsoMap.find(name); computePso != mComputePsoMap.end())
        return computePso->second->CompiledPso.Get();

    assert(false && "no pso with the required name has been created");
    return nullptr;
}

void PsoManager::CompilePsoWithShader(RenderContext& context, REFIID psoRiid, void** psoPpv, const std::wstring& shaderPath, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
    Shader vs;
    bool vsSucceeded = Shader::CompileFromFile(shaderPath, L"vs", L"vs_6_6", vs);
    Shader ps;
    bool psSucceeded = Shader::CompileFromFile(shaderPath, L"ps", L"ps_6_6", ps);
    desc.VS = vsSucceeded ? vs.GetBytecode() : mVsFallback.GetBytecode();
    desc.PS = psSucceeded ? ps.GetBytecode() : mPsFallback.GetBytecode();
    ThrowIfFailed(context.Device->CreateGraphicsPipelineState(&desc, psoRiid, psoPpv));
}

void PsoManager::CompilePsoWithShader(RenderContext& context, REFIID psoRiid, void** psoPpv, const std::wstring& shaderPath, D3D12_COMPUTE_PIPELINE_STATE_DESC& desc)
{
    Shader cs;
    bool succeeded = Shader::CompileFromFile(shaderPath, L"cs", L"cs_6_6", cs);
    desc.CS = succeeded ? cs.GetBytecode() : mCsFallback.GetBytecode();
    ThrowIfFailed(context.Device->CreateComputePipelineState(&desc, psoRiid, psoPpv));
}

}