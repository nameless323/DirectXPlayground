#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

#include "Buffers/HeapBuffer.h"

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
    XMFLOAT3 Norm;
    XMFLOAT2 Uv;
};

struct Image
{
    UINT IndexInHeap = 0;
    std::string Name;
};

class Mesh
{
public:
    class Submesh
    {
    public:
        Submesh() = default;
        ~Submesh()
        {
            SafeDelete(m_indexBuffer);
            SafeDelete(m_vertexBuffer);
        }

        UINT GetIndexCount() const
        {
            return m_indexCount;
        }

        const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const
        {
            return m_vertexBuffer->GetVertexBufferView();
        }
        const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const
        {
            return m_indexBuffer->GetIndexBufferView();
        }

    private:
        friend class Mesh;

        UINT m_indexCount = 0;
        int m_material = 0;

        std::vector<Vertex> m_vertices;
        std::vector<UINT> m_indices;

        std::vector<Image> m_images;

        VertexBuffer* m_vertexBuffer = nullptr;
        IndexBuffer* m_indexBuffer = nullptr;

    };

    Mesh(RenderContext& ctx, const std::string& path);
    Mesh(RenderContext& ctx, std::vector<Vertex> vertices, std::vector<UINT> indices);
    ~Mesh();

    UINT GetIndexCount() const;

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const;
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const;

    const std::vector<Submesh*>& GetSubmeshes() const;

private:
    void LoadModel(const std::string& path, tinygltf::Model& model);
    void ParseModelNodes(RenderContext& ctx, const tinygltf::Model& model, const tinygltf::Node& node);
    void ParseGLTFMesh(RenderContext& ctx, const tinygltf::Model& model, const tinygltf::Node& node, const tinygltf::Mesh& mesh);
    void ParseVertices(Submesh* submesh, const tinygltf::Model& model, const tinygltf::Node& node, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);
    void ParseIndices(Submesh* submesh, const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);

    std::vector<Submesh*> m_submeshes;
};

inline UINT Mesh::GetIndexCount() const
{
    return m_submeshes[0]->GetIndexCount();
}

inline const D3D12_VERTEX_BUFFER_VIEW& Mesh::GetVertexBufferView() const
{
    return m_submeshes[0]->GetVertexBufferView();
}

inline const D3D12_INDEX_BUFFER_VIEW& Mesh::GetIndexBufferView() const
{
    return m_submeshes[0]->GetIndexBufferView();
}

inline const std::vector<Mesh::Submesh*>& Mesh::GetSubmeshes() const
{
    return m_submeshes;
}
}
