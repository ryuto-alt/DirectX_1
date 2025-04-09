#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>
#include <memory>

class DXTexture;

// Material properties for PBR (Physically Based Rendering)
struct MaterialConstants {
    DirectX::XMFLOAT4 Albedo;          // Base color (RGB) and opacity (A)
    DirectX::XMFLOAT4 Emission;        // Emissive color (RGB) and emission strength (A)
    float Metallic;                    // Metallic factor (0=dielectric, 1=metal)
    float Roughness;                   // Surface roughness
    float AmbientOcclusion;            // Ambient occlusion factor
    float Padding;                     // Padding to ensure 16-byte alignment
};

class Material {
public:
    Material(ID3D12Device* device);
    ~Material();

    // Initialize the material with default values
    bool Initialize();

    // Bind material resources to the graphics pipeline
    void Bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex);

    // Accessors for material properties
    MaterialConstants& GetProperties() { return m_properties; }
    const MaterialConstants& GetProperties() const { return m_properties; }

    // Load and set textures
    bool SetAlbedoTexture(const std::shared_ptr<DXTexture>& texture);
    bool SetNormalTexture(const std::shared_ptr<DXTexture>& texture);
    bool SetMetallicRoughnessTexture(const std::shared_ptr<DXTexture>& texture);
    bool SetEmissiveTexture(const std::shared_ptr<DXTexture>& texture);
    bool SetOcclusionTexture(const std::shared_ptr<DXTexture>& texture);

private:
    // Device reference
    ID3D12Device* m_device;

    // Material properties
    MaterialConstants m_properties;

    // Constant buffer for the material
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    void* m_constantBufferMappedData;

    // Textures
    std::shared_ptr<DXTexture> m_albedoTexture;
    std::shared_ptr<DXTexture> m_normalTexture;
    std::shared_ptr<DXTexture> m_metallicRoughnessTexture;
    std::shared_ptr<DXTexture> m_emissiveTexture;
    std::shared_ptr<DXTexture> m_occlusionTexture;
};