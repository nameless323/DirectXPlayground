#pragma once

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
}
