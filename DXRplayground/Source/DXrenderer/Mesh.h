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

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT3 Norm;
    XMFLOAT2 Uv;
};

class Mesh
{
public:
    Mesh(RenderContext& ctx, const std::string& path);
    Mesh(RenderContext& ctx, std::vector<Vertex> vertices, std::vector<UINT> indices);
    ~Mesh();

    UINT GetIndexCount() const;

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const;
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const;

private:
    void LoadModel(const std::string& path, tinygltf::Model& model);
    void ParseModelNodes(const tinygltf::Model& model, const tinygltf::Node& node);
    void ParseGLTFMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh);
    void ParseVertices(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);
    void ParseIndices(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);

    UINT m_indexCount = 0;

    std::vector<Vertex> m_vertices;
    std::vector<UINT> m_indices;

    VertexBuffer* m_vertexBuffer = nullptr;
    IndexBuffer* m_indexBuffer = nullptr;
};

inline UINT Mesh::GetIndexCount() const
{
    return m_indexCount;
}

inline const D3D12_VERTEX_BUFFER_VIEW& Mesh::GetVertexBufferView() const
{
    return m_vertexBuffer->GetVertexBufferView();
}

inline const D3D12_INDEX_BUFFER_VIEW& Mesh::GetIndexBufferView() const
{
    return m_indexBuffer->GetIndexBufferView();
}

}
