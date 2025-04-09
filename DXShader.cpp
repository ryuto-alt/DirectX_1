#include "DXShader.h"
#include <iostream>

using Microsoft::WRL::ComPtr;

DXShader::DXShader(ID3D12Device* device)
    : m_device(device)
{
}

DXShader::~DXShader()
{
    // Resources are automatically cleaned up by ComPtr
}

bool DXShader::CompileVertexShader(const std::wstring& filename, const std::string& entryPoint)
{
    return CompileShader(filename, entryPoint, "vs_5_0", &m_vertexShaderBlob);
}

bool DXShader::CompilePixelShader(const std::wstring& filename, const std::string& entryPoint)
{
    return CompileShader(filename, entryPoint, "ps_5_0", &m_pixelShaderBlob);
}

bool DXShader::CompileShader(const std::wstring& filename, const std::string& entryPoint,
    const std::string& target, ID3DBlob** shaderBlob)
{
    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = D3DCompileFromFile(
        filename.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint.c_str(),
        target.c_str(),
        compileFlags,
        0,
        shaderBlob,
        &m_errorBlob);

    if (FAILED(hr))
    {
        if (m_errorBlob)
        {
#ifdef _DEBUG
            std::cout << "Shader compilation error: " << (char*)m_errorBlob->GetBufferPointer() << std::endl;
#endif
            OutputDebugStringA((char*)m_errorBlob->GetBufferPointer());
        }
        return false;
    }

    return true;
}