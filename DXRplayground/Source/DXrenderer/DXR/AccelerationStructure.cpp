#include "DXrenderer/DXR/AccelerationStructure.h"

#include "DXrenderer/Model.h"

namespace DirectxPlayground::DXR
{

AccelerationStructure::AccelerationStructure(std::vector<Model*> models)
{
    std::swap(models, m_models);
    m_desc.reserve(16);
    m_instanceDescs.reserve(128);
}

AccelerationStructure::~AccelerationStructure()
{
    SafeDelete(m_buffer);
    SafeDelete(m_instanceDescsBuffer);
}

void AccelerationStructure::Prebuild(RenderContext& context, const D3D12_RAYTRACING_GEOMETRY_DESC* defaultDesc /*= nullptr*/, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags /*= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE*/)
{
    assert(!m_isBuilt);
    assert(!m_isPrebuilt);

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

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& buildDescInputs = m_buildDesc.Inputs;
    buildDescInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildDescInputs.Flags = buildFlags;
    buildDescInputs.NumDescs = UINT(m_desc.size());
    buildDescInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    buildDescInputs.pGeometryDescs = m_desc.data();

    context.Device->GetRaytracingAccelerationStructurePrebuildInfo(&buildDescInputs, &m_prebuildInfo);
    assert(m_prebuildInfo.ResultDataMaxSizeInBytes > 0);
    m_buffer = new UnorderedAccessBuffer(context.CommandList, *context.Device, UINT(m_prebuildInfo.ResultDataMaxSizeInBytes), nullptr, false, true);

    m_isPrebuilt = true;
}

void AccelerationStructure::AddDescriptor(const AccelerationStructure& blas, const DirectX::XMFLOAT4X4& transform /* = Identity */, UINT instanceMask /* = 0 */, UINT instanceId /* = 0 */, UINT flags /* = 0 */, UINT contribToHitGroupIndex /* = 0 */)
{
    assert(!m_isBuilt);
    assert(m_isPrebuilt);

    D3D12_RAYTRACING_INSTANCE_DESC desc{};
    desc.Transform[0][0] = transform._11; desc.Transform[0][1] = transform._12; desc.Transform[0][2] = transform._13; desc.Transform[0][3] = transform._14;
    desc.Transform[1][0] = transform._21; desc.Transform[1][1] = transform._22; desc.Transform[1][2] = transform._23; desc.Transform[1][3] = transform._24;
    desc.Transform[2][0] = transform._31; desc.Transform[2][1] = transform._32; desc.Transform[2][2] = transform._33; desc.Transform[2][3] = transform._34;

    desc.InstanceMask = instanceMask;
    desc.InstanceID = instanceId;
    desc.Flags = flags;
    desc.InstanceContributionToHitGroupIndex = contribToHitGroupIndex;
    desc.AccelerationStructure = blas.m_buffer->GetGpuAddress();

    m_instanceDescs.push_back(std::move(desc));
}

void AccelerationStructure::AddDescriptor(D3D12_RAYTRACING_INSTANCE_DESC desc)
{
    assert(!m_isBuilt);
    assert(m_isPrebuilt);

    desc.AccelerationStructure = m_buffer->GetGpuAddress();
    m_instanceDescs.push_back(std::move(desc));
}

void AccelerationStructure::Build(RenderContext& context, UnorderedAccessBuffer* scratchBuffer, bool setUavBarrier /* = true */)
{
    m_buildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGpuAddress();
    m_buildDesc.DestAccelerationStructureData = m_buffer->GetGpuAddress();

    context.CommandList->BuildRaytracingAccelerationStructure(&m_buildDesc, 0, nullptr);
    if (setUavBarrier)
        context.CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_buffer->GetResource()));
    context.CommandList->ResourceBarrier(UINT(m_toIndexVertexTransitions.size()), m_toIndexVertexTransitions.data());
}

}