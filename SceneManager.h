#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

class Scene;

// シーン遷移効果の種類
enum class SceneTransitionEffect {
    None,       // 遷移効果なし
    Fade,       // フェードイン/アウト
    Crossfade,  // クロスフェード
    Slide,      // スライド
    Zoom,       // ズーム
    Rotate      // 回転
};

// シーン遷移状態
enum class SceneTransitionState {
    None,       // 遷移なし
    FadeOut,    // フェードアウト中
    Loading,    // ロード中
    FadeIn      // フェードイン中
};

// シーン遷移情報構造体
struct SceneTransitionInfo {
    std::string fromSceneName;                // 遷移元シーン名
    std::string toSceneName;                  // 遷移先シーン名
    SceneTransitionEffect effect;             // 遷移効果
    float duration;                           // 遷移時間（秒）
    float elapsedTime;                        // 経過時間（秒）
    SceneTransitionState state;               // 遷移状態
    bool loadInBackground;                    // バックグラウンドロードフラグ

    SceneTransitionInfo()
        : fromSceneName("")
        , toSceneName("")
        , effect(SceneTransitionEffect::Fade)
        , duration(1.0f)
        , elapsedTime(0.0f)
        , state(SceneTransitionState::None)
        , loadInBackground(false)
    {
    }
};

// シーンファクトリ関数の型定義
using SceneFactory = std::function<std::shared_ptr<Scene>()>;

// シーン管理クラス
class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    // 初期化
    bool Initialize();

    // シャットダウン
    void Shutdown();

    // 更新
    void Update(float deltaTime);

    // 描画
    void Render();

    // シーンの登録
    void RegisterScene(const std::string& name, SceneFactory factory);

    // シーンの削除
    void UnregisterScene(const std::string& name);

    // シーンの変更
    bool ChangeScene(const std::string& name, SceneTransitionEffect effect = SceneTransitionEffect::Fade, float duration = 1.0f);

    // 現在のシーンの取得
    std::shared_ptr<Scene> GetCurrentScene() const;

    // 現在のシーン名の取得
    std::string GetCurrentSceneName() const;

    // シーン遷移中かどうか
    bool IsTransitioning() const;

    // シーン遷移情報の取得
    const SceneTransitionInfo& GetTransitionInfo() const;

private:
    // シーン遷移の更新
    void UpdateTransition(float deltaTime);

    // シーン遷移の開始
    void StartTransition(const std::string& fromSceneName, const std::string& toSceneName,
        SceneTransitionEffect effect, float duration);

    // シーン遷移の完了
    void CompleteTransition();

    std::unordered_map<std::string, SceneFactory> m_sceneFactories;   // シーンファクトリマップ
    std::shared_ptr<Scene> m_currentScene;                            // 現在のシーン
    std::shared_ptr<Scene> m_nextScene;                               // 次のシーン
    std::string m_currentSceneName;                                   // 現在のシーン名
    SceneTransitionInfo m_transitionInfo;                             // シーン遷移情報
};