#include "SceneManager.h"
#include "Scene.h"
#include "Engine.h"

SceneManager::SceneManager()
    : m_currentScene(nullptr)
    , m_nextScene(nullptr)
    , m_currentSceneName("")
{
}

SceneManager::~SceneManager()
{
    Shutdown();
}

bool SceneManager::Initialize()
{
    return true;
}

void SceneManager::Shutdown()
{
    // 現在のシーンを解放
    if (m_currentScene) {
        m_currentScene->Shutdown();
        m_currentScene = nullptr;
    }

    // 次のシーンを解放
    if (m_nextScene) {
        m_nextScene->Shutdown();
        m_nextScene = nullptr;
    }

    // シーンファクトリをクリア
    m_sceneFactories.clear();

    m_currentSceneName = "";
}

void SceneManager::Update(float deltaTime)
{
    // シーン遷移中なら遷移処理を更新
    if (IsTransitioning()) {
        UpdateTransition(deltaTime);
    }

    // 現在のシーンを更新
    if (m_currentScene && !m_currentScene->IsPaused()) {
        m_currentScene->Update(deltaTime);
    }
}

void SceneManager::Render()
{
    // 現在のシーンを描画
    if (m_currentScene) {
        m_currentScene->Render();
    }

    // シーン遷移中なら次のシーンも描画（例：クロスフェード）
    if (IsTransitioning() && m_nextScene &&
        (m_transitionInfo.state == SceneTransitionState::FadeIn ||
            m_transitionInfo.effect == SceneTransitionEffect::Crossfade)) {
        m_nextScene->Render();
    }
}

void SceneManager::RegisterScene(const std::string& name, SceneFactory factory)
{
    // 既に登録されているなら上書き
    m_sceneFactories[name] = factory;
}

void SceneManager::UnregisterScene(const std::string& name)
{
    // シーンファクトリから削除
    auto it = m_sceneFactories.find(name);
    if (it != m_sceneFactories.end()) {
        m_sceneFactories.erase(it);
    }
}

bool SceneManager::ChangeScene(const std::string& name, SceneTransitionEffect effect, float duration)
{
    // シーン遷移中なら失敗
    if (IsTransitioning()) {
        return false;
    }

    // 同じシーンなら何もしない
    if (name == m_currentSceneName) {
        return true;
    }

    // シーンファクトリを検索
    auto it = m_sceneFactories.find(name);
    if (it == m_sceneFactories.end()) {
        return false;
    }

    // 次のシーンを作成
    std::shared_ptr<Scene> nextScene = it->second();
    if (!nextScene) {
        return false;
    }

    // シーンの名前とシーンマネージャーを設定
    nextScene->SetName(name);
    nextScene->SetSceneManager(this);

    // シーンの初期化
    if (!nextScene->Initialize()) {
        return false;
    }

    // 遷移効果が「None」の場合は即座に切り替え
    if (effect == SceneTransitionEffect::None) {
        // 現在のシーンを解放
        if (m_currentScene) {
            m_currentScene->OnDeactivated();
            m_currentScene->Unload();
        }

        // 次のシーンをロード
        if (!nextScene->Load()) {
            return false;
        }

        // シーンを切り替え
        m_currentScene = nextScene;
        m_currentSceneName = name;
        m_currentScene->OnActivated();

        return true;
    }

    // 遷移処理を開始
    m_nextScene = nextScene;
    StartTransition(m_currentSceneName, name, effect, duration);

    return true;
}

std::shared_ptr<Scene> SceneManager::GetCurrentScene() const
{
    return m_currentScene;
}

std::string SceneManager::GetCurrentSceneName() const
{
    return m_currentSceneName;
}

bool SceneManager::IsTransitioning() const
{
    return m_transitionInfo.state != SceneTransitionState::None;
}

const SceneTransitionInfo& SceneManager::GetTransitionInfo() const
{
    return m_transitionInfo;
}

void SceneManager::UpdateTransition(float deltaTime)
{
    // 経過時間を更新
    m_transitionInfo.elapsedTime += deltaTime;

    // 状態に応じた処理
    switch (m_transitionInfo.state) {
    case SceneTransitionState::FadeOut:
        // フェードアウト完了
        if (m_transitionInfo.elapsedTime >= m_transitionInfo.duration * 0.5f) {
            // 現在のシーンを非アクティブ化
            if (m_currentScene) {
                m_currentScene->OnDeactivated();

                // バックグラウンドロードでない場合はアンロード
                if (!m_transitionInfo.loadInBackground) {
                    m_currentScene->Unload();
                }
            }

            // 次のシーンをロード
            if (m_nextScene) {
                if (!m_nextScene->Load()) {
                    // ロード失敗
                    CompleteTransition();
                    return;
                }
            }

            // フェードイン状態に遷移
            m_transitionInfo.state = SceneTransitionState::FadeIn;
            m_transitionInfo.elapsedTime = 0.0f;
        }
        break;

    case SceneTransitionState::FadeIn:
        // フェードイン完了
        if (m_transitionInfo.elapsedTime >= m_transitionInfo.duration * 0.5f) {
            // シーンを切り替え
            m_currentScene = m_nextScene;
            m_currentSceneName = m_transitionInfo.toSceneName;
            m_nextScene = nullptr;

            // 新しいシーンをアクティブ化
            if (m_currentScene) {
                m_currentScene->OnActivated();
            }

            // 遷移完了
            CompleteTransition();
        }
        break;

    default:
        break;
    }
}

void SceneManager::StartTransition(const std::string& fromSceneName, const std::string& toSceneName,
    SceneTransitionEffect effect, float duration)
{
    m_transitionInfo.fromSceneName = fromSceneName;
    m_transitionInfo.toSceneName = toSceneName;
    m_transitionInfo.effect = effect;
    m_transitionInfo.duration = duration;
    m_transitionInfo.elapsedTime = 0.0f;
    m_transitionInfo.state = SceneTransitionState::FadeOut;
    m_transitionInfo.loadInBackground = false; // デフォルトは非バックグラウンドロード
}

void SceneManager::CompleteTransition()
{
    m_transitionInfo.state = SceneTransitionState::None;
    m_transitionInfo.elapsedTime = 0.0f;
}