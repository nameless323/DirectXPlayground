#include "DXrenderer/PsoManager.h"

#include <cassert>

#include "DXrenderer/Shader.h"
#include "DXrenderer/DXhelpers.h"

namespace DirectxPlayground
{

PsoManager::PsoManager()
{
    m_psos.reserve(m_maxPso);
}

void PsoManager::Init()
{

}

void PsoManager::Shutdown()
{

}

void PsoManager::CreatePso(RenderContext& context, std::string name, std::string shaderPath, D3D12_GRAPHICS_PIPELINE_STATE_DESC desc)
{
    assert(m_psoMap.find(name) == m_psoMap.end() && "PSO with the same name has already been created");
    assert(m_psos.size() < (m_maxPso - 1) && "PSO max size reached, it will lead to reallocation and to breaking all pointers saved in m_psoMap and m_shadersPsos");
    Shader vs = Shader::CompileFromFile(shaderPath, "vs", "vs_5_1");
    Shader ps = Shader::CompileFromFile(shaderPath, "ps", "ps_5_1");
    desc.VS = vs.GetBytecode();
    desc.PS = ps.GetBytecode();

    m_psos.emplace_back();
    PsoDesc* currPsoDesc = &m_psos.back();
    currPsoDesc->Desc = std::move(desc);
    ThrowIfFailed(context.Device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&currPsoDesc->CompiledPso)));
    m_psoMap[std::move(name)] = currPsoDesc;
    m_shadersPsos[std::move(shaderPath)].push_back(currPsoDesc);
}

ID3D12PipelineState* PsoManager::GetPso(const std::string& name)
{
    assert(m_psoMap.find(name) != m_psoMap.end() && "no pso with the required name has been created");
    return m_psoMap[name]->CompiledPso.Get();
}

}