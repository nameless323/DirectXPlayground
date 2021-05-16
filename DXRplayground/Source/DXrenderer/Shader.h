#pragma once

#include <d3d12.h>
#include <string>
#include <wrl.h>

#include "External/dxc/x64/dxcapi.h"
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{
class Shader
{
public:
    static void InitCompiler();
    static bool Shader::CompileFromFile(const std::wstring& path, const std::wstring& entry, const std::wstring& shaderModel, Shader& outShader);

    const CD3DX12_SHADER_BYTECODE& GetBytecode() const;
    bool GetIsCompiled() const;

private:
    static HRESULT CompileFromFile(Shader& shader, LPCWSTR fileName, const D3D_SHADER_MACRO* defines, ID3DInclude* includes, LPCWSTR entry, LPCWSTR target, std::vector<LPCWSTR>& flags);
    static HRESULT CompileFromFile(Shader& shader, LPCWSTR fileName, LPCWSTR entry, LPCWSTR target, std::vector<LPCWSTR>& flags);

    CD3DX12_SHADER_BYTECODE m_bytecode;
    Microsoft::WRL::ComPtr<IDxcBlob> m_shaderBlob;
    bool m_compiled = false;
};

inline const CD3DX12_SHADER_BYTECODE& Shader::GetBytecode() const
{
    return m_bytecode;
}

inline bool Shader::GetIsCompiled() const
{
    return m_compiled;
}

}