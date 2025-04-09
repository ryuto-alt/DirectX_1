#include "DXApplication.h"
#include "DXRenderer.h"
#include <iostream>

DXApplication::DXApplication(int width, int height)
    : m_width(width), m_height(height), m_isRunning(false), m_renderer(nullptr)
{
    m_hInstance = GetModuleHandle(nullptr);
    m_windowTitle = L"DirectX Application";
}

DXApplication::~DXApplication()
{
    if (m_renderer)
    {
        delete m_renderer;
        m_renderer = nullptr;
    }
}

bool DXApplication::Initialize()
{
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = _T("DXApplicationClass");

    if (!RegisterClassEx(&wc))
    {
        std::cerr << "Failed to register window class" << std::endl;
        return false;
    }

    // Calculate window size based on desired client area
    RECT windowRect = { 0, 0, m_width, m_height };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window
    m_hwnd = CreateWindow(
        wc.lpszClassName,
        m_windowTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        m_hInstance,
        nullptr
    );

    if (!m_hwnd)
    {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    // Store this pointer for use in the static window procedure
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Create and initialize the renderer
    m_renderer = new DXRenderer(m_hwnd, m_width, m_height);
    if (!m_renderer->Initialize())
    {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }

    // Show the window
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    m_isRunning = true;
    return true;
}

int DXApplication::Run()
{
    MSG msg = {};

    // Main application loop
    while (m_isRunning)
    {
        // Process all pending Windows messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ProcessMessage(msg);
            if (!m_isRunning)
                break;
        }

        // Render frame
        if (m_isRunning)
        {
            m_renderer->Render();
        }
    }

    return static_cast<int>(msg.wParam);
}

void DXApplication::ProcessMessage(MSG& msg)
{
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    // Check if the application should exit
    if (msg.message == WM_QUIT)
    {
        m_isRunning = false;
    }
}

LRESULT CALLBACK DXApplication::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Get the application instance associated with this window
    DXApplication* app = reinterpret_cast<DXApplication*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        // Handle window resize if needed
        if (app && app->m_renderer)
        {
            // Implement resize logic if needed
        }
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}