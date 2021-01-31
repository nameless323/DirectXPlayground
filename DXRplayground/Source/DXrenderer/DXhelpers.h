#pragma once

#include <exception>
#include <windows.h>
#include <d3dx12.h>
#include <string>

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
}
