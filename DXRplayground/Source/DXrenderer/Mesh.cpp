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
        ParseModelNodes(model, model.nodes[node]);
    m_indexCount = static_cast<UINT>(m_indices.size());

    m_vertexBuffer = new VertexBuffer(reinterpret_cast<byte*>(m_vertices.data()), static_cast<UINT>(sizeof(Vertex) * m_vertices.size()), sizeof(Vertex), ctx.CommandList, ctx.Device);
    m_indexBuffer = new IndexBuffer(reinterpret_cast<byte*>(m_indices.data()), static_cast<UINT>(sizeof(UINT) * m_indices.size()), ctx.CommandList, ctx.Device, DXGI_FORMAT_R32_UINT);
}

Mesh::Mesh(RenderContext& ctx, std::vector<Vertex> vertices, std::vector<UINT> indices)
{
    m_vertices.swap(vertices);
    m_indices.swap(indices);
    m_indexCount = static_cast<UINT>(m_indices.size());

    m_vertexBuffer = new VertexBuffer(reinterpret_cast<byte*>(m_vertices.data()), static_cast<UINT>(sizeof(Vertex) * m_vertices.size()), sizeof(Vertex), ctx.CommandList, ctx.Device);
    m_indexBuffer = new IndexBuffer(reinterpret_cast<byte*>(m_indices.data()), static_cast<UINT>(sizeof(UINT) * m_indices.size()), ctx.CommandList, ctx.Device, DXGI_FORMAT_R32_UINT);
}

Mesh::~Mesh()
{
    delete m_indexBuffer;
    delete m_vertexBuffer;
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

void Mesh::ParseModelNodes(const tinygltf::Model& model, const tinygltf::Node& node)
{
    ParseGLTFMesh(model, model.meshes[node.mesh]);
    for (int i : node.children)
    {
        assert("GLTF node's children aren't supported at the moment" && false);
        ParseModelNodes(model, model.nodes[i]);
    }
}

void Mesh::ParseGLTFMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh)
{
    for (size_t i = 0; i < mesh.primitives.size(); ++i)
    {
        tinygltf::Primitive primitive = mesh.primitives[i];

        ParseVertices(model, mesh, primitive);
        ParseIndices(model, mesh, primitive);
    }
}

void Mesh::ParseVertices(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive)
{
    for (auto& attrib : primitive.attributes)
    {
        tinygltf::Accessor accessor = model.accessors[attrib.second];
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];

        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        const byte* bufferData = &model.buffers[bufferView.buffer].data.at(0);
        const byte* bufferStart = bufferData + bufferView.byteOffset;

        size_t elemCount = accessor.count;
        if (m_vertices.empty())
            m_vertices.resize(elemCount);
        assert(m_vertices.size() == elemCount);

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
                m_vertices[i].Pos = { x, y, z };
            }
        }
        else if (attrib.first.compare("NORMAL") == 0)
        {
            for (size_t i = 0; i < elemCount; ++i)
            {
                float x = GetElementFromBuffer<float>(bufferStart, byteStride, i, 0);
                float y = GetElementFromBuffer<float>(bufferStart, byteStride, i, 4);
                float z = GetElementFromBuffer<float>(bufferStart, byteStride, i, 8);
                m_vertices[i].Norm = { x, y, z };
            }
        }
        else if (attrib.first.compare("TEXCOORD_0") == 0)
        {
            for (size_t i = 0; i < elemCount; ++i)
            {
                float u = GetElementFromBuffer<float>(bufferStart, byteStride, i, 0);
                float v = GetElementFromBuffer<float>(bufferStart, byteStride, i, 4);
                m_vertices[i].Uv = { u, v };
            }
        }
        else
            assert(false);
    }
}

void Mesh::ParseIndices(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive)
{
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
    m_indices.reserve(indexAccessor.count);

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
            m_indices.push_back(index);
        }
    }
    else if (byteStride == 4)
    {
        for (size_t i = 0; i < indexAccessor.count; ++i)
        {
            const byte* bufferStart = bufferData + byteOffset;
            int index = GetElementFromBuffer<int>(bufferStart, byteStride, i);
            m_indices.push_back(index);
        }
    }
    else
        assert(false);
}

}