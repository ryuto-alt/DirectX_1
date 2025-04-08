#pragma once

#include <memory>
#include <string>
#include <vector>
#include <DirectXMath.h>

class Engine;
class SceneManager;
class ResourceManager;
class InputManager;
class SpriteRenderer;
class ModelRenderer;
class PhysicsSystem;
class AudioSystem;
class Camera;

// シーンの基本クラス
class Scene {
public:
    Scene();
    virtual ~Scene();

    // 初期化
    virtual bool Initialize();

    // シャットダウン
    virtual void Shutdown();

    // 読み込み
    virtual bool Load();

    // 解放
    virtual void Unload();

    // 更新
    virtual void Update(float deltaTime);

    // 描画
    virtual void Render();

    // シーンの一時停止
    virtual void Pause();

    // シーンの再開
    virtual void Resume();

    // シーンがアクティブになった時
    virtual void OnActivated();

    // シーンが非アクティブになった時
    virtual void OnDeactivated();

    // 名前の取得
    const std::string& GetName() const;

    // 名前の設定
    void SetName(const std::string& name);

    // 読み込み完了フラグの取得
    bool IsLoaded() const;

    // 一時停止フラグの取得
    bool IsPaused() const;

    // アクティブフラグの取得
    bool IsActive() const;

    // シーンマネージャーの設定
    void SetSceneManager(SceneManager* sceneManager);

protected:
    // エンジンの取得
    Engine* GetEngine() const;

    // シーンマネージャーの取得
    SceneManager* GetSceneManager() const;

    // リソースマネージャーの取得
    ResourceManager* GetResourceManager() const;

    // 入力マネージャーの取得
    InputManager* GetInputManager() const;

    // スプライトレンダラーの取得
    SpriteRenderer* GetSpriteRenderer() const;

    // モデルレンダラーの取得
    ModelRenderer* GetModelRenderer() const;

    // 物理システムの取得
    PhysicsSystem* GetPhysicsSystem() const;

    // オーディオシステムの取得
    AudioSystem* GetAudioSystem() const;

    std::string m_name;            // シーン名
    bool m_isLoaded;               // 読み込み完了フラグ
    bool m_isPaused;               // 一時停止フラグ
    bool m_isActive;               // アクティブフラグ
    SceneManager* m_sceneManager;  // シーンマネージャー
};