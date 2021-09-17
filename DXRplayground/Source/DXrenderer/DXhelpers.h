#pragma once

#include <array>
#include <exception>
#include <map>
#include <windows.h>
#include <string>
#include <DirectXMath.h>
#include "External/Dx12Helpers/d3dx12.h"
#include "DXrenderer/RenderContext.h"
#include "Utils/Logger.h"

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

inline void ThrowIfFailed(HRESULT hr, const wchar_t* msg)
{
    if (FAILED(hr))
    {
        LOG_W(msg);
        throw ComException(hr);
    }
}

inline UINT CalculateConstantBufferByteSize(UINT byteSize) // [a_vorontcov] Dx12 constant buffers must be 255 byte aligned.
{
    return (byteSize + 255) & ~255;
}

inline UINT Align(UINT size, UINT alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
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

#define AUTO_NAME_D3D12_OBJECT(x) SetDXobjectName(x.Get(), L#x)
#define NAME_D3D12_OBJECT(x, s) SetDXobjectName(x.Get(), s)

inline DirectX::XMFLOAT4X4 TransposeMatrix(const DirectX::XMFLOAT4X4& m)
{
    DirectX::XMMATRIX tmp = XMLoadFloat4x4(&m);
    tmp = XMMatrixTranspose(tmp);
    DirectX::XMFLOAT4X4 res;
    XMStoreFloat4x4(&res, tmp);
    return res;
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


constexpr UINT ConstantBuffersCountPerSpace = 8;
constexpr UINT MaxSpacesForConstantBuffers = 2;
constexpr UINT MaxUAV = 6;
constexpr UINT MaxSpacesForUAV = 2;
constexpr UINT TextureTableIndex = ConstantBuffersCountPerSpace * MaxSpacesForConstantBuffers + MaxUAV * MaxSpacesForUAV;
constexpr UINT UAVTableIndex = TextureTableIndex + 1;

inline UINT GetCBRootParamIndex(UINT index, UINT space = 0)
{
    assert(space < 2 && index < ConstantBuffersCountPerSpace && "CB space is invalid");
    return index + space * ConstantBuffersCountPerSpace;
}

inline UINT GetUARootParamIndex(UINT index)
{
    return MaxSpacesForConstantBuffers * ConstantBuffersCountPerSpace;
}

inline HRESULT CreateCommonRootSignature(ID3D12Device* device, REFIID riid, void** ppv)
{
    std::vector<CD3DX12_ROOT_PARAMETER1> cbParams;
    for (UINT i = 0; i < MaxSpacesForConstantBuffers; ++i)
    {
        for (UINT j = 0; j < ConstantBuffersCountPerSpace; ++j)
        {
            cbParams.emplace_back();
            cbParams.back().InitAsConstantBufferView(j, i);
        }
    }

    for (UINT i = 0; i < MaxSpacesForUAV; ++i)
    {
        for (UINT j = 0; j < MaxUAV; ++j)
        {
            cbParams.emplace_back();
            cbParams.back().InitAsUnorderedAccessView(j, i);
        }
    }

    D3D12_DESCRIPTOR_RANGE1 texRange;
    texRange.NumDescriptors = RenderContext::MaxTextures;
    texRange.BaseShaderRegister = 0;
    texRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    texRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE; // Double check perf
    texRange.RegisterSpace = 0;
    texRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    cbParams.emplace_back();
    cbParams.back().InitAsDescriptorTable(1, &texRange, D3D12_SHADER_VISIBILITY_ALL);

    D3D12_DESCRIPTOR_RANGE1 uavTexRange{};
    uavTexRange.NumDescriptors = RenderContext::MaxUAVTextures;
    uavTexRange.BaseShaderRegister = 0;
    uavTexRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    uavTexRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
    uavTexRange.RegisterSpace = 2;
    uavTexRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

    cbParams.emplace_back();
    cbParams.back().InitAsDescriptorTable(1, &uavTexRange, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        1,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    std::array<CD3DX12_STATIC_SAMPLER_DESC, 2> staticSamplers = { linearClamp, linearWrap };

    D3D12_FEATURE_DATA_ROOT_SIGNATURE signatureData = {};
    signatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &signatureData, sizeof(signatureData))))
        signatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

    D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(UINT(cbParams.size()), cbParams.data(), UINT(staticSamplers.size()), staticSamplers.data(), flags);

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureCreationError;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, signatureData.HighestVersion, &signature, &rootSignatureCreationError);
    if (hr != S_OK)
    {
        OutputDebugStringA(reinterpret_cast<char*>(rootSignatureCreationError->GetBufferPointer()));
        return hr;
    }
    return device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), riid, ppv);
}

inline D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDefaultOpaquePsoDescriptor(ID3D12RootSignature* rootSig, UINT numRenderTargets)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = rootSig;
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    D3D12_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthEnable = true;
    dsDesc.StencilEnable = false;
    dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    desc.DepthStencilState = dsDesc;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    desc.NumRenderTargets = numRenderTargets;

    desc.SampleMask = UINT_MAX;
    desc.SampleDesc.Count = 1;

    return desc;
}

inline D3D12_COMPUTE_PIPELINE_STATE_DESC GetDefaultComputePsoDescriptor(ID3D12RootSignature* rootSig)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = rootSig;
    return desc;
}

inline UINT GetPixelSize(DXGI_FORMAT format)
{
    static const std::map<DXGI_FORMAT, UINT> formats{ { DXGI_FORMAT_R8G8B8A8_UNORM, 4 }, { DXGI_FORMAT_R32G32B32A32_FLOAT, 16 }, { DXGI_FORMAT_R32G32B32_FLOAT, 12 } };
    assert(formats.count(format) == 1 && "Pixel size for the format isn't defined");
    return formats.at(format);
}

inline constexpr DirectX::XMFLOAT4X4 IdentityMatrix{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
    };

}
