#include "Material.h"
#include "DXTexture.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

Material::Material(ID3D12Device* device)
    : m_device(device), m_constantBufferMappedData(nullptr)
{
    // Initialize with default values
    m_properties.Albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    m_properties.Emission = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    m_properties.Metallic = 0.0f;
    m_properties.Roughness = 0.5f;
    m_properties.AmbientOcclusion = 1.0f;
    m_properties.Padding = 0.0f;
}

Material::~Material()
{
    if (m_constantBufferMappedData)
    {
        m_constantBuffer->Unmap(0, nullptr);
        m_constantBufferMappedData = nullptr;
    }
}

bool Material::Initialize()
{
    // Create a constant buffer for material properties
    const UINT constantBufferSize = sizeof(MaterialConstants);

    // Align to 256-byte boundary as required by DirectX 12
    const UINT alignedSize = (constantBufferSize + 255) & ~255;

    // Create the constant buffer on an upload heap
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = alignedSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer));

    if (FAILED(hr))
    {
        return false;
    }

    // Map the constant buffer
    hr = m_constantBuffer->Map(0, nullptr, &m_constantBufferMappedData);
    if (FAILED(hr))
    {
        return false;
    }

    // Copy the initial data to the constant buffer
    memcpy(m_constantBufferMappedData, &m_properties, sizeof(MaterialConstants));

    return true;
}

void Material::Bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex)
{
    // Update the constant buffer with current material properties
    memcpy(m_constantBufferMappedData, &m_properties, sizeof(MaterialConstants));

    // Set the constant buffer
    commandList->SetGraphicsRootConstantBufferView(
        rootParameterIndex,
        m_constantBuffer->GetGPUVirtualAddress());

    // If we have textures, we would bind them here to appropriate descriptor tables
    // The specific implementation depends on how your shader and root signature are set up
}

bool Material::SetAlbedoTexture(const std::shared_ptr<DXTexture>& texture)
{
    m_albedoTexture = texture;
    return true;
}

bool Material::SetNormalTexture(const std::shared_ptr<DXTexture>& texture)
{
    m_normalTexture = texture;
    return true;
}

bool Material::SetMetallicRoughnessTexture(const std::shared_ptr<DXTexture>& texture)
{
    m_metallicRoughnessTexture = texture;
    return true;
}

bool Material::SetEmissiveTexture(const std::shared_ptr<DXTexture>& texture)
{
    m_emissiveTexture = texture;
    return true;
}

bool Material::SetOcclusionTexture(const std::shared_ptr<DXTexture>& texture)
{
    m_occlusionTexture = texture;
    return true;
}