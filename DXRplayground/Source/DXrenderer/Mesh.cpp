#include "DXrenderer/Mesh.h"

#include "DXrenderer/RenderContext.h"

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

Mesh::Mesh(RenderContext& ctx, const std::string& path)
{
    tinygltf::Model model;
    LoadModel(path, model);

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for (int node : scene.nodes)
        ParseModelNodes(ctx, model, model.nodes[node]);
}

Mesh::Mesh(RenderContext& ctx, std::vector<Vertex> vertices, std::vector<UINT> indices)
{
    m_submeshes.emplace_back();
    Submesh* sMesh = &m_submeshes.back();

    sMesh->m_vertices.swap(vertices);
    sMesh->m_indices.swap(indices);
    sMesh->m_indexCount = static_cast<UINT>(sMesh->m_indices.size());

    sMesh->m_vertexBuffer = new VertexBuffer(reinterpret_cast<byte*>(sMesh->m_vertices.data()), static_cast<UINT>(sizeof(Vertex) * sMesh->m_vertices.size()), sizeof(Vertex), ctx.CommandList, ctx.Device);
    sMesh->m_indexBuffer = new IndexBuffer(reinterpret_cast<byte*>(sMesh->m_indices.data()), static_cast<UINT>(sizeof(UINT) * sMesh->m_indices.size()), ctx.CommandList, ctx.Device, DXGI_FORMAT_R32_UINT);
}

Mesh::~Mesh()
{
}

void Mesh::LoadModel(const std::string& path, tinygltf::Model& model)
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

void Mesh::ParseModelNodes(RenderContext& ctx, const tinygltf::Model& model, const tinygltf::Node& node)
{
    ParseGLTFMesh(ctx, model, model.meshes[node.mesh]);
    for (int i : node.children)
    {
        ParseModelNodes(ctx, model, model.nodes[i]);
    }
}

void Mesh::ParseGLTFMesh(RenderContext& ctx, const tinygltf::Model& model, const tinygltf::Mesh& mesh)
{
    m_submeshes.emplace_back();
    Submesh& submesh = m_submeshes.back();
    for (size_t i = 0; i < mesh.primitives.size(); ++i)
    {
        tinygltf::Primitive primitive = mesh.primitives[i];

        ParseVertices(submesh, model, mesh, primitive);
        ParseIndices(submesh, model, mesh, primitive);
    }
    submesh.m_indexCount = static_cast<UINT>(submesh.m_indices.size());

    submesh.m_vertexBuffer = new VertexBuffer(reinterpret_cast<byte*>(submesh.m_vertices.data()), static_cast<UINT>(sizeof(Vertex) * submesh.m_vertices.size()), sizeof(Vertex), ctx.CommandList, ctx.Device);
    submesh.m_indexBuffer = new IndexBuffer(reinterpret_cast<byte*>(submesh.m_indices.data()), static_cast<UINT>(sizeof(UINT) * submesh.m_indices.size()), ctx.CommandList, ctx.Device, DXGI_FORMAT_R32_UINT);
}

void Mesh::ParseVertices(Submesh& submesh, const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive)
{
    for (auto& attrib : primitive.attributes)
    {
        tinygltf::Accessor accessor = model.accessors[attrib.second];
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];

        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        const byte* bufferData = &model.buffers[bufferView.buffer].data.at(0);
        const byte* bufferStart = bufferData + bufferView.byteOffset;

        size_t elemCount = accessor.count;
        if (submesh.m_vertices.empty())
            submesh.m_vertices.resize(elemCount);
        assert(submesh.m_vertices.size() == elemCount);

        // TODO: don't assume the elem length in the buffer. i.e. UV can be more than 2 floats.
        //uint32 size = 1;
        //if (accessor.type != TINYGLTF_TYPE_SCALAR)
        //    size = accessor.type;

        UINT byteStride = accessor.ByteStride(bufferView);
        if (attrib.first.compare("POSITION") == 0)
        {
            for (size_t i = 0; i < elemCount; ++i)
            {
                float x = GetElementFromBuffer<float>(bufferStart, byteStride, i, 0);
                float y = GetElementFromBuffer<float>(bufferStart, byteStride, i, 4);
                float z = GetElementFromBuffer<float>(bufferStart, byteStride, i, 8);
                submesh.m_vertices[i].Pos = { x, y, z };
            }
        }
        else if (attrib.first.compare("NORMAL") == 0)
        {
            for (size_t i = 0; i < elemCount; ++i)
            {
                float x = GetElementFromBuffer<float>(bufferStart, byteStride, i, 0);
                float y = GetElementFromBuffer<float>(bufferStart, byteStride, i, 4);
                float z = GetElementFromBuffer<float>(bufferStart, byteStride, i, 8);
                submesh.m_vertices[i].Norm = { x, y, z };
            }
        }
        else if (attrib.first.compare("TEXCOORD_0") == 0)
        {
            for (size_t i = 0; i < elemCount; ++i)
            {
                float u = GetElementFromBuffer<float>(bufferStart, byteStride, i, 0);
                float v = GetElementFromBuffer<float>(bufferStart, byteStride, i, 4);
                submesh.m_vertices[i].Uv = { u, v };
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

void Mesh::ParseIndices(Submesh& submesh, const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive)
{
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
    submesh.m_indices.reserve(indexAccessor.count);

    const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
    const byte* bufferData = &model.buffers[indexView.buffer].data.at(0);
    size_t byteOffset = indexView.byteOffset;
    size_t byteLength = indexView.byteLength;

    UINT byteStride = indexAccessor.ByteStride(indexView);
    if (byteStride == 2)
    {
        for (size_t i = 0; i < indexAccessor.count; ++i)
        {
            const byte* bufferStart = bufferData + byteOffset;
            short index = GetElementFromBuffer<short>(bufferStart, byteStride, i);
            submesh.m_indices.push_back(index);
        }
    }
    else if (byteStride == 4)
    {
        for (size_t i = 0; i < indexAccessor.count; ++i)
        {
            const byte* bufferStart = bufferData + byteOffset;
            int index = GetElementFromBuffer<int>(bufferStart, byteStride, i);
            submesh.m_indices.push_back(index);
        }
    }
    else
        assert(false);
}

}