#pragma once
#include <Windows.h>
#include <tchar.h>
#include <string>

class DXRenderer;

class DXApplication {
public:
    DXApplication(int width, int height);
    ~DXApplication();

    bool Initialize();
    int Run();

    // Getters
    HWND GetHwnd() const { return m_hwnd; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    HWND m_hwnd;
    HINSTANCE m_hInstance;
    int m_width;
    int m_height;
    DXRenderer* m_renderer;
    bool m_isRunning;
    std::wstring m_windowTitle;

    void ProcessMessage(MSG& msg);
};