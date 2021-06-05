#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

#include "Buffers/HeapBuffer.h"
#include "Buffers/UploadBuffer.h"
#include "Utils/Helpers.h"

namespace tinygltf
{
class Model;
class Node;
struct Primitive;
struct Mesh;
}

namespace DirectxPlayground
{
using namespace DirectX;

struct RenderContext;
class TextureManager;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT2 Uv;
    XMFLOAT3 Norm;
    XMFLOAT4 Tangent;
};

struct Image
{
    UINT IndexInHeap = 0;
    std::string Name;
};

struct Material
{
    int BaseColorTexture = 0;
    int MetallicRoughnessTexture = 0;
    int NormalTexture = 0;
    int OcclusionTexture = 0;
    float BaseColorFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

class Model
{
public:
    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh()
        {
            SafeDelete(m_indexBuffer);
            SafeDelete(m_vertexBuffer);
            SafeDelete(m_materialBuffer);
        }

        UINT GetIndexCount() const
        {
            return UINT(m_indices.size());
        }
        UINT GetVertexCount() const
        {
            return UINT(m_vertices.size());
        }

        const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const
        {
            return m_vertexBuffer->GetVertexBufferView();
        }
        const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const
        {
            return m_indexBuffer->GetIndexBufferView();
        }
        D3D12_GPU_VIRTUAL_ADDRESS GetVertexBufferGpuAddress() const
        {
            return m_vertexBuffer->GetVertexBuffer()->GetGPUVirtualAddress();
        }
        D3D12_GPU_VIRTUAL_ADDRESS GetIndexBufferGpuAddress() const
        {
            return m_indexBuffer->GetIndexBuffer()->GetGPUVirtualAddress();
        }

        D3D12_GPU_VIRTUAL_ADDRESS GetMaterialBufferGpuAddress(UINT frame) const
        {
            return m_materialBuffer->GetFrameDataGpuAddress(frame);
        }

        void UpdateMaterialBuffer(UINT frame)
        {
            m_materialBuffer->UploadData(frame, m_material);
        }

    private:
        friend class Model;

        UINT m_indexCount = 0;
        Material m_material{};

        std::vector<Vertex> m_vertices;
        std::vector<UINT> m_indices;

        VertexBuffer* m_vertexBuffer = nullptr;
        IndexBuffer* m_indexBuffer = nullptr;

        UploadBuffer* m_materialBuffer = nullptr;
    };

    Model(RenderContext& ctx, const std::string& path);
    Model(RenderContext& ctx, std::vector<Vertex> vertices, std::vector<UINT> indices);
    ~Model();

    UINT GetIndexCount() const;

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const;
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const;

    const std::vector<Mesh*>& GetMeshes() const;
    void UpdateMeshes(UINT frame);

private:
    void LoadModel(const std::string& path, tinygltf::Model& model);
    void ParseModelNodes(RenderContext& ctx, const tinygltf::Model& model, const tinygltf::Node& node);
    void ParseGLTFMesh(RenderContext& ctx, const tinygltf::Model& model, const tinygltf::Node& node, const tinygltf::Mesh& mesh);
    void ParseVertices(Mesh* mesh, const tinygltf::Model& model, const tinygltf::Node& node, const tinygltf::Primitive& primitive);
    void ParseIndices(Mesh* mesh, const tinygltf::Model& model, const tinygltf::Primitive& primitive);

    std::vector<Mesh*> m_meshes;
    std::vector<Image> m_images;
    std::vector<int> m_textures;
    std::vector<Material> m_materials;
};

inline UINT Model::GetIndexCount() const
{
    return m_meshes[0]->GetIndexCount();
}

inline const D3D12_VERTEX_BUFFER_VIEW& Model::GetVertexBufferView() const
{
    return m_meshes[0]->GetVertexBufferView();
}

inline const D3D12_INDEX_BUFFER_VIEW& Model::GetIndexBufferView() const
{
    return m_meshes[0]->GetIndexBufferView();
}

inline const std::vector<Model::Mesh*>& Model::GetMeshes() const
{
    return m_meshes;
}
}
