#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <string>

class Renderer;
class ResourceManager;
struct Texture;

// スプライト構造体
struct Sprite {
    std::shared_ptr<Texture> texture;    // テクスチャ
    DirectX::XMFLOAT2 position;          // 位置
    DirectX::XMFLOAT2 size;              // サイズ
    DirectX::XMFLOAT2 origin;            // 原点（0～1の範囲、デフォルトは中心(0.5, 0.5)）
    float rotation;                       // 回転（ラジアン）
    DirectX::XMFLOAT4 color;             // 色（RGBA、0～1の範囲）
    DirectX::XMFLOAT2 texCoordTL;        // テクスチャ座標（左上）
    DirectX::XMFLOAT2 texCoordBR;        // テクスチャ座標（右下）
    float depth;                          // 深度（0～1の範囲、小さいほど手前）
    bool visible;                         // 表示フラグ
};

// スプライト頂点構造体
struct SpriteVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 texCoord;
    DirectX::XMFLOAT4 color;
};

// スプライトレンダラークラス
class SpriteRenderer {
public:
    SpriteRenderer();
    ~SpriteRenderer();

    // 初期化
    bool Initialize(Renderer* renderer, ResourceManager* resourceManager);

    // シャットダウン
    void Shutdown();

    // スプライトの作成
    std::shared_ptr<Sprite> CreateSprite(const std::wstring& texturePath);

    // スプライトバッチの開始
    void Begin();

    // スプライトの描画
    void Draw(const std::shared_ptr<Sprite>& sprite);

    // スプライトバッチの終了
    void End();

private:
    // パイプラインステートの作成
    bool CreatePipelineState();

    // ルートシグネチャの作成
    bool CreateRootSignature();

    // 頂点バッファの作成
    bool CreateVertexBuffer();

    // インデックスバッファの作成
    bool CreateIndexBuffer();

    // 定数バッファの作成
    bool CreateConstantBuffer();

    // ビュープロジェクション行列の更新
    void UpdateViewProjection();

    // スプライトの頂点データを設定
    void SetSpriteVertices(const std::shared_ptr<Sprite>& sprite, SpriteVertex* vertices);

    // 参照
    Renderer* m_renderer;
    ResourceManager* m_resourceManager;

    // パイプラインオブジェクト
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

    // 頂点バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    SpriteVertex* m_vertexBufferCPU;     // マップされた頂点バッファ
    int m_maxBatchSize;                  // 最大バッチサイズ
    int m_currentBatchSize;              // 現在のバッチサイズ

    // インデックスバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    // 定数バッファ
    struct SpriteConstantBuffer {
        DirectX::XMFLOAT4X4 viewProjection;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    SpriteConstantBuffer* m_constantBufferCPU;   // マップされた定数バッファ

    // デスクリプタヒープのインデックス
    UINT m_constantBufferIndex;

    // バッチフラグ
    bool m_batchActive;

    // 初期化フラグ
    bool m_initialized;
};