#pragma once
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>

class DXShader {
public:
    DXShader(ID3D12Device* device);
    ~DXShader();

    bool CompileVertexShader(const std::wstring& filename, const std::string& entryPoint);
    bool CompilePixelShader(const std::wstring& filename, const std::string& entryPoint);

    ID3DBlob* GetVertexShaderBlob() const { return m_vertexShaderBlob.Get(); }
    ID3DBlob* GetPixelShaderBlob() const { return m_pixelShaderBlob.Get(); }

private:
    bool CompileShader(const std::wstring& filename, const std::string& entryPoint,
        const std::string& target, ID3DBlob** shaderBlob);

    ID3D12Device* m_device;
    Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> m_errorBlob;
};