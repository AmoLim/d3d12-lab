#include <iostream>
#include <windows.h>

#include "App.h"
#include "utilities.h"

int main() {
#if !defined(_DEBUG)
    std::cerr << "[FAIL] A2 requires a Debug build.\n";
    return 1;
#endif

    App app;

    try {
        app.InitializeWindow();

        // break point
        RECT client{};
        CheckWin32(GetClientRect(app.GetWindowHandle(), &client), "GetClientRect");
        std::cout << "[Info] Client: " << client.right - client.left << "x" << client.bottom - client.top << std::endl;

        ShowWindow(app.GetWindowHandle(), SW_SHOW);

        MSG msg{};
        int shownSelection = app.GetSelected();
        while (app.IsRunning()) {
            const BOOL result = GetMessageW(&msg, nullptr, 0, 0);
            if (result == -1) CheckWin32(FALSE, "GetMessageW");

            if (result == 0) {
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);

            if (!app.IsRunning()) {
                break;
            }

            if (shownSelection != app.GetSelected()) {
                const std::wstring windowTitle = L"A2 Win32 | Key " + std::to_wstring(app.GetSelected());
                CheckWin32(SetWindowTextW(app.GetWindowHandle(), windowTitle.c_str()), "SetWindowTextW");
                shownSelection = app.GetSelected();
                std::cout << "[INFO] Selected: " << shownSelection << std::endl;
            }
        }

        app.Shutdown();
        std::cout << "[INFO] Normal Shutdown." << std::endl;

    } catch(std::exception& error) {
        std::cerr << "[Fail] " << error.what() << '\n';
        std::cerr << "[FAIL] Fatal exit; this run is not an A2 pass.\n" << std::flush;
        app.Shutdown();
        ExitProcess(1);
    }

    return 0;
}