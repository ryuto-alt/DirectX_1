#include "DXApplication.h"
#include "DXRenderer.h"
#include <iostream>
#include <combaseapi.h>  // CoInitializeEx, CoUninitialize

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

    // アプリケーションの実行
    int result = app.Run();

    // COM のクリーンアップ
    CoUninitialize();

    return result;
}
