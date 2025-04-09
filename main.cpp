#include "DXApplication.h"
#include "DXRenderer.h"
#include "OBJLoader.h"
#include "SimpleModelLoader.h" // 簡易版ローダーを使用する場合
#include "PathUtils.h"
#include <iostream>
#include <combaseapi.h>  // CoInitializeEx, CoUninitialize

// OBJモデル描画するための関数
void LoadAndRenderOBJModel(DXRenderer* renderer, const std::string& objFilePath) {
    if (!renderer) return;

    // モデルをロード
    OBJLoader objLoader;
    std::unique_ptr<Model> model = objLoader.LoadModel(renderer->GetDevice(), objFilePath);

    if (!model) {
        std::cerr << "Failed to load model: " << objFilePath << std::endl;
        return;
    }

    // モデルの描画設定を追加
    renderer->AddModelForRendering(std::move(model));
}

#ifdef _DEBUG
int main()
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
    // COM を初期化（マルチスレッド用）
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "COM initialization failed, hr = 0x" << std::hex << hr << std::endl;
        return 1;
    }

    // ウィンドウサイズの定数
    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;

    // アプリケーションの作成
    DXApplication app(WINDOW_WIDTH, WINDOW_HEIGHT);

    // アプリの初期化
    if (!app.Initialize())
    {
#ifdef _DEBUG
        std::cerr << "Failed to initialize the application" << std::endl;
#endif
        CoUninitialize();
        return 1;
    }

    // レンダラーの取得
    DXRenderer* renderer = app.GetRenderer();
    if (!renderer) {
        std::cerr << "Failed to get renderer" << std::endl;
        CoUninitialize();
        return 1;
    }

    // OBJモデルのパス
    std::string objFilePath = "models/cube.obj"; // OBJファイルのパスを指定

    // モデルの読み込みと描画設定
    LoadAndRenderOBJModel(renderer, objFilePath);

    // アプリケーションの実行
    int result = app.Run();

    // COM のクリーンアップ
    CoUninitialize();

    return result;
}