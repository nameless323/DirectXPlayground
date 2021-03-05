#include "DXrenderer/Model.h"

#include "DXrenderer/RenderContext.h"
#include "DXrenderer/TextureManager.h"
#include "DXrenderer/Buffers/UploadBuffer.h"

#include <filesystem>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "External/TinyGLTF/tiny_gltf.h"

namespace DirectxPlayground
{
namespace
{
template <typename T>
T GetElementFromBuffer(const byte* bufferStart, UINT byteStride, size_t elemIndex, UINT offsetInElem = 0)
{
    return *(reinterpret_cast<const T*>(bufferStart + size_t(byteStride) * size_t(elemIndex) + offsetInElem));
}
}

Model::Model(RenderContext& ctx, const std::string& path)
{
    tinygltf::Model model;
    LoadModel(path, model);

    for (UINT i = 0; i < model.accessors.size(); ++i)
    {
        const auto& accessor = model.accessors[i];
        if (accessor.sparse.isSparse)
        {
            assert("Sparse accessors aren't supported at the moment" && false);
        }
    }

    std::filesystem::path pathToModel{ path };
    std::string dir = pathToModel.parent_path().string() + '\\';
    for (const auto& image : model.images)
    {
        UINT index = ctx.TexManager->CreateTexture(ctx, dir + image.uri);
        m_images.push_back({ index, image.uri });
    }
    for (const auto& texture : model.textures)
    {
        m_textures.push_back(texture.source);
    }
    for (const auto& mat : model.materials)
    {
        Material m;
        m.BaseColorTexture = mat.pbrMetallicRoughness.baseColorTexture.index;
        m.MetallicRoughnessTexture = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
        m.NormalTexture = mat.normalTexture.index;
        m.OcclusionTexture = mat.occlusionTexture.index;
        m_materials.push_back(m);
    }

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for (int node : scene.nodes)
        ParseModelNodes(ctx, model, model.nodes[node]);
}

Model::Model(RenderContext& ctx, std::vector<Vertex> vertices, std::vector<UINT> indices)
{
    m_meshes.emplace_back(new Mesh{});
    Mesh* sMesh = m_meshes.back();

    sMesh->m_vertices.swap(vertices);
    sMesh->m_indices.swap(indices);
    sMesh->m_indexCount = static_cast<UINT>(sMesh->m_indices.size());

    sMesh->m_vertexBuffer = new VertexBuffer(reinterpret_cast<byte*>(sMesh->m_vertices.data()), static_cast<UINT>(sizeof(Vertex) * sMesh->m_vertices.size()), sizeof(Vertex), ctx.CommandList, ctx.Device);
    sMesh->m_indexBuffer = new IndexBuffer(reinterpret_cast<byte*>(sMesh->m_indices.data()), static_cast<UINT>(sizeof(UINT) * sMesh->m_indices.size()), ctx.CommandList, ctx.Device, DXGI_FORMAT_R32_UINT);
}

Model::~Model()
{
    for (auto submesh : m_meshes)
    {
        delete submesh;
    }
    m_meshes.clear();
}

void Model::UpdateMeshes(UINT frame)
{
    for (auto mesh : m_meshes)
        mesh->UpdateMaterialBuffer(frame);
}

void Model::LoadModel(const std::string& path, tinygltf::Model& model)
{
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool res = false;

    size_t lastPeriod = path.find_last_of('.');
    if (lastPeriod == std::string::npos)
        assert(false);
    std::string ext = path.substr(lastPeriod);

    if (ext == ".glb")
        res = loader.LoadBinaryFromFile(&model, &err, &warn, path.c_str());
    else if (ext == ".gltf")
        res = loader.LoadASCIIFromFile(&model, &err, &warn, path.c_str());
    else
        assert(false);
    if (!res)
    {
        std::stringstream ss;
        ss << "Failed to load model " << path << std::endl;
        OutputDebugStringA(ss.str().c_str());
        assert(ss.str().c_str() && false);
    }
}

void Model::ParseModelNodes(RenderContext& ctx, const tinygltf::Model& model, const tinygltf::Node& node)
{
    ParseGLTFMesh(ctx, model, node, model.meshes[node.mesh]);
    for (int i : node.children)
    {
        ParseModelNodes(ctx, model, model.nodes[i]);
    }
}

void Model::ParseGLTFMesh(RenderContext& ctx, const tinygltf::Model& model, const tinygltf::Node& node, const tinygltf::Mesh& gltfMesh)
{
    for (size_t i = 0; i < gltfMesh.primitives.size(); ++i)
    {
        m_meshes.push_back(new Mesh{});
        Mesh* mesh = m_meshes.back();

        tinygltf::Primitive primitive = gltfMesh.primitives[i];

        ParseVertices(mesh, model, node, primitive);
        ParseIndices(mesh, model, primitive);

        mesh->m_indexCount = static_cast<UINT>(mesh->m_indices.size());

        Material& modelMat = m_materials[primitive.material];
        mesh->m_material.BaseColorTexture = m_images[m_textures[modelMat.BaseColorTexture]].IndexInHeap;
        if (modelMat.MetallicRoughnessTexture != -1)
            mesh->m_material.MetallicRoughnessTexture = m_images[m_textures[modelMat.MetallicRoughnessTexture]].IndexInHeap;
        if (modelMat.NormalTexture != -1)
            mesh->m_material.NormalTexture = m_images[m_textures[modelMat.NormalTexture]].IndexInHeap;
        if (modelMat.OcclusionTexture != -1)
            mesh->m_material.OcclusionTexture = m_images[m_textures[modelMat.OcclusionTexture]].IndexInHeap;
        memcpy(mesh->m_material.BaseColorFactor, modelMat.BaseColorFactor, sizeof(float) * 4);

        mesh->m_vertexBuffer = new VertexBuffer(reinterpret_cast<byte*>(mesh->m_vertices.data()), static_cast<UINT>(sizeof(Vertex) * mesh->m_vertices.size()), sizeof(Vertex), ctx.CommandList, ctx.Device);
        mesh->m_indexBuffer = new IndexBuffer(reinterpret_cast<byte*>(mesh->m_indices.data()), static_cast<UINT>(sizeof(UINT) * mesh->m_indices.size()), ctx.CommandList, ctx.Device, DXGI_FORMAT_R32_UINT);
        mesh->m_materialBuffer = new UploadBuffer(*ctx.Device, sizeof(Material), true, RenderContext::FramesCount);
    }
}

void Model::ParseVertices(Mesh* mesh, const tinygltf::Model& model, const tinygltf::Node& node, const tinygltf::Primitive& primitive)
{
    for (auto& attrib : primitive.attributes)
    {
        tinygltf::Accessor accessor = model.accessors[attrib.second];
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];

        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        const byte* bufferData = &model.buffers[bufferView.buffer].data.at(0);
        const byte* bufferStart = bufferData + bufferView.byteOffset + accessor.byteOffset;

        size_t elemCount = accessor.count;
        if (mesh->m_vertices.empty())
            mesh->m_vertices.resize(elemCount);
        assert(mesh->m_vertices.size() == elemCount);

        // TODO: don't assume the elem length in the buffer. i.e. UV can be more than 2 floats.
        //uint32 size = 1;
        //if (accessor.type != TINYGLTF_TYPE_SCALAR)
        //    size = accessor.type;

        UINT byteStride = accessor.ByteStride(bufferView);
        XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
        if (node.scale.size() == 3)
            scale = { static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2]) };

        if (attrib.first.compare("POSITION") == 0)
        {
            for (size_t i = 0; i < elemCount; ++i)
            {
                float x = GetElementFromBuffer<float>(bufferStart, byteStride, i, 0);
                float y = GetElementFromBuffer<float>(bufferStart, byteStride, i, 4);
                float z = GetElementFromBuffer<float>(bufferStart, byteStride, i, 8);
                mesh->m_vertices[i].Pos = { x * scale.x, y * scale.y, z * scale.z }; // For now bake the scale directly in the position
            }
        }
        else if (attrib.first.compare("NORMAL") == 0)
        {
            for (size_t i = 0; i < elemCount; ++i)
            {
                float x = GetElementFromBuffer<float>(bufferStart, byteStride, i, 0);
                float y = GetElementFromBuffer<float>(bufferStart, byteStride, i, 4);
                float z = GetElementFromBuffer<float>(bufferStart, byteStride, i, 8);
                mesh->m_vertices[i].Norm = { x, y, z };
            }
        }
        else if (attrib.first.compare("TEXCOORD_0") == 0)
        {
            for (size_t i = 0; i < elemCount; ++i)
            {
                float u = GetElementFromBuffer<float>(bufferStart, byteStride, i, 0);
                float v = GetElementFromBuffer<float>(bufferStart, byteStride, i, 4);
                mesh->m_vertices[i].Uv = { u, v };
            }
        }
        else
        {
            std::stringstream ss;
            ss << "GLTF Warning: attrib " << attrib.first << " isn't parsed properly" << '\n';
            LOG(ss.str().c_str());
        }
    }
}

void Model::ParseIndices(Mesh* mesh, const tinygltf::Model& model, const tinygltf::Primitive& primitive)
{
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
    mesh->m_indices.reserve(indexAccessor.count);

    const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
    const byte* bufferData = &model.buffers[indexView.buffer].data.at(0);
    size_t byteOffset = indexView.byteOffset + indexAccessor.byteOffset;
    size_t byteLength = indexView.byteLength;

    UINT byteStride = indexAccessor.ByteStride(indexView);
    const byte* bufferStart = bufferData + byteOffset;
    assert((indexAccessor.count % 3 == 0) && "GLTF index accessor doesn't represent triangles");
    if (byteStride == 2)
    {
        for (size_t i = 0; i < indexAccessor.count; i ++)
        {
            short i0 = GetElementFromBuffer<short>(bufferStart, byteStride, i + 0);
            mesh->m_indices.push_back(i0);
        }
    }
    else if (byteStride == 4)
    {
        for (size_t i = 0; i < indexAccessor.count; i++)
        {
            UINT i0 = GetElementFromBuffer<UINT>(bufferStart, byteStride, i + 0);
            mesh->m_indices.push_back(i0);
        }
    }
    else
        assert(false);
}


}