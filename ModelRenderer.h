#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

class Renderer;
class ResourceManager;
struct Model;
struct Mesh;
struct Material;

// カメラクラス
class Camera {
public:
    Camera();
    ~Camera() = default;

    // カメラの位置を設定
    void SetPosition(const DirectX::XMFLOAT3& position);

    // カメラの注視点を設定
    void SetTarget(const DirectX::XMFLOAT3& target);

    // カメラの上方向を設定
    void SetUp(const DirectX::XMFLOAT3& up);

    // 視野角を設定（ラジアン）
    void SetFov(float fov);

    // アスペクト比を設定
    void SetAspectRatio(float aspectRatio);

    // 近平面と遠平面を設定
    void SetClipPlanes(float nearPlane, float farPlane);

    // ビュー行列を取得
    DirectX::XMMATRIX GetViewMatrix() const;

    // プロジェクション行列を取得
    DirectX::XMMATRIX GetProjectionMatrix() const;

    // カメラの位置を取得
    DirectX::XMFLOAT3 GetPosition() const { return m_position; }

    // カメラの注視点を取得
    DirectX::XMFLOAT3 GetTarget() const { return m_target; }

    // カメラの上方向を取得
    DirectX::XMFLOAT3 GetUp() const { return m_up; }

private:
    DirectX::XMFLOAT3 m_position;     // カメラの位置
    DirectX::XMFLOAT3 m_target;       // カメラの注視点
    DirectX::XMFLOAT3 m_up;           // カメラの上方向
    float m_fov;                      // 視野角（ラジアン）
    float m_aspectRatio;              // アスペクト比
    float m_nearPlane;                // 近平面
    float m_farPlane;                 // 遠平面
};

// 3Dモデルレンダラークラス
class ModelRenderer {
public:
    ModelRenderer();
    ~ModelRenderer();

    // 初期化
    bool Initialize(Renderer* renderer, ResourceManager* resourceManager);

    // シャットダウン
    void Shutdown();

    // カメラの設定
    void SetCamera(const std::shared_ptr<Camera>& camera);

    // カメラの取得
    std::shared_ptr<Camera> GetCamera() const { return m_camera; }

    // モデルの描画
    void DrawModel(const std::shared_ptr<Model>& model, const DirectX::XMMATRIX& worldMatrix);

    // メッシュの描画
    void DrawMesh(const std::shared_ptr<Mesh>& mesh, const DirectX::XMMATRIX& worldMatrix);

    // モデルの描画開始
    void Begin();

    // モデルの描画終了
    void End();

private:
    // パイプラインステートの作成
    bool CreatePipelineState();

    // ルートシグネチャの作成
    bool CreateRootSignature();

    // 定数バッファの作成
    bool CreateConstantBuffer();

    // ワールド・ビュー・プロジェクション行列の更新
    void UpdateWorldViewProjection(const DirectX::XMMATRIX& worldMatrix);

    // マテリアルの更新
    void UpdateMaterial(const std::shared_ptr<Material>& material);

    // 参照
    Renderer* m_renderer;
    ResourceManager* m_resourceManager;
    std::shared_ptr<Camera> m_camera;

    // パイプラインオブジェクト
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

    // 定数バッファ（ワールド・ビュー・プロジェクション行列）
    struct ModelConstantBuffer {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 viewProjection;
        DirectX::XMFLOAT3 cameraPosition;
        float padding;
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    ModelConstantBuffer* m_constantBufferCPU;
    UINT m_constantBufferIndex;

    // 定数バッファ（マテリアル）
    struct MaterialConstantBuffer {
        DirectX::XMFLOAT4 diffuse;
        DirectX::XMFLOAT4 ambient;
        DirectX::XMFLOAT4 specular;
        float shininess;
        DirectX::XMFLOAT3 padding;
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> m_materialConstantBuffer;
    MaterialConstantBuffer* m_materialConstantBufferCPU;
    UINT m_materialConstantBufferIndex;

    // 初期化フラグ
    bool m_initialized;

    // バッチフラグ
    bool m_batchActive;
};