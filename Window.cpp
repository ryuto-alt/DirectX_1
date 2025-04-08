#include "Window.h"
#include <windowsx.h>

Window::Window()
    : m_hwnd(nullptr)
    , m_hInstance(nullptr)
    , m_title(L"DirectX12 Game Engine")
    , m_width(1280)
    , m_height(720)
    , m_fullscreen(false)
{
    ZeroMemory(&m_windowedRect, sizeof(RECT));
}

Window::~Window()
{
    Shutdown();
}

bool Window::Initialize(HINSTANCE hInstance, int nCmdShow, const std::wstring& title, int width, int height)
{
    m_hInstance = hInstance;
    m_title = title;
    m_width = width;
    m_height = height;

    // ウィンドウクラスの登録
    if (!RegisterWindowClass(hInstance)) {
        return false;
    }

    // ウィンドウのスタイル
    DWORD windowStyle = WS_OVERLAPPEDWINDOW;

    // ウィンドウサイズの調整（クライアント領域が指定したサイズになるように）
    RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&windowRect, windowStyle, FALSE);

    // ウィンドウの作成
    m_hwnd = CreateWindowW(
        L"DirectX12GameEngine",           // ウィンドウクラス名
        title.c_str(),                    // ウィンドウタイトル
        windowStyle,                      // ウィンドウスタイル
        CW_USEDEFAULT,                    // X座標
        CW_USEDEFAULT,                    // Y座標
        windowRect.right - windowRect.left, // 幅
        windowRect.bottom - windowRect.top, // 高さ
        nullptr,                          // 親ウィンドウ
        nullptr,                          // メニュー
        hInstance,                        // インスタンスハンドル
        this                              // 追加のパラメータ
    );

    if (!m_hwnd) {
        return false;
    }

    // ウィンドウの表示
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    // クライアント領域のサイズを取得
    GetClientSize(m_width, m_height);

    return true;
}

void Window::Shutdown()
{
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    UnregisterClassW(L"DirectX12GameEngine", m_hInstance);
}

bool Window::RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Window::WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"DirectX12GameEngine";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    return RegisterClassExW(&wcex) != 0;
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // UserDataから自身のインスタンスポインタを取得
    Window* window = nullptr;

    if (message == WM_NCCREATE) {
        // ウィンドウ作成時に、ウィンドウインスタンスをUserDataに関連付ける
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = static_cast<Window*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    else {
        // UserDataからウィンドウインスタンスを取得
        window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        if (window) {
            // ウィンドウサイズ変更時の処理
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            // 最小化の場合は更新しない
            if (wParam != SIZE_MINIMIZED) {
                window->m_width = width;
                window->m_height = height;
                // ここでレンダラーにリサイズの通知が必要（レンダラークラスで処理）
            }
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            // ESCキーでアプリ終了
            PostQuitMessage(0);
        }
        else if (wParam == VK_F11 && window) {
            // F11キーでフルスクリーン切り替え
            window->ToggleFullscreen();
        }
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

void Window::GetClientSize(int& width, int& height) const
{
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    width = clientRect.right - clientRect.left;
    height = clientRect.bottom - clientRect.top;
}

void Window::SetSize(int width, int height)
{
    if (m_fullscreen) {
        return; // フルスクリーンモード時は変更しない
    }

    RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&windowRect, GetWindowLong(m_hwnd, GWL_STYLE), FALSE);

    SetWindowPos(
        m_hwnd,
        nullptr,
        0, 0,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
    );

    // クライアント領域のサイズを更新
    GetClientSize(m_width, m_height);
}

void Window::SetTitle(const std::wstring& title)
{
    m_title = title;
    SetWindowText(m_hwnd, title.c_str());
}

void Window::ToggleFullscreen()
{
    m_fullscreen = !m_fullscreen;

    if (m_fullscreen) {
        // 現在のウィンドウ位置とサイズを保存
        GetWindowRect(m_hwnd, &m_windowedRect);

        // フルスクリーンモードに切り替え
        UINT flags = SWP_FRAMECHANGED | SWP_NOACTIVATE;

        // ボーダーレスフルスクリーンスタイルに変更
        SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP);

        // モニターの解像度を取得
        HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO info = {};
        info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(monitor, &info);

        SetWindowPos(
            m_hwnd,
            HWND_TOP,
            info.rcMonitor.left, info.rcMonitor.top,
            info.rcMonitor.right - info.rcMonitor.left,
            info.rcMonitor.bottom - info.rcMonitor.top,
            flags
        );
    }
    else {
        // ウィンドウモードに戻す
        SetWindowLong(m_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

        SetWindowPos(
            m_hwnd,
            HWND_TOP,
            m_windowedRect.left, m_windowedRect.top,
            m_windowedRect.right - m_windowedRect.left,
            m_windowedRect.bottom - m_windowedRect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE
        );
    }

    // クライアント領域のサイズを更新
    GetClientSize(m_width, m_height);
}