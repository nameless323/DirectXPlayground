#pragma once

#include <d3dcompiler.h>
#include <d3d12.h>
#include <string>
#include <wrl.h>

#include "d3dx12.h"

namespace DirectxPlayground
{
class Shader
{
public:
    static Shader CompileFromFile(const std::string& path, const std::string& entry, const std::string& shaderModel);

    const CD3DX12_SHADER_BYTECODE& GetBytecode() const;
    char* GetErrorMsg() const;
    bool GetIsCompiled() const;

private:
    static HRESULT CompileFromFile(Shader& shader, LPCWSTR fileName, const D3D_SHADER_MACRO* defines, ID3DInclude* includes, LPCSTR entry, LPCSTR target, UINT flags1, UINT flags2);
    static HRESULT CompileFromFile(Shader& shader, LPCWSTR fileName, LPCSTR entry, LPCSTR target, UINT flags);

    CD3DX12_SHADER_BYTECODE m_bytecode;
    Microsoft::WRL::ComPtr<ID3DBlob> m_shaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> m_error;
    bool m_compiled = false;
};

inline const CD3DX12_SHADER_BYTECODE& Shader::GetBytecode() const
{
    return m_bytecode;
}

inline char* Shader::GetErrorMsg() const
{
    return reinterpret_cast<char*>(m_error->GetBufferPointer());
}

inline bool Shader::GetIsCompiled() const
{
    return m_compiled;
}

}