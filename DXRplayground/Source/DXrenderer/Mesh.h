#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

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

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT3 Norm;
    XMFLOAT2 Uv;
};

class Mesh
{
public:
    Mesh(const std::string& path);

private:
    void LoadModel(const std::string& path, tinygltf::Model& model);
    void ParseModelNodes(const tinygltf::Model& model, const tinygltf::Node& node);
    void ParseGLTFMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh);
    void ParseVertices(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);
    void ParseIndices(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);

    std::vector<Vertex> m_vertices;
    std::vector<UINT> m_indices;
};
}
