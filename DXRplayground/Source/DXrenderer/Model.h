#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

#include "Buffers/HeapBuffer.h"
#include "Buffers/UploadBuffer.h"
#include "Utils/Helpers.h"
#include "Utils/Asset.h"

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

class Model : public Asset
{
public:
    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh()
        {
            SafeDelete(mIndexBuffer);
            SafeDelete(mVertexBuffer);
            SafeDelete(mMaterialBuffer);
        }

        UINT GetIndexCount() const
        {
            return UINT(mIndices.size());
        }
        UINT GetVertexCount() const
        {
            return UINT(mVertices.size());
        }

        const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const
        {
            return mVertexBuffer->GetVertexBufferView();
        }

        const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const
        {
            return mIndexBuffer->GetIndexBufferView();
        }

        D3D12_GPU_VIRTUAL_ADDRESS GetVertexBufferGpuAddress() const
        {
            return mVertexBuffer->GetVertexBuffer()->GetGPUVirtualAddress();
        }

        D3D12_GPU_VIRTUAL_ADDRESS GetIndexBufferGpuAddress() const
        {
            return mIndexBuffer->GetIndexBuffer()->GetGPUVirtualAddress();
        }

        D3D12_GPU_VIRTUAL_ADDRESS GetMaterialBufferGpuAddress(UINT frame) const
        {
            return mMaterialBuffer->GetFrameDataGpuAddress(frame);
        }

        ID3D12Resource* GetIndexBufferResource() const
        {
            return mIndexBuffer->GetIndexBuffer();
        }

        ID3D12Resource* GetVertexBufferResource() const
        {
            return mVertexBuffer->GetVertexBuffer();
        }

        void UpdateMaterialBuffer(UINT frame)
        {
            mMaterialBuffer->UploadData(frame, mMaterial);
        }

    private:
        friend class Model;

        UINT mIndexCount = 0;
        Material mMaterial{};

        std::vector<Vertex> mVertices;
        std::vector<UINT> mIndices;

        VertexBuffer* mVertexBuffer = nullptr;
        IndexBuffer* mIndexBuffer = nullptr;

        UploadBuffer* mMaterialBuffer = nullptr;
    };

    Model(RenderContext& ctx, const std::string& path);
    Model(RenderContext& ctx, std::vector<Vertex> vertices, std::vector<UINT> indices);
    ~Model();

    UINT GetIndexCount() const;

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const;
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const;
    const Mesh* GetMesh() const;

    const std::vector<Mesh*>& GetMeshes() const;
    void UpdateMeshes(UINT frame);

    void Parse(const std::string& filename) override;
    void Serialize(BinaryContainer& container) override;
    void Deserialize(BinaryContainer& container) override;

private:
    void LoadModel(const std::string& path, tinygltf::Model& model);
    void ParseModelNodes(RenderContext& ctx, const tinygltf::Model& model, const tinygltf::Node& node);
    void ParseGLTFMesh(RenderContext& ctx, const tinygltf::Model& model, const tinygltf::Node& node, const tinygltf::Mesh& mesh);
    void ParseVertices(Mesh* mesh, const tinygltf::Model& model, const tinygltf::Node& node, const tinygltf::Primitive& primitive);
    void ParseIndices(Mesh* mesh, const tinygltf::Model& model, const tinygltf::Primitive& primitive);

    std::vector<Mesh*> mMeshes;
    std::vector<Image> mImages;
    std::vector<int> mTextures;
    std::vector<Material> mMaterials;
};

inline UINT Model::GetIndexCount() const
{
    return mMeshes[0]->GetIndexCount();
}

inline const D3D12_VERTEX_BUFFER_VIEW& Model::GetVertexBufferView() const
{
    return mMeshes[0]->GetVertexBufferView();
}

inline const D3D12_INDEX_BUFFER_VIEW& Model::GetIndexBufferView() const
{
    return mMeshes[0]->GetIndexBufferView();
}

inline const Model::Mesh* Model::GetMesh() const
{
    return mMeshes.at(0);
}

inline const std::vector<Model::Mesh*>& Model::GetMeshes() const
{
    return mMeshes;
}
}
