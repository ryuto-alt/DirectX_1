#include <Windows.h>
#include "Engine.h"
#include "GameScene.h"
#include "SceneManager.h"

// Windowsアプリケーションのエントリポイント
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // エンジンの取得
    Engine& engine = Engine::GetInstance();

    // エンジンの初期化
    if (!engine.Initialize(hInstance, nCmdShow, L"DirectX12 Game Engine", 1280, 720)) {
        MessageBox(NULL, L"エンジンの初期化に失敗しました。", L"エラー", MB_OK | MB_ICONERROR);
        return 1;
    }

    // シーンの登録
    engine.GetSceneManager()->RegisterScene("GameScene", []() {
        return std::make_shared<GameScene>();
        });

    // 初期シーンの設定
    if (!engine.GetSceneManager()->ChangeScene("GameScene")) {
        MessageBox(NULL, L"初期シーンの読み込みに失敗しました。", L"エラー", MB_OK | MB_ICONERROR);
        return 1;
    }

    // メインループの実行
    engine.Run();

    // エンジンの終了処理
    engine.Shutdown();

    return 0;
}