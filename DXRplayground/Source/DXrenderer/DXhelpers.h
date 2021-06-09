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

#define NAME_D3D12_OBJECT(x) SetDXobjectName(x.Get(), L#x)

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
constexpr UINT UAVCubemapTableIndex = TextureTableIndex + 1;

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

    D3D12_DESCRIPTOR_RANGE1 cubeUavRange{};
    cubeUavRange.NumDescriptors = RenderContext::MaxCubemapsUAV;
    cubeUavRange.BaseShaderRegister = 0;
    cubeUavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    cubeUavRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
    cubeUavRange.RegisterSpace = 2;
    cubeUavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

    cbParams.emplace_back();
    cbParams.back().InitAsDescriptorTable(1, &cubeUavRange, D3D12_SHADER_VISIBILITY_ALL);

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

// DXR

// Pretty-print a state object tree.
inline void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc)
{
    std::wstringstream wstr;
    wstr << L"\n";
    wstr << L"--------------------------------------------------------------------\n";
    wstr << L"| D3D12 State Object 0x" << static_cast<const void*>(desc) << L": ";
    if (desc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION) wstr << L"Collection\n";
    if (desc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) wstr << L"Raytracing Pipeline\n";

    auto ExportTree = [](UINT depth, UINT numExports, const D3D12_EXPORT_DESC* exports)
    {
        std::wostringstream woss;
        for (UINT i = 0; i < numExports; i++)
        {
            woss << L"|";
            if (depth > 0)
            {
                for (UINT j = 0; j < 2 * depth - 1; j++) woss << L" ";
            }
            woss << L" [" << i << L"]: ";
            if (exports[i].ExportToRename) woss << exports[i].ExportToRename << L" --> ";
            woss << exports[i].Name << L"\n";
        }
        return woss.str();
    };

    for (UINT i = 0; i < desc->NumSubobjects; i++)
    {
        wstr << L"| [" << i << L"]: ";
        switch (desc->pSubobjects[i].Type)
        {
        case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
            wstr << L"Global Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
            wstr << L"Local Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
            wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8) << *static_cast<const UINT*>(desc->pSubobjects[i].pDesc) << std::setw(0) << std::dec << L"\n";
            break;
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
        {
            wstr << L"DXIL Library 0x";
            auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[i].pDesc);
            wstr << lib->DXILLibrary.pShaderBytecode << L", " << lib->DXILLibrary.BytecodeLength << L" bytes\n";
            wstr << ExportTree(1, lib->NumExports, lib->pExports);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
        {
            wstr << L"Existing Library 0x";
            auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(desc->pSubobjects[i].pDesc);
            wstr << collection->pExistingCollection << L"\n";
            wstr << ExportTree(1, collection->NumExports, collection->pExports);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
        {
            wstr << L"Subobject to Exports Association (Subobject [";
            auto association = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
            UINT index = static_cast<UINT>(association->pSubobjectToAssociate - desc->pSubobjects);
            wstr << index << L"])\n";
            for (UINT j = 0; j < association->NumExports; j++)
            {
                wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
        {
            wstr << L"DXIL Subobjects to Exports Association (";
            auto association = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
            wstr << association->SubobjectToAssociate << L")\n";
            for (UINT j = 0; j < association->NumExports; j++)
            {
                wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG:
        {
            wstr << L"Raytracing Shader Config\n";
            auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(desc->pSubobjects[i].pDesc);
            wstr << L"|  [0]: Max Payload Size: " << config->MaxPayloadSizeInBytes << L" bytes\n";
            wstr << L"|  [1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes << L" bytes\n";
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG:
        {
            wstr << L"Raytracing Pipeline Config\n";
            auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(desc->pSubobjects[i].pDesc);
            wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth << L"\n";
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
        {
            wstr << L"Hit Group (";
            auto hitGroup = static_cast<const D3D12_HIT_GROUP_DESC*>(desc->pSubobjects[i].pDesc);
            wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]") << L")\n";
            wstr << L"|  [0]: Any Hit Import: " << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport : L"[none]") << L"\n";
            wstr << L"|  [1]: Closest Hit Import: " << (hitGroup->ClosestHitShaderImport ? hitGroup->ClosestHitShaderImport : L"[none]") << L"\n";
            wstr << L"|  [2]: Intersection Import: " << (hitGroup->IntersectionShaderImport ? hitGroup->IntersectionShaderImport : L"[none]") << L"\n";
            break;
        }
        }
        wstr << L"|--------------------------------------------------------------------\n";
    }
    wstr << L"\n";
    OutputDebugStringW(wstr.str().c_str());
}

}
