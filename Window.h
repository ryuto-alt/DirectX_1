#pragma once

#include <Windows.h>
#include <string>

// ウィンドウを管理するクラス
class Window {
public:
    Window();
    ~Window();

    // ウィンドウの初期化
    bool Initialize(HINSTANCE hInstance, int nCmdShow, const std::wstring& title, int width, int height);

    // ウィンドウの終了処理
    void Shutdown();

    // ウィンドウハンドルの取得
    HWND GetHwnd() const { return m_hwnd; }

    // ウィンドウサイズの取得
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    // アスペクト比の取得
    float GetAspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }

    // ウィンドウのクライアント領域を取得
    void GetClientSize(int& width, int& height) const;

    // ウィンドウのサイズを設定
    void SetSize(int width, int height);

    // ウィンドウのタイトルを設定
    void SetTitle(const std::wstring& title);

    // フルスクリーンモードの切り替え
    void ToggleFullscreen();

    // フルスクリーンモードの取得
    bool IsFullscreen() const { return m_fullscreen; }

    // ウィンドウのプロシージャ
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    // ウィンドウクラスの登録
    bool RegisterWindowClass(HINSTANCE hInstance);

    HWND m_hwnd;                // ウィンドウハンドル
    HINSTANCE m_hInstance;      // インスタンスハンドル
    std::wstring m_title;       // ウィンドウタイトル
    int m_width;                // ウィンドウ幅
    int m_height;               // ウィンドウ高さ
    bool m_fullscreen;          // フルスクリーンフラグ
    RECT m_windowedRect;        // ウィンドウモード時の矩形
};