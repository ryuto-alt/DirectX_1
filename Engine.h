#pragma once

#include <memory>
#include <string>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <DirectXMath.h>

// 前方宣言
class Window;
class ResourceManager;
class Renderer;
class SpriteRenderer;
class ModelRenderer;
class InputManager;
class PhysicsSystem;
class AudioSystem;
class SceneManager;

// ゲームエンジンのメインクラス
class Engine {
public:
    // シングルトンインスタンスの取得
    static Engine& GetInstance();

    // エンジンの初期化
    bool Initialize(HINSTANCE hInstance, int nCmdShow, const std::wstring& windowTitle, int windowWidth, int windowHeight);

    // メインループの実行
    void Run();

    // エンジンの終了
    void Shutdown();

    // 各サブシステムへのアクセス
    Window* GetWindow() const { return m_window.get(); }
    ResourceManager* GetResourceManager() const { return m_resourceManager.get(); }
    Renderer* GetRenderer() const { return m_renderer.get(); }
    SpriteRenderer* GetSpriteRenderer() const { return m_spriteRenderer.get(); }
    ModelRenderer* GetModelRenderer() const { return m_modelRenderer.get(); }
    InputManager* GetInputManager() const { return m_inputManager.get(); }
    PhysicsSystem* GetPhysicsSystem() const { return m_physicsSystem.get(); }
    AudioSystem* GetAudioSystem() const { return m_audioSystem.get(); }
    SceneManager* GetSceneManager() const { return m_sceneManager.get(); }

    // デバイスとコマンドキューへのアクセス
    ID3D12Device* GetDevice() const;
    ID3D12CommandQueue* GetCommandQueue() const;

    // デルタタイムの取得
    float GetDeltaTime() const { return m_deltaTime; }

private:
    // シングルトンのためにコンストラクタとデストラクタは非公開
    Engine();
    ~Engine();

    // コピー不可
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // フレーム時間の更新
    void UpdateDeltaTime();

    // 各サブシステム
    std::unique_ptr<Window> m_window;
    std::unique_ptr<ResourceManager> m_resourceManager;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<SpriteRenderer> m_spriteRenderer;
    std::unique_ptr<ModelRenderer> m_modelRenderer;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<PhysicsSystem> m_physicsSystem;
    std::unique_ptr<AudioSystem> m_audioSystem;
    std::unique_ptr<SceneManager> m_sceneManager;

    // 時間関連
    float m_deltaTime;
    __int64 m_prevTime;
    double m_secondsPerCount;

    // 終了フラグ
    bool m_isRunning;
};