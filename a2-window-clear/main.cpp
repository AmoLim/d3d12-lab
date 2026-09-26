
#include <iostream>
#include <memory>
#include <windows.h>

#include "Core/App.h"


int main() {

    App app{};
    try {
        app.InitializeWindow();

        ShowWindow(app.GetHWnd(), SW_SHOW);

        MSG msg{};

        while (app.IsRunning()) {
            const BOOL result = GetMessageW(&msg, nullptr, 0, 0);
            if (result == -1) {
                Utils::CheckWin32(FALSE, "GetMessageW");
            }

            if (result == 0) {
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);

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