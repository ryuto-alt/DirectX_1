#include "DXApplication.h"

#ifdef _DEBUG
#include <iostream>
#endif

// Entry point for the application
#ifdef _DEBUG
int main()
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
    // Constants
    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;

    // Create the application instance
    DXApplication app(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Initialize the application
    if (!app.Initialize())
    {
#ifdef _DEBUG
        std::cerr << "Failed to initialize the application" << std::endl;
#endif
        return 1;
    }

    // Run the application
    return app.Run();
}