#pragma once

#include <d3d12.h>
#include <vector>
#include <DirectXMath.h>

#include "DXrenderer/DXhelpers.h"
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{

class Model;
class UnorderedAccessBuffer;
struct RenderContext;

namespace DXR
{
class AccelerationStructure
{
public:
    // TODO: mixed aabb/model
    AccelerationStructure(std::vector<Model*> models);
    ~AccelerationStructure();
    void Prebuild(RenderContext& context, const D3D12_RAYTRACING_GEOMETRY_DESC* defaultDesc = nullptr, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);
 
    void AddDescriptor(const AccelerationStructure& blas, const DirectX::XMFLOAT4X4& transform = IdentityMatrix, UINT instanceMask = 0, UINT instanceId = 0, UINT flags = 0, UINT contribToHitGroupIndex = 0);
    void AddDescriptor(D3D12_RAYTRACING_INSTANCE_DESC desc);

    void Build(RenderContext& context, UnorderedAccessBuffer* scratchBuffer, bool setUavBarrier = true);

    UINT64 GetResultMaxSize() const;
    UINT64 GetScratchSize() const;

private:
   std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_desc;
   std::vector<Model*> m_models;
   std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;

   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO m_prebuildInfo = {};
   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_buildDesc = {};

   std::vector<CD3DX12_RESOURCE_BARRIER> m_toNonPixelTransitions;
   std::vector<CD3DX12_RESOURCE_BARRIER> m_toIndexVertexTransitions;

   UnorderedAccessBuffer* m_buffer = nullptr;
   UploadBuffer* m_instanceDescsBuffer = nullptr;

   bool m_isPrebuilt = false;
   bool m_isBuilt = false;
};


}
}
