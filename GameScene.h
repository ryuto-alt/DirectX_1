#pragma once

#include "Scene.h"
#include <memory>
#include <vector>
#include <DirectXMath.h>

class Sprite;
class Camera;
class AudioSource;
class Collider;
class Rigidbody;

// ゲームのメインシーンクラス（テンプレート）
class GameScene : public Scene {
public:
    GameScene();
    virtual ~GameScene();

    // 初期化
    virtual bool Initialize() override;

    // シャットダウン
    virtual void Shutdown() override;

    // 読み込み
    virtual bool Load() override;

    // 解放
    virtual void Unload() override;

    // 更新
    virtual void Update(float deltaTime) override;

    // 描画
    virtual void Render() override;

    // シーンがアクティブになった時
    virtual void OnActivated() override;

    // シーンが非アクティブになった時
    virtual void OnDeactivated() override;

private:
    // 衝突イベントのハンドラ
    void OnCollision(const CollisionEvent& event);

    // キー入力の処理
    void ProcessInput(float deltaTime);

    // アセットの読み込み
    bool LoadAssets();

    // オブジェクトの初期化
    void InitializeObjects();

    std::shared_ptr<Camera> m_camera;                            // 3Dカメラ
    std::vector<std::shared_ptr<Sprite>> m_sprites;              // スプライトリスト
    std::vector<std::shared_ptr<Collider>> m_colliders;          // コライダーリスト
    std::vector<std::shared_ptr<Rigidbody>> m_rigidbodies;       // 剛体リスト
    std::vector<std::shared_ptr<AudioSource>> m_audioSources;    // オーディオソースリスト

    // プレイヤー関連
    std::shared_ptr<Sprite> m_playerSprite;                      // プレイヤースプライト
    std::shared_ptr<Collider> m_playerCollider;                  // プレイヤーコライダー
    std::shared_ptr<Rigidbody> m_playerRigidbody;                // プレイヤー剛体
    DirectX::XMFLOAT2 m_playerPosition;                          // プレイヤー位置
    float m_playerSpeed;                                          // プレイヤー速度

    // 敵関連
    std::vector<std::shared_ptr<Sprite>> m_enemySprites;         // 敵スプライトリスト
    std::vector<std::shared_ptr<Collider>> m_enemyColliders;     // 敵コライダーリスト
    std::vector<DirectX::XMFLOAT2> m_enemyPositions;             // 敵位置リスト
    std::vector<DirectX::XMFLOAT2> m_enemyVelocities;            // 敵速度リスト

    // 効果音関連
    std::shared_ptr<AudioSource> m_bgmSource;                    // BGMソース
    std::shared_ptr<AudioSource> m_seHitSource;                  // ヒット効果音ソース
    std::shared_ptr<AudioSource> m_seJumpSource;                 // ジャンプ効果音ソース

    // 背景関連
    std::shared_ptr<Sprite> m_backgroundSprite;                  // 背景スプライト

    // UI関連
    std::shared_ptr<Sprite> m_titleSprite;                       // タイトルスプライト
    std::shared_ptr<Sprite> m_scoreSprite;                       // スコアスプライト

    // ゲーム状態
    int m_score;                                                  // スコア
    bool m_gameOver;                                              // ゲームオーバーフラグ
    bool m_gameClear;                                             // ゲームクリアフラグ
    float m_timer;                                                // タイマー
};