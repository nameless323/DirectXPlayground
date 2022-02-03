#include "DXrenderer/DXR/AccelerationStructure.h"

#include "DXrenderer/Model.h"

namespace DirectxPlayground::DXR
{

AccelerationStructure::~AccelerationStructure()
{
    SafeDelete(mBuffer);
}

void AccelerationStructure::Build(RenderContext& context, UnorderedAccessBuffer* scratchBuffer, bool setUavBarrier)
{
    mBuildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGpuAddress();
    mBuildDesc.DestAccelerationStructureData = mBuffer->GetGpuAddress();

    context.CommandList->BuildRaytracingAccelerationStructure(&mBuildDesc, 0, nullptr);
    if (setUavBarrier) {
      D3D12_RESOURCE_BARRIER b = CD3DX12_RESOURCE_BARRIER::UAV(mBuffer->GetResource());
      context.CommandList->ResourceBarrier(1, &b);
    }
    mIsBuilt = true;
}

UINT64 AccelerationStructure::GetMaxScratchSize(std::vector<AccelerationStructure*> structs)
{
    UINT64 res = 0;
    for (auto s : structs)
        res = std::max(res, s->GetScratchSize());
    return res;
}

//////////////////////////////////////////////////////////////////////////
/// BLAS
//////////////////////////////////////////////////////////////////////////


BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(RenderContext& context, std::vector<Model*> models, std::vector<D3D12_RAYTRACING_AABB> aabbs)
{
    std::swap(models, m_models);
    m_desc.reserve(16);

    m_aabbsCount = UINT(aabbs.size());
    if (m_aabbsCount > 0)
    {
        m_aabbUploadBuffer = new UploadBuffer(*context.Device, sizeof(D3D12_RAYTRACING_AABB) * UINT(aabbs.size()), false, 1);
        m_aabbUploadBuffer->UploadData(0, reinterpret_cast<byte*>(aabbs.data()));

        CD3DX12_HEAP_PROPERTIES hProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC rDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(D3D12_RAYTRACING_AABB));
        HRESULT hr = context.Device->CreateCommittedResource(
            &hProps,
            D3D12_HEAP_FLAG_NONE,
            &rDesc,
            m_aabbResource.GetCurrentState(),
            nullptr,
            IID_PPV_ARGS(m_aabbResource.GetAddressOf())
        );
        context.CommandList->CopyResource(m_aabbResource.Get(), m_aabbUploadBuffer->GetResource());

        std::vector<CD3DX12_RESOURCE_BARRIER> transitions;
        transitions.push_back(m_aabbResource.GetBarrier(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
        context.CommandList->ResourceBarrier(UINT(transitions.size()), transitions.data());
    }
}


BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(RenderContext& context, std::vector<Model*> models)
    : BottomLevelAccelerationStructure(context, models, {})
{
}

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(RenderContext& context, Model* model)
    : BottomLevelAccelerationStructure(context, std::vector<Model*>{ model }, {})
{
}

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(RenderContext& context, std::vector<D3D12_RAYTRACING_AABB> aabbs)
    : BottomLevelAccelerationStructure(context, {}, std::move(aabbs))
{
}

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(RenderContext& context, D3D12_RAYTRACING_AABB aabb)
    : BottomLevelAccelerationStructure(context, {}, std::vector<D3D12_RAYTRACING_AABB>{ aabb })
{
}

BottomLevelAccelerationStructure::~BottomLevelAccelerationStructure()
{
    SafeDelete(m_aabbUploadBuffer);
}

void BottomLevelAccelerationStructure::Prebuild(RenderContext& context, const D3D12_RAYTRACING_GEOMETRY_DESC* defaultDesc /*= nullptr*/, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags /*= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE*/)
{
    assert(!mIsBuilt);
    assert(!mIsPrebuilt);

    D3D12_RAYTRACING_GEOMETRY_DESC desc{};
    if (defaultDesc != nullptr)
    {
        desc = *defaultDesc;
    }
    else
    {
        desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
        desc.Triangles.Transform3x4 = 0;
        desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        desc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
        desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    }
    for (const auto model : m_models)
    {
        for (const auto mesh : model->GetMeshes())
        {
            m_toNonPixelTransitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(mesh->GetIndexBufferResource(), D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
            m_toNonPixelTransitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(mesh->GetVertexBufferResource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

            m_toIndexVertexTransitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(mesh->GetIndexBufferResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_INDEX_BUFFER));
            m_toIndexVertexTransitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(mesh->GetVertexBufferResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

            desc.Triangles.IndexBuffer = mesh->GetIndexBufferGpuAddress();
            desc.Triangles.IndexCount = mesh->GetIndexCount();
            desc.Triangles.VertexCount = mesh->GetVertexCount();
            desc.Triangles.VertexBuffer.StartAddress = mesh->GetVertexBufferGpuAddress();
            m_desc.push_back(desc);
        }
    }

    if (m_aabbsCount > 0)
    {
        D3D12_RAYTRACING_GEOMETRY_DESC aabbDesc;
        aabbDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
        aabbDesc.AABBs.AABBCount = m_aabbsCount;
        aabbDesc.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
        aabbDesc.AABBs.AABBs.StartAddress = m_aabbResource.Get()->GetGPUVirtualAddress();
        aabbDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

        m_desc.push_back(aabbDesc);
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& buildDescInputs = mBuildDesc.Inputs;
    buildDescInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildDescInputs.Flags = buildFlags;
    buildDescInputs.NumDescs = UINT(m_desc.size());
    buildDescInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    buildDescInputs.pGeometryDescs = m_desc.data();

    context.Device->GetRaytracingAccelerationStructurePrebuildInfo(&buildDescInputs, &mPrebuildInfo);
    assert(mPrebuildInfo.ResultDataMaxSizeInBytes > 0);
    mBuffer = new UnorderedAccessBuffer(context.CommandList, *context.Device, UINT(mPrebuildInfo.ResultDataMaxSizeInBytes), nullptr, false, true);

    mIsPrebuilt = true;
}


void BottomLevelAccelerationStructure::Build(RenderContext& context, UnorderedAccessBuffer* scratchBuffer, bool setUavBarrier)
{
    if (m_toNonPixelTransitions.size() > 0)
        context.CommandList->ResourceBarrier(UINT(m_toNonPixelTransitions.size()), m_toNonPixelTransitions.data());
    AccelerationStructure::Build(context, scratchBuffer, setUavBarrier);
    if (m_toIndexVertexTransitions.size() > 0)
        context.CommandList->ResourceBarrier(UINT(m_toIndexVertexTransitions.size()), m_toIndexVertexTransitions.data());
}

//////////////////////////////////////////////////////////////////////////
/// TLAS
//////////////////////////////////////////////////////////////////////////

TopLevelAccelerationStructure::TopLevelAccelerationStructure()
{
    m_instanceDescs.reserve(128);
}
TopLevelAccelerationStructure::~TopLevelAccelerationStructure()
{
    SafeDelete(m_instanceDescsBuffer);
}

void TopLevelAccelerationStructure::AddDescriptor(const BottomLevelAccelerationStructure& blas, const DirectX::XMFLOAT4X4& transform /* = Identity */, UINT instanceMask /* = 0 */, UINT instanceId /* = 0 */, UINT flags /* = 0 */, UINT contribToHitGroupIndex /* = 0 */)
{
    assert(!mIsBuilt);
    assert(!mIsPrebuilt);

    D3D12_RAYTRACING_INSTANCE_DESC desc{};
    desc.Transform[0][0] = transform._11; desc.Transform[0][1] = transform._12; desc.Transform[0][2] = transform._13; desc.Transform[0][3] = transform._14;
    desc.Transform[1][0] = transform._21; desc.Transform[1][1] = transform._22; desc.Transform[1][2] = transform._23; desc.Transform[1][3] = transform._24;
    desc.Transform[2][0] = transform._31; desc.Transform[2][1] = transform._32; desc.Transform[2][2] = transform._33; desc.Transform[2][3] = transform._34;

    desc.InstanceMask = instanceMask;
    desc.InstanceID = instanceId;
    desc.Flags = flags;
    desc.InstanceContributionToHitGroupIndex = contribToHitGroupIndex;
    desc.AccelerationStructure = blas.GetBuffer()->GetGpuAddress();

    m_instanceDescs.push_back(std::move(desc));
}

void TopLevelAccelerationStructure::AddDescriptor(D3D12_RAYTRACING_INSTANCE_DESC desc)
{
    assert(!mIsBuilt);
    assert(!mIsPrebuilt);

    m_instanceDescs.push_back(std::move(desc));
}

void TopLevelAccelerationStructure::Prebuild(RenderContext& context, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags /* = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE*/)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& buildDescInputs = mBuildDesc.Inputs;
    buildDescInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildDescInputs.Flags = buildFlags;
    buildDescInputs.NumDescs = UINT(m_instanceDescs.size());
    buildDescInputs.pGeometryDescs = nullptr;
    buildDescInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    context.Device->GetRaytracingAccelerationStructurePrebuildInfo(&buildDescInputs, &mPrebuildInfo);
    assert(mPrebuildInfo.ResultDataMaxSizeInBytes > 0);
    mBuffer = new UnorderedAccessBuffer(context.CommandList, *context.Device, UINT(mPrebuildInfo.ResultDataMaxSizeInBytes), nullptr, false, true);
    m_instanceDescsBuffer = new UploadBuffer(*context.Device, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * UINT(m_instanceDescs.size()), false, 1);
    m_instanceDescsBuffer->UploadData(0, reinterpret_cast<byte*>(m_instanceDescs.data()));

    mIsPrebuilt = true;
}

void TopLevelAccelerationStructure::Build(RenderContext& context, UnorderedAccessBuffer* scratchBuffer, bool setUavBarrier)
{
    mBuildDesc.Inputs.InstanceDescs = m_instanceDescsBuffer->GetFrameDataGpuAddress(0);
    AccelerationStructure::Build(context, scratchBuffer, setUavBarrier);
}

}