#pragma once

#include <d3d12.h>
#include <vector>
#include <DirectXMath.h>

#include "DXrenderer/DXhelpers.h"
#include "DXrenderer/Buffers/UploadBuffer.h"
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{

class Model;
class UnorderedAccessBuffer;
class UploadBuffer;
struct RenderContext;

namespace DXR
{
class AccelerationStructure
{
public:
    virtual ~AccelerationStructure();

    virtual void Build(RenderContext& context, UnorderedAccessBuffer* scratchBuffer, bool setUavBarrier);
    void SetName(const std::wstring& name);

    UnorderedAccessBuffer* GetBuffer() const;

    UINT64 GetResultMaxSize() const;
    UINT64 GetScratchSize() const;

    UINT64 UpdateScratchSize(AccelerationStructure& other);

    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const;

    static UINT64 GetMaxScratchSize(std::vector<AccelerationStructure*> structs);

protected:
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO m_prebuildInfo = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_buildDesc = {};

    UnorderedAccessBuffer* m_buffer = nullptr;

    bool m_isPrebuilt = false;
    bool m_isBuilt = false;
};

inline UnorderedAccessBuffer* AccelerationStructure::GetBuffer() const
{
    return m_buffer;
}

inline UINT64 AccelerationStructure::GetResultMaxSize() const
{
    assert(m_isPrebuilt);
    return m_prebuildInfo.ResultDataMaxSizeInBytes;
}

inline UINT64 AccelerationStructure::GetScratchSize() const
{
    assert(m_isPrebuilt);
    return m_prebuildInfo.ScratchDataSizeInBytes;
}

inline UINT64 AccelerationStructure::UpdateScratchSize(AccelerationStructure& other)
{
    assert(m_isPrebuilt);
    assert(other.m_isPrebuilt);
    return std::max(other.GetScratchSize(), GetScratchSize());
}

inline void AccelerationStructure::SetName(const std::wstring& name)
{
    m_buffer->SetName(name);
}

inline D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructure::GetGpuAddress() const
{
    return m_buffer->GetGpuAddress();
}

//////////////////////////////////////////////////////////////////////////
/// BLAS
//////////////////////////////////////////////////////////////////////////

class BottomLevelAccelerationStructure : public AccelerationStructure
{
public:
    BottomLevelAccelerationStructure(RenderContext& context, std::vector<Model*> models, std::vector<D3D12_RAYTRACING_AABB> aabbs);
    BottomLevelAccelerationStructure(RenderContext& context, std::vector<Model*> models);
    BottomLevelAccelerationStructure(RenderContext& context, Model* model);
    BottomLevelAccelerationStructure(RenderContext& context, std::vector<D3D12_RAYTRACING_AABB> aabbs);
    BottomLevelAccelerationStructure(RenderContext& context, D3D12_RAYTRACING_AABB aabb);

    ~BottomLevelAccelerationStructure();
    void Prebuild(RenderContext& context, const D3D12_RAYTRACING_GEOMETRY_DESC* defaultDesc = nullptr, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);
    void Build(RenderContext& context, UnorderedAccessBuffer* scratchBuffer, bool setUavBarrier) override;

private:
   std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_desc;
   std::vector<Model*> m_models;
   UINT m_aabbsCount = 0;

   Microsoft::WRL::ComPtr<ID3D12Resource> m_aabbResource;
   UploadBuffer* m_aabbUploadBuffer = nullptr;

   std::vector<CD3DX12_RESOURCE_BARRIER> m_toNonPixelTransitions;
   std::vector<CD3DX12_RESOURCE_BARRIER> m_toIndexVertexTransitions;
};

//////////////////////////////////////////////////////////////////////////
/// TLAS
//////////////////////////////////////////////////////////////////////////

class TopLevelAccelerationStructure : public AccelerationStructure
{
public:
    TopLevelAccelerationStructure();
    ~TopLevelAccelerationStructure();

    void AddDescriptor(const BottomLevelAccelerationStructure& blas, const DirectX::XMFLOAT4X4& transform = IdentityMatrix, UINT instanceMask = 0, UINT instanceId = 0, UINT flags = 0, UINT contribToHitGroupIndex = 0);
    void AddDescriptor(D3D12_RAYTRACING_INSTANCE_DESC desc);

    void Prebuild(RenderContext& context, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);
    void Build(RenderContext& context, UnorderedAccessBuffer* scratchBuffer, bool setUavBarrier) override;

private:
   std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;
   UploadBuffer* m_instanceDescsBuffer = nullptr;
};
}
}
