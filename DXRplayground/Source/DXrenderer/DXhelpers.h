#pragma once

#include <array>
#include <exception>
#include <windows.h>
#include <string>
#include <DirectXMath.h>
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{

class ComException : public std::exception // [a_vorontcov] https://github.com/Microsoft/DirectXTK/wiki/ThrowIfFailed
{
public:
    ComException(HRESULT hr)
        : m_result(hr)
    {}

    virtual const char* what() const override
    {
        static char str[64] = { 0 };
        sprintf_s(str, "Failure with HRESULT of %08X", m_result);
        return str;
    }

private:
    HRESULT m_result = -1;
};

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw ComException(hr);
    }
}

inline UINT CalculateConstantBufferByteSize(UINT byteSize) // [a_vorontcov] Dx12 constant buffers must be 255 byte aligned.
{
    return (byteSize + 255) & ~255;
}

#if defined(_DEBUG)
inline void SetDXobjectName(ID3D12Object* object, LPCWSTR name)
{
    object->SetName(name);
}
#else
inline void SetDXobjectName(ID3D12Object*, LPCWSTR)
{
}
#endif

#define NAME_D3D12_OBJECT(x) SetDXobjectName(x.Get(), L#x)

inline DirectX::XMFLOAT4X4 TransposeMatrix(const DirectX::XMFLOAT4X4& m)
{
    DirectX::XMMATRIX tmp = XMLoadFloat4x4(&m);
    tmp = XMMatrixTranspose(tmp);
    DirectX::XMFLOAT4X4 res;
    XMStoreFloat4x4(&res, tmp);
    return res;
}

#ifdef _DEBUG
#define LOG_(msg, file, line) \
{ \
    std::stringstream ss___; \
    ss___ << file << "(" << line << "): " << msg << std::endl; \
    OutputDebugStringA(ss___.str().c_str()); \
    printf("%s\n", ss___.str().c_str()); \
}
#define LOG(msg) LOG_(msg, __FILE__, __LINE__)
#else
#define LOG(msg)
#endif

template <typename T>
inline void SafeDelete(T*& ptr)
{
    if (ptr != nullptr)
    {
        delete ptr;
        ptr = nullptr;
    }
}

inline std::array<D3D12_INPUT_ELEMENT_DESC, 4>& GetInputLayoutUV_N_T()
{
    static std::array<D3D12_INPUT_ELEMENT_DESC, 4> layout = 
    { {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    } };
    return layout;
}
}
