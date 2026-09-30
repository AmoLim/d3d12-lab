
#include <iostream>
#include <memory>
#include <windows.h>

#include "Core/App.h"
#include "Utils/Exception.h"


int main() {

    App app{};
    try {
        app.InitializeWindow();
        app.InitializeRenderer();

        ShowWindow(app.GetHWnd(), SW_SHOW);

        MSG msg{};

        while (app.IsRunning()) {
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (!app.IsRunning()) {
                break;
            }
        }

        app.CleanUp();
        std::cout << "[Info] Normal ShutDown" << std::endl;

    } catch (std::exception& error) {
        std::cerr << "[Fail] " << error.what() << std::endl;
        app.CleanUp();
        ExitProcess(1);
    }

    return 0;
}
