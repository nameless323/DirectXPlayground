#include "DXrenderer/Model.h"

#include "DXrenderer/RenderContext.h"
#include "DXrenderer/Textures/TextureManager.h"
#include "DXrenderer/Buffers/UploadBuffer.h"
#include "Utils/Logger.h"
#include "Utils/BinaryContainer.h"
#include "Utils/AssetSystem.h"

#include <filesystem>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#define STB_IMAGE_WRITE_IMPLEMENTATION
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
    AssetSystem::Load(path, *this);

    InitializeRuntimeData(ctx, path);
}

Model::Model(RenderContext& ctx, std::vector<Vertex> vertices, std::vector<UINT> indices)
{
    mMeshes.push_back({});
    Mesh* sMesh = &mMeshes.back();

    sMesh->mVertices.swap(vertices);
    sMesh->mIndices.swap(indices);
    sMesh->mIndexCount = static_cast<UINT>(sMesh->mIndices.size());

    sMesh->mVertexBuffer = new VertexBuffer(reinterpret_cast<byte*>(sMesh->mVertices.data()), static_cast<UINT>(sizeof(Vertex) * sMesh->mVertices.size()), sizeof(Vertex), ctx.CommandList, ctx.Device);
    sMesh->mIndexBuffer = new IndexBuffer(reinterpret_cast<byte*>(sMesh->mIndices.data()), static_cast<UINT>(sizeof(UINT) * sMesh->mIndices.size()), ctx.CommandList, ctx.Device, DXGI_FORMAT_R32_UINT);
}

Model::~Model()
{
}

void Model::UpdateMeshes(UINT frame)
{
    for (auto& mesh : mMeshes)
        mesh.UpdateMaterialBuffer(frame);
}

void Model::Parse(const std::string& filename)
{
    tinygltf::Model model;
    LoadModel(filename, model);

    for (UINT i = 0; i < model.accessors.size(); ++i)
    {
        const auto& accessor = model.accessors[i];
        if (accessor.sparse.isSparse)
        {
            assert("Sparse accessors aren't supported at the moment" && false);
        }
    }

    std::filesystem::path pathToModel{ filename };
    std::string dir = pathToModel.parent_path().string() + '\\';
    for (const auto& texture : model.textures)
    {
        mTextures.push_back(texture.source);
    }
    for (const auto& mat : model.materials)
    {
        Material m;
        m.BaseColorTexture = mat.pbrMetallicRoughness.baseColorTexture.index;
        m.MetallicRoughnessTexture = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
        m.NormalTexture = mat.normalTexture.index;
        m.OcclusionTexture = mat.occlusionTexture.index;
        mMaterials.push_back(m);
    }

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for (int node : scene.nodes)
        ParseModelNodes(model, model.nodes[node]);

    for (const auto& image : model.images)
    {
        mImages.push_back({ ~0U, image.uri });
    }
}

void Model::Serialize(BinaryContainer& container)
{
    container << mMeshes.size();
    for (int i = 0; i < mMeshes.size(); ++i)
    {
        container << mMeshes[i];
    }
    container << mImages.size();
    for (int i = 0; i < mImages.size(); ++i)
    {
        container << mImages[i];
    }
    container << mTextures;
    container << mMaterials;
}

void Model::Deserialize(BinaryContainer& container)
{
    size_t sz = 0;
    container >> sz;
    mMeshes.resize(sz);
    for (int i = 0; i < mMeshes.size(); ++i)
    {
        container >> mMeshes[i];
    }
    container >> sz;
    mImages.resize(sz);
    for (int i = 0; i < mImages.size(); ++i)
    {
        container >> mImages[i];
    }
    container >> mTextures;
    container >> mMaterials;
}

void Model::InitializeRuntimeData(RenderContext& ctx, const std::string& path)
{
    std::filesystem::path pathToModel{ path };
    std::string dir = pathToModel.parent_path().string() + '\\';
    for (auto& image : mImages)
    {
        UINT index = ctx.TexManager->CreateTexture(ctx, dir + image.Name).SRVOffset;
        image.IndexInHeap = index;
    }

    for (auto& mesh : mMeshes)
    {
        const Material& modelMat = mesh.mMaterial;
        if (modelMat.BaseColorTexture != -1)
            mesh.mRuntimeMaterial.BaseColorTexture = mImages[mTextures[modelMat.BaseColorTexture]].IndexInHeap;
        if (modelMat.MetallicRoughnessTexture != -1)
            mesh.mRuntimeMaterial.MetallicRoughnessTexture = mImages[mTextures[modelMat.MetallicRoughnessTexture]].IndexInHeap;
        if (modelMat.NormalTexture != -1)
            mesh.mRuntimeMaterial.NormalTexture = mImages[mTextures[modelMat.NormalTexture]].IndexInHeap;
        if (modelMat.OcclusionTexture != -1)
            mesh.mRuntimeMaterial.OcclusionTexture = mImages[mTextures[modelMat.OcclusionTexture]].IndexInHeap;
        memcpy(mesh.mRuntimeMaterial.BaseColorFactor, modelMat.BaseColorFactor, sizeof(float) * 4);

        mesh.mVertexBuffer = new VertexBuffer(reinterpret_cast<byte*>(mesh.mVertices.data()), static_cast<UINT>(sizeof(Vertex) * mesh.mVertices.size()), sizeof(Vertex), ctx.CommandList, ctx.Device);
        mesh.mIndexBuffer = new IndexBuffer(reinterpret_cast<byte*>(mesh.mIndices.data()), static_cast<UINT>(sizeof(UINT) * mesh.mIndices.size()), ctx.CommandList, ctx.Device, DXGI_FORMAT_R32_UINT);
        mesh.mMaterialBuffer = new UploadBuffer(*ctx.Device, sizeof(Material), true, RenderContext::FramesCount);
    }
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

void Model::ParseModelNodes(const tinygltf::Model& model, const tinygltf::Node& node)
{
    if (node.mesh != -1) // Camera usually
        ParseGLTFMesh(model, node, model.meshes[node.mesh]);
    for (int i : node.children)
    {
        ParseModelNodes(model, model.nodes[i]);
    }
}

void Model::ParseGLTFMesh(const tinygltf::Model& model, const tinygltf::Node& node, const tinygltf::Mesh& gltfMesh)
{
    for (size_t i = 0; i < gltfMesh.primitives.size(); ++i)
    {
        mMeshes.push_back({});
        Mesh* mesh = &mMeshes.back();

        tinygltf::Primitive primitive = gltfMesh.primitives[i];

        ParseVertices(mesh, model, node, primitive);
        ParseIndices(mesh, model, primitive);

        mesh->mIndexCount = static_cast<UINT>(mesh->mIndices.size());

        mesh->mMaterial = mMaterials[primitive.material];
        memcpy(mesh->mMaterial.BaseColorFactor, mMaterials[primitive.material].BaseColorFactor, sizeof(float) * 4);
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
        if (mesh->mVertices.empty())
            mesh->mVertices.resize(elemCount);
        assert(mesh->mVertices.size() == elemCount);

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
                mesh->mVertices[i].Pos = { x * scale.x, y * scale.y, z * scale.z }; // For now bake the scale directly in the position
            }
        }
        else if (attrib.first.compare("NORMAL") == 0)
        {
            for (size_t i = 0; i < elemCount; ++i)
            {
                float x = GetElementFromBuffer<float>(bufferStart, byteStride, i, 0);
                float y = GetElementFromBuffer<float>(bufferStart, byteStride, i, 4);
                float z = GetElementFromBuffer<float>(bufferStart, byteStride, i, 8);
                mesh->mVertices[i].Norm = { x, y, z };
            }
        }
        else if (attrib.first.compare("TEXCOORD_0") == 0)
        {
            for (size_t i = 0; i < elemCount; ++i)
            {
                float u = GetElementFromBuffer<float>(bufferStart, byteStride, i, 0);
                float v = GetElementFromBuffer<float>(bufferStart, byteStride, i, 4);
                mesh->mVertices[i].Uv = { u, v };
            }
        }
        else if (attrib.first.compare("TANGENT") == 0)
        {
            for (size_t i = 0; i < elemCount; ++i)
            {
                float x = GetElementFromBuffer<float>(bufferStart, byteStride, i, 0);
                float y = GetElementFromBuffer<float>(bufferStart, byteStride, i, 4);
                float z = GetElementFromBuffer<float>(bufferStart, byteStride, i, 8);
                float w = GetElementFromBuffer<float>(bufferStart, byteStride, i, 12);
                mesh->mVertices[i].Tangent = { x, y, z, w };
            }
        }
        else
        {
            LOG("GLTF Warning: attrib ", attrib.first, " isn't parsed properly\n");
        }
    }
}

void Model::ParseIndices(Mesh* mesh, const tinygltf::Model& model, const tinygltf::Primitive& primitive)
{
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
    mesh->mIndices.reserve(indexAccessor.count);

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
            mesh->mIndices.push_back(i0);
        }
    }
    else if (byteStride == 4)
    {
        for (size_t i = 0; i < indexAccessor.count; i++)
        {
            UINT i0 = GetElementFromBuffer<UINT>(bufferStart, byteStride, i + 0);
            mesh->mIndices.push_back(i0);
        }
    }
    else
        assert(false);
}
}