#include "DXrenderer/DXR/AccelerationStructure.h"

#include "DXrenderer/Model.h"

namespace DirectxPlayground::DXR
{

AccelerationStructure::AccelerationStructure(std::vector<Model*> models)
{
    std::swap(models, m_models);
}

void AccelerationStructure::Prebuild(RenderContext& context, const D3D12_RAYTRACING_GEOMETRY_DESC* descriptor /*= nullptr*/, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags /*= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE*/)
{
    D3D12_RAYTRACING_GEOMETRY_DESC desc{};
    if (descriptor != nullptr)
    {
        desc = *descriptor;
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

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& buildDescInputs = buildDesc.Inputs;
    buildDescInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildDescInputs.Flags = buildFlags;
    buildDescInputs.NumDescs = UINT(m_desc.size());
    buildDescInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    buildDescInputs.pGeometryDescs = m_desc.data();

    context.Device->GetRaytracingAccelerationStructurePrebuildInfo(&buildDescInputs, &m_prebuildInfo);
    assert(m_prebuildInfo.ResultDataMaxSizeInBytes > 0);
}

}