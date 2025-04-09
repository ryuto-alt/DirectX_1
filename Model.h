#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

// Forward declarations
class Material;
class Skeleton;
class Animation;
class DXTexture;

// Vertex structure with position, normal, texture coordinates, and bone influences
struct ModelVertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexCoord;
    DirectX::XMFLOAT4 BoneWeights;    // Weights for up to 4 bones
    DirectX::XMUINT4 BoneIndices;     // Indices for up to 4 bones
};

// A submesh represents a part of a model with a single material
struct SubMesh {
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    INT BaseVertexLocation = 0;
    DirectX::XMFLOAT4X4 LocalTransform; // Local transform relative to the model
    std::shared_ptr<Material> Material;
};

class Model {
public:
    Model(ID3D12Device* device);
    virtual ~Model();

    // Initialize buffers after data is loaded
    bool Initialize();

    // Bind vertex and index buffers for rendering
    void Bind(ID3D12GraphicsCommandList* commandList);

    // Getters
    std::vector<SubMesh>& GetSubMeshes() { return m_subMeshes; }
    const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }
    ID3D12Resource* GetVertexBuffer() const { return m_vertexBuffer.Get(); }
    ID3D12Resource* GetIndexBuffer() const { return m_indexBuffer.Get(); }
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_vertexBufferView; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_indexBufferView; }
    const DirectX::XMFLOAT4X4& GetGlobalTransform() const { return m_globalTransform; }
    std::shared_ptr<Skeleton> GetSkeleton() const { return m_skeleton; }
    void SetSkeleton(std::shared_ptr<Skeleton> skeleton) { m_skeleton = skeleton; }
    std::vector<std::shared_ptr<Animation>>& GetAnimations() { return m_animations; }
    const std::vector<std::shared_ptr<Animation>>& GetAnimations() const { return m_animations; }
    std::vector<ModelVertex>& GetVertices() { return m_vertices; }
    std::vector<uint32_t>& GetIndices() { return m_indices; }

    // Setters
    void SetGlobalTransform(const DirectX::XMFLOAT4X4& transform) { m_globalTransform = transform; }

protected:
    // Device reference
    ID3D12Device* m_device;

    // Model data
    std::vector<ModelVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<SubMesh> m_subMeshes;

    // Buffer resources
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    // Animation data
    std::shared_ptr<Skeleton> m_skeleton;
    std::vector<std::shared_ptr<Animation>> m_animations;

    // Transform
    DirectX::XMFLOAT4X4 m_globalTransform;

    // Utility method to create buffer from vector data
    template<typename T>
    bool CreateDefaultBuffer(const std::vector<T>& data, Microsoft::WRL::ComPtr<ID3D12Resource>& buffer);
};

// ModelLoader is an abstract base class for model loaders
class ModelLoader {
public:
    virtual ~ModelLoader() = default;
    virtual std::unique_ptr<Model> LoadModel(ID3D12Device* device, const std::string& filename) = 0;
};