#include <Windows.h>
#include <tchar.h>
#ifdef _DEBUG
#include <iostream>
#endif

using namespace std;

// ウィンドウの幅と高さを指定
const int window_width = 800;
const int window_height = 600;

///@brief コンソール画面にフォーマット付き文字列を表示
///@param format フォーマット(%dとか%fとかの)
///@param 可変長引数
///@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
    va_list valist;
    va_start(valist, format);
    vprintf(format, valist);  // 修正: printf ではなく vprintf を使う
    va_end(valist);
#endif // DEBUG
}

// WindowProcedure のプロトタイプ宣言
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#ifdef _DEBUG
int main() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
    DebugOutputFormatString("Show window test.\n");

    // ウィンドウクラスの生成
    WNDCLASSEX w = {};
    w.cbSize = sizeof(WNDCLASSEX);
    w.lpfnWndProc = (WNDPROC)WindowProcedure;  // コールバック関数の指定
    w.lpszClassName = _T("DX12Sample");
    w.hInstance = GetModuleHandle(nullptr);  // 修正: w.hinstance -> w.hInstance

    RegisterClassEx(&w);  // アプリケーションクラス
    RECT wrc{ 0, 0, window_width, window_height };  // ウィンドウサイズを決める

    // 関数を使ってウィンドウのサイズを補正する
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    // ウィンドウオブジェクトの生成
    HWND hwnd = CreateWindow(w.lpszClassName,
        _T("DX12テスト"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        w.hInstance,
        nullptr);
    // ウィンドウ表示
    ShowWindow(hwnd, SW_SHOW);

    // メッセージループ
    MSG msg = {};
    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        //アプリケーションが終わるときにメッセージがWM_QUITになる
        if (msg.message==WM_QUIT) {
            break;
        }
    }

    //もうクラスは使わないので登録解除する
    UnregisterClass(w.lpszClassName, w.hInstance);

    return 0;
}

// ウィンドウプロシージャ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    // ウィンドウが破棄されたら呼ばれる
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);  // OSに対して「このアプリは終わる」と伝える
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);  // 規定の処理を行う
}
