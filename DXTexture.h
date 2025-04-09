#pragma once
#include <d3d12.h>
#include <DirectXTex.h>
#include <wrl/client.h>
#include <string>

class DXTexture {
public:
    DXTexture(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
    ~DXTexture();

    bool LoadFromFile(const std::wstring& filename);
    ID3D12Resource* GetResource() const { return m_textureResource.Get(); }
    D3D12_SHADER_RESOURCE_VIEW_DESC GetSrvDesc() const;

private:
    bool CreateTextureResource(const DirectX::TexMetadata& metadata);
    bool CreateTextureUploadHeap(const DirectX::Image* image);
    bool CopyTextureData(const DirectX::Image* image);
    size_t AlignmentedSize(size_t size, size_t alignment);

    ID3D12Device* m_device;
    ID3D12GraphicsCommandList* m_commandList;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_textureResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_textureUploadHeap;
    DirectX::TexMetadata m_metadata;
    DirectX::ScratchImage m_scratchImage;
};