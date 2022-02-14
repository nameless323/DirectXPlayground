#pragma once

#include <d3d12.h>
#include <string>
#include <wrl.h>

#include "External/dxc/x64_cfg/dxcapi.h"
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{
class Shader
{
public:
    static void InitCompiler();
    static bool CompileFromFile(const std::wstring& path, const std::wstring& entry, const std::wstring& shaderModel, Shader& outShader, std::vector<DxcDefine>* defines);

    const CD3DX12_SHADER_BYTECODE& GetBytecode() const;
    CD3DX12_SHADER_BYTECODE& GetBytecode();
    bool GetIsCompiled() const;

private:
    static HRESULT CompileFromFile(Shader& shader, LPCWSTR fileName, LPCWSTR entry, LPCWSTR target, std::vector<LPCWSTR>& flags, std::vector<DxcDefine>* defines);

    CD3DX12_SHADER_BYTECODE mBytecode;
    Microsoft::WRL::ComPtr<IDxcBlob> mShaderBlob;
    bool mCompiled = false;
};

inline const CD3DX12_SHADER_BYTECODE& Shader::GetBytecode() const
{
    return mBytecode;
}

inline CD3DX12_SHADER_BYTECODE& Shader::GetBytecode()
{
    return mBytecode;
}

inline bool Shader::GetIsCompiled() const
{
    return mCompiled;
}

}