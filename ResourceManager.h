#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <DirectXMath.h>
#include <DirectXTex.h>

// 前方宣言
struct Texture;
struct Model;
struct Mesh;
struct Material;
struct AudioClip;
struct Shader;

// テクスチャ構造体
struct Texture {
    std::wstring name;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap;
    UINT descriptorIndex;
    DirectX::TexMetadata metadata;
};

// マテリアル構造体
struct Material {
    std::wstring name;
    DirectX::XMFLOAT4 diffuse;
    DirectX::XMFLOAT4 ambient;
    DirectX::XMFLOAT4 specular;
    float shininess;
    std::shared_ptr<Texture> diffuseMap;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> specularMap;
};

// 頂点構造体
struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 texCoord;
    DirectX::XMFLOAT3 tangent;
    DirectX::XMFLOAT3 bitangent;
};

// メッシュ構造体
struct Mesh {
    std::wstring name;
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;
    std::shared_ptr<Material> material;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    UINT indexCount;
};

// モデル構造体
struct Model {
    std::wstring name;
    std::vector<std::shared_ptr<Mesh>> meshes;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 rotation;
    DirectX::XMFLOAT3 scale;
};

// オーディオクリップ構造体
struct AudioClip {
    std::wstring name;
    std::vector<BYTE> data;
    UINT sampleRate;
    UINT channelCount;
    UINT bitsPerSample;
};

// シェーダー構造体
struct Shader {
    std::wstring name;
    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> geometryShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> hullShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> domainShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> computeShaderBlob;
};

// リソース管理クラス
class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    // 初期化
    bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

    // シャットダウン
    void Shutdown();

    // テクスチャの読み込み
    std::shared_ptr<Texture> LoadTexture(const std::wstring& filename);

    // モデルの読み込み
    std::shared_ptr<Model> LoadModel(const std::wstring& filename);

    // OBJモデルの読み込み
    std::shared_ptr<Model> LoadObjModel(const std::wstring& filename);

    // FBXモデルの読み込み
    std::shared_ptr<Model> LoadFbxModel(const std::wstring& filename);

    // オーディオの読み込み
    std::shared_ptr<AudioClip> LoadAudio(const std::wstring& filename);

    // シェーダーの読み込み
    std::shared_ptr<Shader> LoadShader(
        const std::wstring& name,
        const std::wstring& vsFilename,
        const std::wstring& psFilename,
        const std::wstring& gsFilename = L"",
        const std::wstring& hsFilename = L"",
        const std::wstring& dsFilename = L"",
        const std::wstring& csFilename = L""
    );

    // アップロードバッファの作成
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(const void* data, UINT64 dataSize);

    // デフォルトバッファの作成
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        const void* data,
        UINT64 dataSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer
    );

    // テクスチャの取得
    std::shared_ptr<Texture> GetTexture(const std::wstring& name);

    // モデルの取得
    std::shared_ptr<Model> GetModel(const std::wstring& name);

    // オーディオの取得
    std::shared_ptr<AudioClip> GetAudio(const std::wstring& name);

    // シェーダーの取得
    std::shared_ptr<Shader> GetShader(const std::wstring& name);

    // デバイスの取得
    ID3D12Device* GetDevice() const { return m_device; }

    // コマンドリストの取得
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList; }

    // CBV/SRV/UAVディスクリプタヒープの取得
    ID3D12DescriptorHeap* GetCbvSrvUavHeap() const;

    // アップロードヒープの実行
    void ExecuteUploadHeap();

private:
    // D3D12デバイスとコマンドリスト
    ID3D12Device* m_device;
    ID3D12GraphicsCommandList* m_commandList;

    // リソースキャッシュ
    std::unordered_map<std::wstring, std::shared_ptr<Texture>> m_textures;
    std::unordered_map<std::wstring, std::shared_ptr<Model>> m_models;
    std::unordered_map<std::wstring, std::shared_ptr<AudioClip>> m_audioClips;
    std::unordered_map<std::wstring, std::shared_ptr<Shader>> m_shaders;

    // 現在のディスクリプタインデックス
    UINT m_currentDescriptorIndex;

    // 初期化状態
    bool m_initialized;

    // WAVファイルの読み込み
    bool LoadWavFile(const std::wstring& filename, std::shared_ptr<AudioClip> audioClip);

    // MP3ファイルの読み込み
    bool LoadMp3File(const std::wstring& filename, std::shared_ptr<AudioClip> audioClip);

    // OGGファイルの読み込み
    bool LoadOggFile(const std::wstring& filename, std::shared_ptr<AudioClip> audioClip);
};