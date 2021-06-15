#pragma once

#include <d3d12.h>
#include <vector>

#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{

class Model;
struct RenderContext;

namespace DXR
{
class AccelerationStructure
{
public:
    // TODO: mixed aabb/model
    AccelerationStructure(std::vector<Model*> models);
    void Prebuild(RenderContext& context, const D3D12_RAYTRACING_GEOMETRY_DESC* descriptor = nullptr, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);

    UINT64 GetResultMaxSize() const;
    UINT64 GetScratchSize() const;

private:
   std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_desc;
   std::vector<Model*> m_models;
   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO m_prebuildInfo{};

   std::vector<CD3DX12_RESOURCE_BARRIER> m_toNonPixelTransitions;
   std::vector<CD3DX12_RESOURCE_BARRIER> m_toIndexVertexTransitions;
};
}
}
