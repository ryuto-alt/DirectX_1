#include "Engine.h"
#include "Window.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "SpriteRenderer.h"
#include "ModelRenderer.h"
#include "InputManager.h"
#include "PhysicsSystem.h"
#include "AudioSystem.h"
#include "SceneManager.h"

#include <Windows.h>

Engine::Engine()
    : m_deltaTime(0.0f)
    , m_prevTime(0)
    , m_secondsPerCount(0.0)
    , m_isRunning(false)
{
    // パフォーマンスカウンタの周波数を取得
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    m_secondsPerCount = 1.0 / static_cast<double>(frequency.QuadPart);
}

Engine::~Engine()
{
    Shutdown();
}

Engine& Engine::GetInstance()
{
    static Engine instance;
    return instance;
}

bool Engine::Initialize(HINSTANCE hInstance, int nCmdShow, const std::wstring& windowTitle, int windowWidth, int windowHeight)
{
    // ウィンドウの初期化
    m_window = std::make_unique<Window>();
    if (!m_window->Initialize(hInstance, nCmdShow, windowTitle, windowWidth, windowHeight)) {
        return false;
    }

    // レンダラーの初期化
    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->Initialize(m_window.get())) {
        return false;
    }

    // リソースマネージャーの初期化
    m_resourceManager = std::make_unique<ResourceManager>();
    if (!m_resourceManager->Initialize(m_renderer->GetDevice(), m_renderer->GetCommandList())) {
        return false;
    }

    // スプライトレンダラーの初期化
    m_spriteRenderer = std::make_unique<SpriteRenderer>();
    if (!m_spriteRenderer->Initialize(m_renderer.get(), m_resourceManager.get())) {
        return false;
    }

    // モデルレンダラーの初期化
    m_modelRenderer = std::make_unique<ModelRenderer>();
    if (!m_modelRenderer->Initialize(m_renderer.get(), m_resourceManager.get())) {
        return false;
    }

    // 入力マネージャーの初期化
    m_inputManager = std::make_unique<InputManager>();
    if (!m_inputManager->Initialize(m_window->GetHwnd())) {
        return false;
    }

    // 物理システムの初期化
    m_physicsSystem = std::make_unique<PhysicsSystem>();
    if (!m_physicsSystem->Initialize()) {
        return false;
    }

    // オーディオシステムの初期化
    m_audioSystem = std::make_unique<AudioSystem>();
    if (!m_audioSystem->Initialize()) {
        return false;
    }

    // シーンマネージャーの初期化
    m_sceneManager = std::make_unique<SceneManager>();
    if (!m_sceneManager->Initialize()) {
        return false;
    }

    // 時間の初期化
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    m_prevTime = currentTime.QuadPart;

    m_isRunning = true;
    return true;
}

void Engine::Run()
{
    // メインゲームループ
    MSG msg = {};

    while (m_isRunning)
    {
        // Windowsメッセージの処理
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                m_isRunning = false;
                break;
            }
        }

        if (!m_isRunning)
            break;

        // デルタタイムの更新
        UpdateDeltaTime();

        // 入力の更新
        m_inputManager->Update();

        // シーンの更新
        m_sceneManager->Update(m_deltaTime);

        // 物理システムの更新
        m_physicsSystem->Update(m_deltaTime);

        // レンダリングの開始
        m_renderer->BeginFrame();

        // シーンの描画
        m_sceneManager->Render();

        // レンダリングの終了
        m_renderer->EndFrame();
    }
}

void Engine::Shutdown()
{
    // シーンマネージャーの終了処理
    if (m_sceneManager) {
        m_sceneManager->Shutdown();
    }

    // オーディオシステムの終了処理
    if (m_audioSystem) {
        m_audioSystem->Shutdown();
    }

    // 物理システムの終了処理
    if (m_physicsSystem) {
        m_physicsSystem->Shutdown();
    }

    // 入力マネージャーの終了処理
    if (m_inputManager) {
        m_inputManager->Shutdown();
    }

    // モデルレンダラーの終了処理
    if (m_modelRenderer) {
        m_modelRenderer->Shutdown();
    }

    // スプライトレンダラーの終了処理
    if (m_spriteRenderer) {
        m_spriteRenderer->Shutdown();
    }

    // リソースマネージャーの終了処理
    if (m_resourceManager) {
        m_resourceManager->Shutdown();
    }

    // レンダラーの終了処理
    if (m_renderer) {
        m_renderer->Shutdown();
    }

    // ウィンドウの終了処理
    if (m_window) {
        m_window->Shutdown();
    }
}

void Engine::UpdateDeltaTime()
{
    // 現在の時間を取得
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    // 前回のフレームからの経過時間を計算
    m_deltaTime = static_cast<float>((currentTime.QuadPart - m_prevTime) * m_secondsPerCount);

    // 現在の時間を保存
    m_prevTime = currentTime.QuadPart;

    // デルタタイムの値が大きすぎる場合は制限する（例：0.25秒以上）
    if (m_deltaTime > 0.25f)
        m_deltaTime = 0.25f;
}

ID3D12Device* Engine::GetDevice() const
{
    if (m_renderer) {
        return m_renderer->GetDevice();
    }
    return nullptr;
}

ID3D12CommandQueue* Engine::GetCommandQueue() const
{
    if (m_renderer) {
        return m_renderer->GetCommandQueue();
    }
    return nullptr;
}