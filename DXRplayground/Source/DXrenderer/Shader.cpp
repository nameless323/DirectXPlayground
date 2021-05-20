#include "Shader.h"

#include "Utils/Logger.h"

namespace DirectxPlayground
{
namespace
{
Microsoft::WRL::ComPtr<IDxcLibrary> DxcLibrary;
Microsoft::WRL::ComPtr<IDxcCompiler2> DxcCompiler;
Microsoft::WRL::ComPtr<IDxcIncludeHandler> IncludeHandler;
UINT CodePage = CP_UTF8;
}

void Shader::InitCompiler()
{
    HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&DxcLibrary));
    if (hr != S_OK)
    {
        LOG("DXC library creation failed");
        assert("DXC library creation failed." && false);
    }
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DxcCompiler));
    if (hr != S_OK)
    {
        LOG("DXC creation failed");
        assert("DXC creation failed." && false);
    }
    Microsoft::WRL::ComPtr<IDxcUtils> utils;
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    if (hr != S_OK)
    {
        LOG("DXC utils creation failed");
        assert("DXC utils creation failed." && false);
    }
    utils->CreateDefaultIncludeHandler(&IncludeHandler);
}

bool Shader::CompileFromFile(const std::wstring& path, const std::wstring& entry, const std::wstring& shaderModel, Shader& outShader)
{
#ifdef _DEBUG
    static std::vector<LPCWSTR> flags = { L"-Zi", L"-Qembed_debug", L"-Od" };
#else
    static std::vector<LPCWSTR> flags = {};
#endif
    HRESULT hr = CompileFromFile(outShader, path.c_str(), entry.c_str(), shaderModel.c_str(), flags);
    return SUCCEEDED(hr);
}

HRESULT Shader::CompileFromFile(Shader& shader, LPCWSTR fileName, const D3D_SHADER_MACRO* defines, ID3DInclude* includes, LPCWSTR entry, LPCWSTR target, std::vector<LPCWSTR>& flags)
{
    shader.m_compiled = false;

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT hr = DxcLibrary->CreateBlobFromFile(fileName, &CodePage, &sourceBlob);
    if (hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
    {
        LOG_W("ERROR_SHARING_VIOLATION occurred while compiling ", fileName, " please retry \n");
    }
    else if (hr != S_OK)
    {
        LOG_W("Error opening the shader file ", fileName, " \n");
    }
    if (FAILED(hr))
        return hr;

    Microsoft::WRL::ComPtr<IDxcOperationResult> result;
    hr = DxcCompiler->Compile(sourceBlob.Get(), fileName, entry, target, flags.data(), static_cast<UINT32>(flags.size()), NULL, 0, IncludeHandler.Get(), &result);
    if (!FAILED(hr))
        result->GetStatus(&hr);

    if (FAILED(hr))
    {
        if (result)
        {
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> error;
            HRESULT errorHr = result->GetErrorBuffer(&error);
            if (SUCCEEDED(errorHr) && error)
            {
                LOG("Shader compilation error ", reinterpret_cast<const char*>(error->GetBufferPointer()));
            }
        }
        return hr;
    }

    result->GetResult(&shader.m_shaderBlob);
    shader.m_compiled = true;
    shader.m_bytecode.BytecodeLength = shader.m_shaderBlob->GetBufferSize();
    shader.m_bytecode.pShaderBytecode = shader.m_shaderBlob->GetBufferPointer();

    return hr;
}

HRESULT Shader::CompileFromFile(Shader& shader, LPCWSTR fileName, LPCWSTR entry, LPCWSTR target, std::vector<LPCWSTR>& flags)
{
    return CompileFromFile(shader, fileName, nullptr, nullptr, entry, target, flags);
}

}